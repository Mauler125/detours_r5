//===========================================================================//
//
// Purpose: Implements all the functions exported by the GameUI dll.
//
// $NoKeywords: $
//===========================================================================//

#include <core/stdafx.h>
#include <tier1/cvar.h>
#include <engine/sys_utils.h>
#include <vgui/vgui_debugpanel.h>
#include <vgui/vgui_baseui_interface.h>
#include <vguimatsurface/MatSystemSurface.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEngineVGui::VPaint(CEngineVGui* const thisptr, const PaintMode_t mode)
{
	int result = CEngineVGui__Paint(thisptr, mode);

	if (/*mode == PaintMode_t::PAINT_UIPANELS ||*/ mode == PaintMode_t::PAINT_INGAMEPANELS) // Render in-main menu and in-game.
	{
		g_TextOverlay.Update();
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
void VEngineVGui::Detour(const bool bAttach) const
{
	DetourSetup(&CEngineVGui__Paint, &CEngineVGui::VPaint, bAttach);
}

///////////////////////////////////////////////////////////////////////////////