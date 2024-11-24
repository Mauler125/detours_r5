#pragma once
#ifndef DEDICATED // This file should not be compiled for DEDICATED!
//------------------------------
#include <d3d11.h>

/////////////////////////////////////////////////////////////////////////////
// Internals
void DirectX_Init();
void DirectX_Shutdown();

extern HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags);
extern bool LoadTextureBuffer(unsigned char* buffer, int len, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);

extern void ResetInput();

/////////////////////////////////////////////////////////////////////////////
// Typedefs
typedef HRESULT(__stdcall* IDXGISwapChainPresent)(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags);
typedef HRESULT(__stdcall* IDXGIResizeBuffers)   (IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags);

/////////////////////////////////////////////////////////////////////////////
// Enums
enum class D3D11DeviceVTbl : short
{
	// IUnknown
	QueryInterface                       = 0,
	AddRef                               = 1,
	Release                              = 2,

	// ID3D11Device
	CreateBuffer                         = 3,
	CreateTexture1D                      = 4,
	CreateTexture2D                      = 5,
	CreateTexture3D                      = 6,
	CreateShaderResourceView             = 7,
	CreateUnorderedAccessView            = 8,
	CreateRenderTargetView               = 9,
	CreateDepthStencilView               = 10,
	CreateInputLayout                    = 11,
	CreateVertexShader                   = 12,
	CreateGeometryShader                 = 13,
	CreateGeometryShaderWithStreamOutput = 14,
	CreatePixelShader                    = 15,
	CreateHullShader                     = 16,
	CreateDomainShader                   = 17,
	CreateComputeShader                  = 18,
	CreateClassLinkage                   = 19,
	CreateBlendState                     = 20,
	CreateDepthStencilState              = 21,
	CreateRasterizerState                = 22,
	CreateSamplerState                   = 23,
	CreateQuery                          = 24,
	CreatePredicate                      = 25,
	CreateCounter                        = 26,
	CreateDeferredContext                = 27,
	OpenSharedResource                   = 28,
	CheckFormatSupport                   = 29,
	CheckMultisampleQualityLevels        = 30,
	CheckCounterInfo                     = 31,
	CheckCounter                         = 32,
	CheckFeatureSupport                  = 33,
	GetPrivateData                       = 34,
	SetPrivateData                       = 35,
	SetPrivateDataInterface              = 36,
	GetFeatureLevel                      = 37,
	GetCreationFlags                     = 38,
	GetDeviceRemovedReason               = 39,
	GetImmediateContext                  = 40,
	SetExceptionMode                     = 41,
	GetExceptionMode                     = 42,
};

enum class DXGISwapChainVTbl : short
{
	// IUnknown
	QueryInterface                       = 0,
	AddRef                               = 1,
	Release                              = 2,

	// IDXGIObject
	SetPrivateData                       = 3,
	SetPrivateDataInterface              = 4,
	GetPrivateData                       = 5,
	GetParent                            = 6,

	// IDXGIDeviceSubObject
	GetDevice                            = 7,

	// IDXGISwapChain
	Present                              = 8,
	GetBuffer                            = 9,
	SetFullscreenState                   = 10,
	GetFullscreenState                   = 11,
	GetDesc                              = 12,
	ResizeBuffers                        = 13,
	ResizeTarget                         = 14,
	GetContainingOutput                  = 15,
	GetFrameStatistics                   = 16,
	GetLastPresentCount                  = 17,
};

#ifndef BUILDING_LIBIMGUI
inline ID3D11Device**        g_ppGameDevice       = nullptr;
inline ID3D11DeviceContext** g_ppImmediateContext = nullptr;
inline IDXGISwapChain**      g_ppSwapChain        = nullptr;

FORCEINLINE ID3D11Device*        D3D11Device()        { Assert(g_ppGameDevice);       return (*g_ppGameDevice);       }
FORCEINLINE ID3D11DeviceContext* D3D11DeviceContext() { Assert(g_ppImmediateContext); return (*g_ppImmediateContext); }
FORCEINLINE IDXGISwapChain*      D3D11SwapChain()     { Assert(g_ppSwapChain);        return (*g_ppSwapChain);        }

class VDXGI : public IDetour
{
	virtual void GetAdr(void) const;
	virtual void GetFun(void) const;
	virtual void GetVar(void) const;
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
	///////////////////////////////////////////////////////////////////////////////
};
#endif // !BUILDING_LIBIMGUI
#endif // !DEDICATED
