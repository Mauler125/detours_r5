//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#ifndef SYS_MAINWIND_H
#define SYS_MAINWIND_H
#include "inputsystem/iinputsystem.h"
#include "inputsystem/iinputstacksystem.h"

inline void (*CGame__AttachToWindow)(void);
inline void(*CGame__PlayStartupVideos)(void);
inline LRESULT (*CGame__WindowProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//-----------------------------------------------------------------------------
// Purpose: Main game interface, including message pump and window creation
//-----------------------------------------------------------------------------
class CGame
{
public:
	static void PlayStartupVideos(void);

	static LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT ImguiWindowProc(HWND hWnd, UINT& uMsg, WPARAM wParam, LPARAM lParam);

	inline HWND GetWindow() const { return m_hWindow; }
	void GetWindowRect(int* const x, int* const y, int* const w, int* const h) const;

	void SetWindowPosition(const int x, const int y);
	void SetWindowSize(const int w, const int h);

	inline int GetDesktopWidth() const { return m_iDesktopWidth; }
	inline int GetDesktopHeight() const { return m_iDesktopHeight; }
	inline int GetDesktopRefreshRate() const { return m_iDesktopRefreshRate; }
	inline float GetTVRefreshRate() const // Avoid stutter on TV's running on broadcast frame rates.
	{ return ((float)m_iDesktopRefreshRate == 59.0f || (float)m_iDesktopRefreshRate == 60.0f) ? 59.939999f : (float)m_iDesktopRefreshRate; }

	void DispatchKeyEvent(const uint64_t currentTick, const ButtonCode_t buttonCode) const;
	void DispatchAllStoredGameMessages() const;

private:
	HWND m_hWindow;
	HINSTANCE m_hInstance;
	WNDPROC m_ChainedWindowProc;
	int m_x;
	int m_y;
	int m_width;
	int m_height;
	bool m_bPostedFirstAppEvent;
	bool m_bPostFirstAppEvent;
	bool m_bExternallySuppliedWindow;
	int m_iDesktopWidth;
	int m_iDesktopHeight;
	int m_iDesktopRefreshRate;
	InputContextHandle_t m_hInputContext;
}; static_assert(sizeof(CGame) == 64);

inline CGame* g_pGame = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VGame : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CGame::AttachToWindow", CGame__AttachToWindow);
		LogFunAdr("CGame::PlayStartupVideos", CGame__PlayStartupVideos);
		LogVarAdr("g_Game", g_pGame);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 85 C9 0F 84 ?? ?? ?? ?? BA ?? ?? ?? ??").GetPtr(CGame__AttachToWindow);
		g_GameDll.FindPatternSIMD("48 8B C4 55 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??").GetPtr(CGame__PlayStartupVideos);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 33 F6").GetPtr(CGame__WindowProc);
	}
	virtual void GetVar(void) const
	{
		g_pGame = CMemory(CGame__AttachToWindow).FindPattern("48 8B 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CGame*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // SYS_MAINWIND_H