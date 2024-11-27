//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "windows/id3dx.h"
#include "windows/input.h"
#include "engine/sys_mainwind.h"
#include "engine/sys_engine.h"
#include "engine/keys.h"
#include "gameui/IConsole.h"
#include "gameui/IBrowser.h"
#include "gameui/imgui_system.h"

//-----------------------------------------------------------------------------
// Purpose: plays the startup video's
//-----------------------------------------------------------------------------
void CGame::PlayStartupVideos(void)
{
	if (!CommandLine()->CheckParm("-novid"))
	{
		CGame__PlayStartupVideos();
	}
}

//-----------------------------------------------------------------------------
// Purpose: main windows procedure
//-----------------------------------------------------------------------------
LRESULT CGame::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImguiSystem()->IsInitialized())
		ImguiWindowProc(hWnd, uMsg, wParam, lParam);

	return CGame__WindowProc(hWnd, uMsg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Purpose: imgui windows procedure
//-----------------------------------------------------------------------------
LRESULT CGame::ImguiWindowProc(HWND hWnd, UINT& uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT hr = NULL;

	if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
	{
		if (wParam == g_ImGuiConfig.m_ConsoleConfig.m_nBind0 ||
			wParam == g_ImGuiConfig.m_ConsoleConfig.m_nBind1)
		{
			g_Console.ToggleActive();
			ResetInput(); // Disable input to game when console is drawn.
		}

		if (wParam == g_ImGuiConfig.m_BrowserConfig.m_nBind0 ||
			wParam == g_ImGuiConfig.m_BrowserConfig.m_nBind1)
		{
			g_Browser.ToggleActive();
			ResetInput(); // Disable input to game when browser is drawn.
		}
	}

	if (ImguiSystem()->IsSurfaceActive())
	{//////////////////////////////////////////////////////////////////////////////
		hr = ImguiSystem()->MessageHandler(hWnd, uMsg, wParam, lParam);

		switch (uMsg)
		{
		// This is required as the game calls CInputStackSystem::SetCursorPosition(),
		// which hides the cursor. It keeps calling it as the game window is the top
		// most window, even when the ImGui window is enabled. We could in the future
		// create a new input context for the imgui system, then push it to the stack
		// after the game's context and call CInputStackSystem::EnableInputContext()
		// on the new imgui context.
		case WM_SETCURSOR:
			uMsg = WM_NULL;
			break;
		default:
			break;
		}

		g_bBlockInput = true;
	}//////////////////////////////////////////////////////////////////////////////
	else
	{
		if (g_bBlockInput.exchange(false))
		{
			// Dry run with kill focus msg to clear the keydown state, we have to do
			// this as the menu's can be closed while still holding down a key. That
			// key will remain pressed down so the next time a window is opened that
			// key will be spammed, until that particular key msg is sent here again.
			hr = ImguiSystem()->MessageHandler(hWnd, WM_KILLFOCUS, wParam, lParam);
		}
	}

	return hr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the window rect
//-----------------------------------------------------------------------------
void CGame::GetWindowRect(int* const x, int* const y, int* const w, int* const h) const
{
	if (x)
	{
		*x = m_x;
	}
	if (y)
	{
		*y = m_y;
	}
	if (w)
	{
		*w = m_width;
	}
	if (h)
	{
		*h = m_height;
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the window position
//-----------------------------------------------------------------------------
void CGame::SetWindowPosition(const int x, const int y)
{
	m_x = x;
	m_y = y;
}

//-----------------------------------------------------------------------------
// Purpose: sets the window size
//-----------------------------------------------------------------------------
void CGame::SetWindowSize(const int w, const int h)
{
	m_width = w;
	m_height = h;
}

//-----------------------------------------------------------------------------
// Purpose: dispatch key event
//-----------------------------------------------------------------------------
void CGame::DispatchKeyEvent(const uint64_t currentTick, const ButtonCode_t buttonCode) const
{
	// Controller 'hold' keys are delayed longer.
	// TODO[ AMOS ]: use ConVar's instead?
	const float delay = buttonCode == KEY_XBUTTON_BACK ? 1.0f : 0.2f;
	KeyInfo_t& keyInfo = g_pKeyInfo[buttonCode];

	if (!keyInfo.m_bKeyDown && ((currentTick - keyInfo.m_nEventTick) * 0.001f) >= delay)
	{
		KeyEvent_t keyEvent;

		keyEvent.m_pCommand = keyInfo.m_pKeyBinding[KeyInfo_t::KEY_HELD_BIND];
		keyEvent.m_nTick = buttonCode;
		keyEvent.m_bDown = true;

		v_Key_Event(keyEvent);
		keyInfo.m_bKeyDown = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: dispatch all the queued up messages
//-----------------------------------------------------------------------------
void CGame::DispatchAllStoredGameMessages() const
{
	const uint64_t ticks = Plat_MSTime();
	const short eventCount = *g_nKeyEventCount;

	for (short i = 0; i < eventCount; i++)
	{
		DispatchKeyEvent(ticks, g_pKeyEventTicks[i]);
	}
}

///////////////////////////////////////////////////////////////////////////////
void VGame::Detour(const bool bAttach) const
{
	DetourSetup(&CGame__PlayStartupVideos, &CGame::PlayStartupVideos, bAttach);
	DetourSetup(&CGame__WindowProc, &CGame::WindowProc, bAttach);
}
