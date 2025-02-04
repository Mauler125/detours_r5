//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier0/crashhandler.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "tier1/keyvalues.h"
#include "rtech/pak/pakstate.h"
#include "engine/cmodel_bsp.h"
#include "engine/sys_engine.h"
#include "engine/sys_dll2.h"
#include "geforce/reflex.h"
#include "radeon/antilag.h"
#ifndef MATERIALSYSTEM_NODX
#include "windows/id3dx.h"
#include "gameui/imgui_system.h"
#include "materialsystem/cmaterialglue.h"
#include "materialsystem/texturestreaming.h"
#endif // !MATERIALSYSTEM_NODX
#include "materialsystem/cmaterialsystem.h"

#ifndef MATERIALSYSTEM_NODX
PCLSTATS_DEFINE()
#endif // MATERIALSYSTEM_NODX

bool CMaterialSystem::Connect(CMaterialSystem* thisptr, const CreateInterfaceFn factory)
{
	const bool result = CMaterialSystem__Connect(thisptr, factory);
	return result;
}

void CMaterialSystem::Disconnect(CMaterialSystem* thisptr)
{
	CMaterialSystem__Disconnect(thisptr);
}

#ifndef MATERIALSYSTEM_NODX
static bool s_useLowLatency = false;
#endif

//-----------------------------------------------------------------------------
// Purpose: initialization of the material system
//-----------------------------------------------------------------------------
InitReturnVal_t CMaterialSystem::Init(CMaterialSystem* thisptr)
{
#ifdef MATERIALSYSTEM_NODX
	// Only load the startup pak files, as 'common_early.rpak' has assets
	// that references assets in 'startup.rpak'.
	g_pakLoadApi->LoadAsyncAndWait("startup.rpak", AlignedMemAlloc(), 5, 0);
	g_pakLoadApi->LoadAsyncAndWait("startup_sdk.rpak", AlignedMemAlloc(), 5, 0);

	// Trick: return INIT_FAILED to disable the loading of hardware
	// configuration data, since we don't need it on the dedi.
	return INIT_FAILED;
#else
	// Initialize as usual.
	s_useLowLatency = !CommandLine()->CheckParm("-gfx_disableLowLatency");

	GeForce_EnableLowLatencySDK(s_useLowLatency);
	Radeon_EnableLowLatencySDK(s_useLowLatency);

	if (s_useLowLatency)
	{
		GeForce_InitLowLatencySDK();
		Radeon_InitLowLatencySDK();

		PCLSTATS_INIT(0);
		g_PCLStatsAvailable = true;
	}

	const InitReturnVal_t result = CMaterialSystem__Init(thisptr);

	// Must be loaded after the call to CMaterialSystem::Init() as we want
	// to load startup_sdk.rpak after startup.rpak. This pak file can be
	// used to load paks as early as startup.rpak, while still offering the
	// ability to patch/update its containing assets on time.
	g_pakLoadApi->LoadAsyncAndWait("startup_sdk.rpak", AlignedMemAlloc(), 5, 0);
	return result;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: shutdown of the material system
//-----------------------------------------------------------------------------
int CMaterialSystem::Shutdown(CMaterialSystem* thisptr)
{
#ifndef MATERIALSYSTEM_NODX
	if (s_useLowLatency)
	{
		if (g_PCLStatsAvailable)
			PCLSTATS_SHUTDOWN();

		Radeon_ShutdownLowLatencySDK();
		GeForce_ShutdownLowLatencySDK();
	}
#endif

	return CMaterialSystem__Shutdown(thisptr);
}

#ifndef MATERIALSYSTEM_NODX
//---------------------------------------------------------------------------------
// Purpose: draw frame
//---------------------------------------------------------------------------------
void* __fastcall DispatchDrawCall(int64_t a1, uint64_t a2, int a3, int a4, int64_t a5, int a6, uint8_t a7, int64_t a8, uint32_t a9, uint32_t a10, int a11, __m128* a12, int a13, int64_t a14)
{
	// This only happens when the BSP is in a horrible condition (bad depth buffer draw calls!)
	// but allows you to load BSP's with virtually all missing shaders/materials and models 
	// being replaced with 'material_for_aspect/error.rpak' and 'mdl/error.rmdl'.
	if (!*s_pRenderContext)
		return nullptr;

	return v_DispatchDrawCall(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
}

//---------------------------------------------------------------------------------
// Purpose: run IDXGISwapChain::Present
//---------------------------------------------------------------------------------
ssize_t SpinPresent(void)
{
	CImguiSystem* const imguiSystem = ImguiSystem();

	if (imguiSystem->IsInitialized())
		imguiSystem->RenderFrame();

	const ssize_t val = v_SpinPresent();
	return val;
}

void* CMaterialSystem::SwapBuffers(CMaterialSystem* pMatSys)
{
	CImguiSystem* const imguiSystem = ImguiSystem();

	// See https://github.com/ocornut/imgui/issues/7615, looking for status msg
	// DXGI_STATUS_OCCLUDED isn't compatible with DXGI_SWAP_EFFECT_FLIP_DISCARD.
	// This engine however does not use the flip model.
	if (imguiSystem->IsInitialized() && D3D11SwapChain()->Present(0, DXGI_PRESENT_TEST) != DXGI_STATUS_OCCLUDED)
	{
		imguiSystem->SampleFrame();
		imguiSystem->SwapBuffers();
	}

	return CMaterialSystem__SwapBuffers(pMatSys);
}

//-----------------------------------------------------------------------------
// Purpose: finds a material
// Input  : *pMatSys - 
//			*pMaterialName - 
//			nMaterialType - 
//			nUnk - 
//			bComplain - 
// Output : pointer to material
//-----------------------------------------------------------------------------
static ConVar mat_alwaysComplain("mat_alwaysComplain", "0", FCVAR_RELEASE | FCVAR_MATERIAL_SYSTEM_THREAD, "Always complain when a material is missing");

CMaterialGlue* CMaterialSystem::FindMaterialEx(CMaterialSystem* pMatSys, const char* pMaterialName, uint8_t nMaterialType, int nUnk, bool bComplain)
{
	CMaterialGlue* pMaterial = CMaterialSystem__FindMaterialEx(pMatSys, pMaterialName, nMaterialType, nUnk, bComplain);

	if ((bComplain || mat_alwaysComplain.GetBool()) && pMaterial->IsErrorMaterial())
	{
		Error(eDLL_T::MS, NO_ERROR, "Material \"%s\" not found; replacing with \"%s\".\n", pMaterialName, pMaterial->GetName());
	}
	return pMaterial;
}

//-----------------------------------------------------------------------------
// Purpose: get screen size
// Input  : *pMatSys - 
// Output : Vector2D screen size
//-----------------------------------------------------------------------------
Vector2D CMaterialSystem::GetScreenSize(CMaterialSystem* pMatSys)
{
	Vector2D vecScreenSize;

	CMaterialSystem__GetScreenSize(pMatSys, &vecScreenSize.x, &vecScreenSize.y);

	return vecScreenSize;
}

//-----------------------------------------------------------------------------
// Purpose: same as StreamDB_CreditWorldTextures, but also takes the coverage
//          of the dynamic model into account.
// Input  : *pMatSys - 
//			*materialGlue - 
//			a3 - 
//			a4 - 
//			a5 - 
//			*pViewOrigin - 
//			tanOfHalfFov - 
//			viewWidthPixels - 
//			a9 - 
//-----------------------------------------------------------------------------
void CMaterialSystem::CreditModelTextures(CMaterialSystem* const pMatSys, CMaterialGlue* const materialGlue, __int64 a3, __int64 a4, unsigned int a5, const Vector3D* const pViewOrigin, const float tanOfHalfFov, const float viewWidthPixels, int a9)
{
	if (!materialGlue->CanCreditModelTextures())
		return;

	// If we use the GPU driven texture streaming system, do not run this code
	// as the compute shaders deals with both static and dynamic model textures.
	if (gpu_driven_tex_stream->GetBool())
		return;

	MaterialGlue_s* const material = materialGlue->Get();
	material->lastFrame = s_textureStreamMgr->thisFrame;

	v_StreamDB_CreditModelTextures(material->streamingTextureHandles, material->streamingTextureHandleCount, a3, a4, a5, pViewOrigin, tanOfHalfFov, viewWidthPixels, a9);
}

//-----------------------------------------------------------------------------
// Purpose: updates the stream camera used for getting the column from the STBSP
// Input  : *pMatSys - 
//			*camPos - 
//			*camAng - 
//			halfFovX - 
//			viewWidth - 
//-----------------------------------------------------------------------------
void CMaterialSystem::UpdateStreamCamera(CMaterialSystem* const pMatSys, const Vector3D* const camPos, 
	const QAngle* const camAng, const float halfFovX, const float viewWidth)
{
	// The stream camera is only used for the STBSP. If we use the GPU feedback
	// driven texture streaming system instead, do not run this code.
	if (gpu_driven_tex_stream->GetBool())
		return;

	// NOTE: 'camAng' is set and provided to the function below, but the actual
	// function that updates the global state (StreamDB_SetCameraPosition)
	// isn't using it. The parameter is unused.
	CMaterialSystem__UpdateStreamCamera(pMatSys, camPos, camAng, halfFovX, viewWidth);
}
#endif // !MATERIALSYSTEM_NODX

///////////////////////////////////////////////////////////////////////////////
void VMaterialSystem::Detour(const bool bAttach) const
{
	DetourSetup(&CMaterialSystem__Init, &CMaterialSystem::Init, bAttach);
	DetourSetup(&CMaterialSystem__Shutdown, &CMaterialSystem::Shutdown, bAttach);

	DetourSetup(&CMaterialSystem__Connect, &CMaterialSystem::Connect, bAttach);
	DetourSetup(&CMaterialSystem__Disconnect, &CMaterialSystem::Disconnect, bAttach);

#ifndef MATERIALSYSTEM_NODX
	DetourSetup(&CMaterialSystem__SwapBuffers, &CMaterialSystem::SwapBuffers, bAttach);
	DetourSetup(&CMaterialSystem__FindMaterialEx, &CMaterialSystem::FindMaterialEx, bAttach);

	DetourSetup(&CMaterialSystem__CreditModelTextures, &CMaterialSystem::CreditModelTextures, bAttach);
	DetourSetup(&CMaterialSystem__UpdateStreamCamera, &CMaterialSystem::UpdateStreamCamera, bAttach);

	DetourSetup(&v_DispatchDrawCall, &DispatchDrawCall, bAttach);
	DetourSetup(&v_SpinPresent, &SpinPresent, bAttach);
#endif // !MATERIALSYSTEM_NODX
}
