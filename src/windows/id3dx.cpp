#include "core/stdafx.h"
#ifndef DEDICATED // This file should not be compiled for DEDICATED!
//------------------------------
#define STB_IMAGE_IMPLEMENTATION
#include "tier0/threadtools.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "windows/id3dx.h"
#include "windows/input.h"
#include "geforce/reflex.h"
#include "gameui/IConsole.h"
#include "gameui/IBrowser.h"
#include "gameui/imgui_system.h"
#include "engine/framelimit.h"
#include "engine/sys_mainwind.h"
#include "inputsystem/inputsystem.h"
#include "materialsystem/cmaterialsystem.h"
#include "public/bitmap/stb_image.h"
#include "public/rendersystem/schema/texture.g.h"

/**********************************************************************************
-----------------------------------------------------------------------------------
File   : id3dx.cpp
Date   : 15:06:2021
Author : Kawe Mazidjatari
Purpose: Microsoft DirectX 11 'IDXGISwapChain::Present' hook implementation
-----------------------------------------------------------------------------------
History:
- 15:06:2021 | 14:56 : Created by Kawe Mazidjatari
- 17:06:2021 | 13:12 : Destroy / release objects with 'GetResizeBuffers' callback

**********************************************************************************/

///////////////////////////////////////////////////////////////////////////////////
typedef BOOL(WINAPI* IPostMessageA)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
typedef BOOL(WINAPI* IPostMessageW)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////////
static IDXGIResizeBuffers       s_fnResizeBuffers    = NULL;
static IDXGISwapChainPresent    s_fnSwapChainPresent = NULL;

///////////////////////////////////////////////////////////////////////////////////
static CFrameLimit s_FrameLimiter;

//#################################################################################
// WINDOW PROCEDURE
//#################################################################################

LRESULT CALLBACK DXGIMsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//#################################################################################
// IDXGI
//#################################################################################

static ConVar fps_max_rt("fps_max_rt", "0", FCVAR_RELEASE | FCVAR_MATERIAL_SYSTEM_THREAD, "Frame rate limiter within the render thread. -1 indicates the use of desktop refresh. 0 is disabled.", true, -1.f, true, 295.f);
static ConVar fps_max_rt_tolerance("fps_max_rt_tolerance", "0.25", FCVAR_RELEASE | FCVAR_MATERIAL_SYSTEM_THREAD, "Maximum amount of frame time before frame limiter restarts.", true, 0.f, false, 0.f);
static ConVar fps_max_rt_sleep_threshold("fps_max_rt_sleep_threshold", "0.016666667", FCVAR_RELEASE | FCVAR_MATERIAL_SYSTEM_THREAD, "Frame limiter starts to sleep when frame time exceeds this threshold.", true, 0.f, false, 0.f);

HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags)
{
	if (nFlags & DXGI_PRESENT_TEST)
		return s_fnSwapChainPresent(pSwapChain, nSyncInterval, nFlags);

	float targetFps = fps_max_rt.GetFloat();

	if (targetFps > 0.0f)
	{
		const float globalFps = fps_max->GetFloat();

		// Make sure the global fps limiter is 'unlimited'
		// before we let the rt frame limiter cap it to
		// the desktop's refresh rate; not adhering to
		// this will result in a major performance drop.
		if (globalFps == 0.0f && targetFps == -1)
			targetFps = g_pGame->GetTVRefreshRate();

		if (targetFps > 0.0f)
		{
			const float sleepThreshold = fps_max_rt_sleep_threshold.GetFloat();
			const float maxTolerance = fps_max_rt_tolerance.GetFloat();

			s_FrameLimiter.Run(targetFps, sleepThreshold, maxTolerance);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// NOTE: -1 since we need to sync this with its corresponding frame, g_FrameNum
	// gets incremented in CMaterialSystem::SwapBuffers, which is after the markers
	// for simulation start/end and render submit start. The render thread (here)
	// continues after to finish the frame.
	const NvU64 frameID = (NvU64)MaterialSystem()->GetCurrentFrameCount() - 1;
	GeForce_SetLatencyMarker(D3D11Device(), RENDERSUBMIT_END, frameID);

	GeForce_SetLatencyMarker(D3D11Device(), PRESENT_START, frameID);
	const HRESULT result = s_fnSwapChainPresent(pSwapChain, nSyncInterval, nFlags);
	GeForce_SetLatencyMarker(D3D11Device(), PRESENT_END, frameID);

	return result;
}

HRESULT __stdcall ResizeBuffers(IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags)
{
	///////////////////////////////////////////////////////////////////////////////
	g_pGame->SetWindowSize(nWidth, nHeight);
	return s_fnResizeBuffers(pSwapChain, nBufferCount, nWidth, nHeight, dxFormat, nSwapChainFlags);
}

//#################################################################################
// INTERNALS
//#################################################################################

#pragma warning( push )
// Disable stack warning, tells us to move more data to the heap instead. Not really possible with 'initialData' here. Since its parallel processed.
// Also disable 6378, complains that there is no control path where it would use 'nullptr', if that happens 'Error' will be called though.
#pragma warning( disable : 6262 6387)
void(*v_CreateTextureResource)(TextureHeader_t*, INT_PTR);
constexpr uint32_t ALIGNMENT_SIZE = 15; // Creates 2D texture and shader resource from textureHeader and imageData.
void CreateTextureResource(TextureHeader_t* textureHeader, INT_PTR imageData)
{
	if (textureHeader->m_nDepth && !textureHeader->m_nHeight) // Return never gets hit. Maybe its some debug check?
		return;

	__int64 initialData[4096]{};
	textureHeader->m_nTextureMipLevels = textureHeader->m_nPermanentMipCount;

	const int totalStreamedMips = textureHeader->m_nOptStreamedMipCount + textureHeader->m_nStreamedMipCount;
	int mipLevel = textureHeader->m_nPermanentMipCount + totalStreamedMips;
	if (mipLevel != totalStreamedMips)
	{
		do
		{
			--mipLevel;
			if (textureHeader->m_nArraySize)
			{
				int mipWidth = 0;
				if (textureHeader->m_nWidth >> mipLevel > 1)
					mipWidth = (textureHeader->m_nWidth >> mipLevel) - 1;

				int mipHeight = 0;
				if (textureHeader->m_nHeight >> mipLevel > 1)
					mipHeight = (textureHeader->m_nHeight >> mipLevel) - 1;

				uint8_t x = s_pBytesPerPixel[textureHeader->m_nImageFormat].first;
				uint8_t y = s_pBytesPerPixel[textureHeader->m_nImageFormat].second;

				uint32_t bppWidth = (y + mipWidth) >> (y >> 1);
				uint32_t bppHeight = (y + mipHeight) >> (y >> 1);
				uint32_t sliceWidth = x * (y >> (y >> 1));

				uint32_t rowPitch = sliceWidth * bppWidth;
				uint32_t slicePitch = x * bppWidth * bppHeight;

				uint32_t subResourceEntry = mipLevel;
				for (int i = 0; i < textureHeader->m_nArraySize; i++)
				{
					uint32_t offsetCurrentResourceData = subResourceEntry << 4u;

					*(int64_t*)((uint8_t*)initialData + offsetCurrentResourceData) = imageData;
					*(uint32_t*)((uint8_t*)&initialData[1] + offsetCurrentResourceData) = rowPitch;
					*(uint32_t*)((uint8_t*)&initialData[1] + offsetCurrentResourceData + 4) = slicePitch;

					imageData += (slicePitch + ALIGNMENT_SIZE) & ~ALIGNMENT_SIZE;
					subResourceEntry += textureHeader->m_nPermanentMipCount;
				}
			}
		} while (mipLevel != totalStreamedMips);
	}

	const DXGI_FORMAT dxgiFormat = g_TxtrAssetToDxgiFormat[textureHeader->m_nImageFormat]; // Get dxgi format

	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = textureHeader->m_nWidth >> mipLevel;
	textureDesc.Height = textureHeader->m_nHeight >> mipLevel;
	textureDesc.MipLevels = textureHeader->m_nPermanentMipCount;
	textureDesc.ArraySize = textureHeader->m_nArraySize;
	textureDesc.Format = dxgiFormat;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = textureHeader->m_nCPUAccessFlag != 2 ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.MiscFlags = 0;

	const uint32_t offsetStartResourceData = mipLevel << 4u;
	const D3D11_SUBRESOURCE_DATA* subResData = (D3D11_SUBRESOURCE_DATA*)((uint8_t*)initialData + offsetStartResourceData);
	const HRESULT createTextureRes = D3D11Device()->CreateTexture2D(&textureDesc, subResData, &textureHeader->m_ppTexture);
	if (createTextureRes < S_OK)
		Error(eDLL_T::RTECH, EXIT_FAILURE, "Couldn't create texture \"%s\": error code = %08x\n", textureHeader->m_pDebugName, createTextureRes);

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResource{};
	shaderResource.Format = dxgiFormat;
	shaderResource.Texture2D.MipLevels = textureHeader->m_nTextureMipLevels;
	if (textureHeader->m_nArraySize > 1) // Do we have a texture array?
	{
		shaderResource.Texture2DArray.FirstArraySlice = 0;
		shaderResource.Texture2DArray.ArraySize = textureHeader->m_nArraySize;
		shaderResource.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
	}
	else
	{
		shaderResource.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	}

	const HRESULT createShaderResourceRes = D3D11Device()->CreateShaderResourceView(textureHeader->m_ppTexture, &shaderResource, &textureHeader->m_ppShaderResourceView);
	if (createShaderResourceRes < S_OK)
		Error(eDLL_T::RTECH, EXIT_FAILURE, "Couldn't create shader resource view for texture \"%s\": error code = %08x\n", textureHeader->m_pDebugName, createShaderResourceRes);
}
#pragma warning( pop )

bool LoadTextureBuffer(unsigned char* buffer, int len, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	// Load PNG buffer to a raw RGBA buffer
	int nImageWidth = 0;
	int nImageHeight = 0;
	unsigned char* pImageData = stbi_load_from_memory(buffer, len, &nImageWidth, &nImageHeight, NULL, 4);

	if (!pImageData)
	{
		assert(pImageData);
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	ID3D11Texture2D* pTexture = nullptr;
	D3D11_TEXTURE2D_DESC            desc;
	D3D11_SUBRESOURCE_DATA          subResource;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

	///////////////////////////////////////////////////////////////////////////////
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = nImageWidth;
	desc.Height = nImageHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	///////////////////////////////////////////////////////////////////////////////
	subResource.pSysMem = pImageData;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;
	D3D11Device()->CreateTexture2D(&desc, &subResource, &pTexture);

	// Create texture view
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	if (pTexture)
	{
		D3D11Device()->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
		pTexture->Release();
	}

	*out_width = nImageWidth;
	*out_height = nImageHeight;
	stbi_image_free(pImageData);

	return true;
}

void ResetInput()
{
	// Enables the input system when no imgui surface is drawn.
	g_pInputSystem->EnableInput(!ImguiSystem()->IsSurfaceActive());
}

bool PanelsVisible()
{
	if (ImguiSystem()->IsSurfaceActive())
	{
		return true;
	}
	return false;
}

//#################################################################################
// ENTRYPOINT
//#################################################################################

void DirectX_Init()
{
	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook SwapChain
	DWORD_PTR* pSwapChainVtable = *reinterpret_cast<DWORD_PTR**>(g_ppSwapChain[0]);

	int pIDX = static_cast<int>(DXGISwapChainVTbl::Present);
	s_fnSwapChainPresent = reinterpret_cast<IDXGISwapChainPresent>(pSwapChainVtable[pIDX]);

	int rIDX = static_cast<int>(DXGISwapChainVTbl::ResizeBuffers);
	s_fnResizeBuffers = reinterpret_cast<IDXGIResizeBuffers>(pSwapChainVtable[rIDX]);

	DetourAttach(&(LPVOID&)s_fnSwapChainPresent, (PBYTE)Present);
	DetourAttach(&(LPVOID&)s_fnResizeBuffers, (PBYTE)ResizeBuffers);

	// Commit the transaction
	HRESULT hr = DetourTransactionCommit();
	if (hr != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		Assert(0);
		Error(eDLL_T::COMMON, 0xBAD0C0DE, "Failed to detour process: error code = %08x\n", hr);
	}

	if (ImguiSystem()->IsEnabled())
	{
		if (ImguiSystem()->Init())
		{
			ImguiSystem()->AddSurface(&g_Console);
			ImguiSystem()->AddSurface(&g_Browser);
		}
		else
		{
			Error(eDLL_T::COMMON, 0, "ImguiSystem()->Init() failed!\n");

			// Remove any log that was stored in the buffer for rendering
			// as the console will not render past this stage due to init
			// failure. Logging happens before the imgui surface system
			// is initialized, as the initialization needs to happen after
			// directx is initialized, but on initialization success, we
			// do want the logs prior to this stage to be displayed.
			g_Console.ClearLog();
		}
	}
}

void DirectX_Shutdown()
{
	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Unhook SwapChain
	DetourDetach(&(LPVOID&)s_fnSwapChainPresent, (PBYTE)Present);
	DetourDetach(&(LPVOID&)s_fnResizeBuffers, (PBYTE)ResizeBuffers);

	// Commit the transaction
	DetourTransactionCommit();

	if (ImguiSystem()->IsInitialized())
	{
		ImguiSystem()->Shutdown();

		ImguiSystem()->RemoveSurface(&g_Browser);
		ImguiSystem()->RemoveSurface(&g_Console);
	}
}

void VDXGI::GetAdr(void) const
{
	///////////////////////////////////////////////////////////////////////////////
	LogFunAdr("IDXGISwapChain::Present", s_fnSwapChainPresent);
	LogFunAdr("CreateTextureResource", v_CreateTextureResource);
	LogVarAdr("g_pSwapChain", g_ppSwapChain);
	LogVarAdr("g_pGameDevice", g_ppGameDevice);
	LogVarAdr("g_pImmediateContext", g_ppImmediateContext);
}

void VDXGI::GetFun(void) const
{
	g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B C7 48 8B D5 48 8B CB 48 83 C4 60").FollowNearCallSelf().GetPtr(v_CreateTextureResource);
}

void VDXGI::GetVar(void) const
{
	CMemory base = g_GameDll.FindPatternSIMD("4C 8B DC 49 89 4B 08 48 83 EC 58");

	// Grab device pointers..
	g_ppGameDevice = base.FindPattern("48 8D 05").ResolveRelativeAddressSelf(0x3, 0x7).RCast<ID3D11Device**>();
	g_ppImmediateContext = base.FindPattern("48 89 0D", CMemory::Direction::DOWN, 512, 3).ResolveRelativeAddressSelf(0x3, 0x7).RCast<ID3D11DeviceContext**>();

	// Grab swap chain..
	base = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 45 33 C0 33 D2");
	g_ppSwapChain = base.FindPattern("48 8B 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<IDXGISwapChain**>();
}

void VDXGI::Detour(const bool bAttach) const
{
	DetourSetup(&v_CreateTextureResource, &CreateTextureResource, bAttach);
}

#endif // !DEDICATED
