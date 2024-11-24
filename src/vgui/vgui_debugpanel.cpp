//===========================================================================//
//
// Purpose: Implements the debug panels.
//
// $NoKeywords: $
//===========================================================================//

#include <core/stdafx.h>
#include <tier1/cvar.h>
#include <tier1/keyvalues.h>
#include <windows/id3dx.h>
#include <mathlib/color.h>
#include <rtech/rui/rui.h>
#include <vgui/vgui_debugpanel.h>
#include <vguimatsurface/MatSystemSurface.h>
#include <materialsystem/cmaterialsystem.h>
#ifndef CLIENT_DLL
#include <engine/server/server.h>
#endif // !CLIENT_DLL
#include <engine/sys_engine.h>
#include <engine/sys_mainwind.h>
#include <engine/debugoverlay.h>
#include <engine/client/clientstate.h>
#include <materialsystem/cmaterialglue.h>

static ConVar con_drawnotify("con_drawnotify", "0", FCVAR_RELEASE | FCVAR_MATERIAL_SYSTEM_THREAD, "Draws the RUI console to the hud");

// Various cvars that dictate how many lines and how long the text is shown
static ConVar con_notifylines("con_notifylines", "3", FCVAR_MATERIAL_SYSTEM_THREAD, "Number of console lines to overlay for debugging", true, 1.f, false, 0.f);
static ConVar con_notifytime("con_notifytime", "6", FCVAR_MATERIAL_SYSTEM_THREAD, "How long to display recent console text to the upper part of the game window", true, 1.f, false, 0.f);

// Various cvars that dictate where the debug text is shown on the screen
static ConVar con_notify_invert_x("con_notify_invert_x", "0", FCVAR_MATERIAL_SYSTEM_THREAD, "Inverts the X offset for RUI console overlay");
static ConVar con_notify_invert_y("con_notify_invert_y", "0", FCVAR_MATERIAL_SYSTEM_THREAD, "Inverts the Y offset for RUI console overlay");
static ConVar con_notify_offset_x("con_notify_offset_x", "10", FCVAR_MATERIAL_SYSTEM_THREAD, "X offset for RUI console overlay");
static ConVar con_notify_offset_y("con_notify_offset_y", "10", FCVAR_MATERIAL_SYSTEM_THREAD, "Y offset for RUI console overlay");

// Various cvars that dictate the colors of script debug text
static ConVar con_notify_script_server_clr("con_notify_script_server_clr", "130 120 245 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script SERVER VM RUI console overlay log color");
static ConVar con_notify_script_client_clr("con_notify_script_client_clr", "117 116 139 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script CLIENT VM RUI console overlay log color");
static ConVar con_notify_script_ui_clr("con_notify_script_ui_clr", "200 110 110 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script UI VM RUI console overlay log color");

// Various cvars that dictate the colors of code debug text
static ConVar con_notify_native_server_clr("con_notify_native_server_clr", "20 50 248 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native SERVER RUI console overlay log color");
static ConVar con_notify_native_client_clr("con_notify_native_client_clr", "70 70 70 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native CLIENT RUI console overlay log color");
static ConVar con_notify_native_ui_clr("con_notify_native_ui_clr", "200 60 60 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native UI RUI console overlay log color");
static ConVar con_notify_native_engine_clr("con_notify_native_engine_clr", "255 255 255 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native engine RUI console overlay log color");
static ConVar con_notify_native_fs_clr("con_notify_native_fs_clr", "0 100 225 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native FileSystem RUI console overlay log color");
static ConVar con_notify_native_rtech_clr("con_notify_native_rtech_clr", "25 120 20 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native RTech RUI console overlay log color");
static ConVar con_notify_native_ms_clr("con_notify_native_ms_clr", "200 20 180 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native MaterialSystem RUI console overlay log color");
static ConVar con_notify_native_audio_clr("con_notify_native_audio_clr", "238 43 10 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native AudioSystem RUI console overlay log color");
static ConVar con_notify_native_video_clr("con_notify_native_video_clr", "115 0 235 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native VideoSystem RUI console overlay log color");

static ConVar con_notify_netcon_clr("con_notify_netcon_clr", "255 255 255 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Netconsole RUI console overlay log color");
static ConVar con_notify_common_clr("con_notify_common_clr", "255 140 80 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Common RUI console overlay log color");

static ConVar con_notify_warning_clr("con_notify_warning_clr", "180 180 20 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Warning RUI console overlay log color");
static ConVar con_notify_error_clr("con_notify_error_clr", "225 20 20 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Error RUI console overlay log color");


static ConVar cl_notify_invert_x("cl_notify_invert_x", "0", FCVAR_DEVELOPMENTONLY, "Inverts the X offset for console notify debug overlay");
static ConVar cl_notify_invert_y("cl_notify_invert_y", "0", FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for console notify debug overlay");
static ConVar cl_notify_offset_x("cl_notify_offset_x", "10", FCVAR_DEVELOPMENTONLY, "X offset for console notify debug overlay");
static ConVar cl_notify_offset_y("cl_notify_offset_y", "10", FCVAR_DEVELOPMENTONLY, "Y offset for console notify debug overlay");

static ConVar cl_showsimstats("cl_showsimstats", "0", FCVAR_DEVELOPMENTONLY, "Shows the tick counter for the server/client simulation and the render frame");
static ConVar cl_simstats_invert_x("cl_simstats_invert_x", "1", FCVAR_DEVELOPMENTONLY, "Inverts the X offset for simulation debug overlay");
static ConVar cl_simstats_invert_y("cl_simstats_invert_y", "1", FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for simulation debug overlay");
static ConVar cl_simstats_offset_x("cl_simstats_offset_x", "650", FCVAR_DEVELOPMENTONLY, "X offset for simulation debug overlay");
static ConVar cl_simstats_offset_y("cl_simstats_offset_y", "120", FCVAR_DEVELOPMENTONLY, "Y offset for simulation debug overlay");

static ConVar cl_showgpustats("cl_showgpustats", "0", FCVAR_DEVELOPMENTONLY, "Texture streaming debug overlay");
static ConVar cl_gpustats_invert_x("cl_gpustats_invert_x", "1", FCVAR_DEVELOPMENTONLY, "Inverts the X offset for texture streaming debug overlay");
static ConVar cl_gpustats_invert_y("cl_gpustats_invert_y", "1", FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for texture streaming debug overlay");
static ConVar cl_gpustats_offset_x("cl_gpustats_offset_x", "650", FCVAR_DEVELOPMENTONLY, "X offset for texture streaming debug overlay");
static ConVar cl_gpustats_offset_y("cl_gpustats_offset_y", "105", FCVAR_DEVELOPMENTONLY, "Y offset for texture streaming debug overlay");

static ConVar cl_showmaterialinfo("cl_showmaterialinfo", "0", FCVAR_DEVELOPMENTONLY, "Draw info for the material under the crosshair on screen");
static ConVar cl_materialinfo_offset_x("cl_materialinfo_offset_x", "0", FCVAR_DEVELOPMENTONLY, "X offset for material debug info overlay");
static ConVar cl_materialinfo_offset_y("cl_materialinfo_offset_y", "420", FCVAR_DEVELOPMENTONLY, "Y offset for material debug info overlay");

//-----------------------------------------------------------------------------
// Purpose: proceed a log update
//-----------------------------------------------------------------------------
void CTextOverlay::Update(void)
{
	if (!g_pMatSystemSurface)
	{
		return;
	}
	Con_NPrintf();
	if (con_drawnotify.GetBool())
	{
		DrawNotify();
	}
	if (cl_showsimstats.GetBool())
	{
		DrawSimStats();
	}
	if (cl_showgpustats.GetBool())
	{
		DrawGPUStats();
	}
	if (cl_showmaterialinfo.GetBool())
	{
		DrawCrosshairMaterial();
	}
	if (stream_overlay->GetBool())
	{
		DrawStreamOverlay();
	}
}

//-----------------------------------------------------------------------------
// Purpose: add a log to the vector.
//-----------------------------------------------------------------------------
void CTextOverlay::AddLog(const eDLL_T context, const char* pszText)
{
	Assert(pszText);

	if (!con_drawnotify.GetBool() || !VALID_CHARSTAR(pszText))
	{
		return;
	}

	AUTO_LOCK(m_Mutex);
	m_NotifyLines.AddToTail(CTextNotify{ context, con_notifytime.GetFloat() , pszText });

	while (m_NotifyLines.Count() > 0 &&
		(m_NotifyLines.Count() > con_notifylines.GetInt()))
	{
		m_NotifyLines.Remove(0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: draw notify logs on screen.
//-----------------------------------------------------------------------------
void CTextOverlay::DrawNotify(void)
{
	int w, h;
	g_pGame->GetWindowRect(nullptr, nullptr, &w, &h);

	int x = con_notify_invert_x.GetBool() ? w - con_notify_offset_x.GetInt() : con_notify_offset_x.GetInt();
	int y = con_notify_invert_y.GetBool() ? h - con_notify_offset_y.GetInt() : con_notify_offset_y.GetInt();

	AUTO_LOCK(m_Mutex);

	for (int i = 0, j = m_NotifyLines.Count(); i < j; i++)
	{
		const CTextNotify& notify = m_NotifyLines[i];
		Color c = GetLogColorForType(notify.m_Type);

		const float flTimeleft = notify.m_flLifeRemaining;

		if (flTimeleft < 1.0f)
		{
			const float f = clamp(flTimeleft, 0.0f, 1.0f) / 1.0f;
			c[3] = uint8_t(f * 255.0f);

			if (i == 0 && f < 0.2f)
			{
				y -= int(m_nFontHeight * (float(1.0f - f / 0.2f)));
			}
		}
		else
		{
			c[3] = 255;
		}
		CMatSystemSurface__DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(),
			m_nFontHeight, x, y, c.r(), c.g(), c.b(), c.a(), "%s", notify.m_Text.String());

		if (IsX360())
		{
			// For some reason the fontTall value on 360 is about twice as high as it should be
			y += 12;
		}
		else
		{
			y += m_nFontHeight;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: draws formatted text over RUI
// Input  : x - 
//			y - 
//			pszFormat - 
//			... - 
//-----------------------------------------------------------------------------
void CTextOverlay::DrawFormat(const int x, const int y, const Color c, const char* pszFormat, ...) const
{
	char szLogbuf[4096];
	{/////////////////////////////
		va_list args{};
		va_start(args, pszFormat);

		vsnprintf(szLogbuf, sizeof(szLogbuf), pszFormat, args);

		szLogbuf[sizeof(szLogbuf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	CMatSystemSurface__DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, x, y, c.r(), c.g(), c.b(), c.a(), "%s", szLogbuf);
}

//-----------------------------------------------------------------------------
// Purpose: checks if the notify text is expired
// Input  : flFrameTime - 
//-----------------------------------------------------------------------------
void CTextOverlay::ShouldDraw(const float flFrameTime)
{
	if (con_drawnotify.GetBool())
	{
		AUTO_LOCK(m_Mutex);

		FOR_EACH_VEC_BACK(m_NotifyLines, i)
		{
			CTextNotify* pNotify = &m_NotifyLines[i];
			pNotify->m_flLifeRemaining -= flFrameTime;

			if (pNotify->m_flLifeRemaining <= 0.0f)
			{
				m_NotifyLines.Remove(i);
				continue;
			}
		}
	}
	else if (!m_NotifyLines.IsEmpty())
	{
		AUTO_LOCK(m_Mutex);
		m_NotifyLines.Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: draws console messages on screen (only used for 'host_speeds'!, deprecated!!).
//-----------------------------------------------------------------------------
void CTextOverlay::Con_NPrintf(void)
{
	if (!m_szCon_NPrintf_Buf[0])
	{
		return;
	}

	int w, h;
	g_pGame->GetWindowRect(nullptr, nullptr, &w, &h);

	const int nWidth = cl_notify_invert_x.GetBool() ? w - cl_notify_offset_x.GetInt() : cl_notify_offset_x.GetInt() + m_nCon_NPrintf_Idx * m_nFontHeight;
	const int nHeight = cl_notify_invert_y.GetBool() ? h - cl_notify_offset_y.GetInt() : cl_notify_offset_y.GetInt();

	static const Color c = { 255, 255, 255, 255 };
	CMatSystemSurface__DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, nWidth, nHeight, c.r(), c.g(), c.b(), c.a(), "%s", m_szCon_NPrintf_Buf);

	m_nCon_NPrintf_Idx = 0;
	m_szCon_NPrintf_Buf[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: draws live simulation stats on screen.
//-----------------------------------------------------------------------------
void CTextOverlay::DrawSimStats(void) const
{
	int w, h;
	g_pGame->GetWindowRect(nullptr, nullptr, &w, &h);

	const int nWidth  = cl_simstats_invert_x.GetBool() ? w - cl_simstats_offset_x.GetInt() : cl_simstats_offset_x.GetInt();
	const int nHeight = cl_simstats_invert_y.GetBool() ? h - cl_simstats_offset_y.GetInt() : cl_simstats_offset_y.GetInt();

	static const Color c = { 255, 255, 255, 255 };

	DrawFormat(nWidth, nHeight, c, "Server Frame: (%d) Client Frame: (%d) Render Frame: (%d)\n", 
		g_pClientState->GetServerTickCount(), g_pClientState->GetClientTickCount(), *g_nRenderTickCount);
}

//-----------------------------------------------------------------------------
// Purpose: draws live gpu stats on screen.
//-----------------------------------------------------------------------------
void CTextOverlay::DrawGPUStats(void) const
{
	int w, h;
	g_pGame->GetWindowRect(nullptr, nullptr, &w, &h);

	const int nWidth  = cl_gpustats_invert_x.GetBool() ? w - cl_gpustats_offset_x.GetInt() : cl_gpustats_offset_x.GetInt();
	const int nHeight = cl_gpustats_invert_y.GetBool() ? h - cl_gpustats_offset_y.GetInt() : cl_gpustats_offset_y.GetInt();

	static const Color c = { 255, 255, 255, 255 };

	DrawFormat(nWidth, nHeight, c, "%8zd/%8zd/%8zdkiB unusable/unfree/total GPU Streaming Texture memory\n",
		*g_nUnusableStreamingTextureMemory / 1024, *g_nUnfreeStreamingTextureMemory / 1024, *g_nTotalStreamingTextureMemory / 1024);
}

//-----------------------------------------------------------------------------
// Purpose: draws currently traced material info on screen.
//-----------------------------------------------------------------------------
void CTextOverlay::DrawCrosshairMaterial(void) const
{
	CMaterialGlue* pMaterialGlue = v_GetMaterialAtCrossHair();
	if (!pMaterialGlue)
		return;

	static const Color c = { 255, 255, 255, 255 };
	DrawFormat(cl_materialinfo_offset_x.GetInt(), cl_materialinfo_offset_y.GetInt(), c, "name: %s\nguid: %llx\ndimensions: %d x %d\nsurface: %s/%s\nstc: %i\ntc: %i",
		pMaterialGlue->name,
		pMaterialGlue->assetGuid,
		pMaterialGlue->width, pMaterialGlue->height,
		pMaterialGlue->surfaceProp, pMaterialGlue->surfaceProp2,
		pMaterialGlue->numStreamingTextureHandles,
		pMaterialGlue->shaderset->m_nTextureInputCount);
}

//-----------------------------------------------------------------------------
// Purpose: draws the stream overlay on screen.
//-----------------------------------------------------------------------------
void CTextOverlay::DrawStreamOverlay(void) const
{
	static char szLogbuf[4096];
	static const Color c = { 255, 255, 255, 255 };
	
	CMaterialSystem__GetStreamOverlay(stream_overlay_mode->GetString(), szLogbuf, sizeof(szLogbuf));
	CMatSystemSurface__DrawColoredText(g_pMatSystemSurface, v_Rui_GetFontFace(), m_nFontHeight, 20, 300, c.r(), c.g(), c.b(), c.a(), "%s", szLogbuf);
}

//-----------------------------------------------------------------------------
// Purpose: gets the log color for context.
// Input  : context - 
// Output : Color
//-----------------------------------------------------------------------------
Color CTextOverlay::GetLogColorForType(const eDLL_T context) const
{
	switch (context)
	{
	case eDLL_T::SCRIPT_SERVER:
		return { con_notify_script_server_clr.GetColor() };
	case eDLL_T::SCRIPT_CLIENT:
		return { con_notify_script_client_clr.GetColor() };
	case eDLL_T::SCRIPT_UI:
		return { con_notify_script_ui_clr.GetColor() };
	case eDLL_T::SERVER:
		return { con_notify_native_server_clr.GetColor() };
	case eDLL_T::CLIENT:
		return { con_notify_native_client_clr.GetColor() };
	case eDLL_T::UI:
		return { con_notify_native_ui_clr.GetColor() };
	case eDLL_T::ENGINE:
		return { con_notify_native_engine_clr.GetColor() };
	case eDLL_T::FS:
		return { con_notify_native_fs_clr.GetColor() };
	case eDLL_T::RTECH:
		return { con_notify_native_rtech_clr.GetColor() };
	case eDLL_T::MS:
		return { con_notify_native_ms_clr.GetColor() };
	case eDLL_T::AUDIO:
		return { con_notify_native_audio_clr.GetColor() };
	case eDLL_T::VIDEO:
		return { con_notify_native_video_clr.GetColor() };
	case eDLL_T::NETCON:
		return { con_notify_netcon_clr.GetColor() };
	case eDLL_T::COMMON:
		return { con_notify_common_clr.GetColor() };
	case eDLL_T::SYSTEM_WARNING:
		return { con_notify_warning_clr.GetColor() };
	case eDLL_T::SYSTEM_ERROR:
		return { con_notify_error_clr.GetColor() };
	default:
		return { con_notify_native_engine_clr.GetColor() };
	}
}

///////////////////////////////////////////////////////////////////////////////
CTextOverlay g_TextOverlay;
