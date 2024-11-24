//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#if !defined( IENGINEVGUI_H )
#define IENGINEVGUI_H

#ifdef _WIN32
#pragma once
#endif

//#include "interface.h"
#include "vgui/vgui.h"

// Forward declarations.
namespace vgui
{
	class Panel;
};

// Todo: r5 seems to have the same number of enumerants, however the order is
// still unconfirmed!
enum VGuiPanel_t
{
	PANEL_ROOT = 0,
	PANEL_GAMEUIDLL,  // the console, game menu
	PANEL_CLIENTDLL,
	PANEL_TOOLS,
	PANEL_INGAMESCREENS,
	PANEL_GAMEDLL,
	PANEL_CLIENTDLL_TOOLS,
	PANEL_GAMEUIBACKGROUND, // the console background, shows under all other stuff in 3d engine view
	PANEL_TRANSITIONEFFECT,
	PANEL_STEAMOVERLAY,
};

// In-game panels are cropped to the current engine viewport size
enum PaintMode_t
{
	PAINT_UIPANELS = (1 << 0),
	PAINT_INGAMEPANELS = (1 << 1),
};

// Might not be complete:
enum LevelLoadingProgress_e
{
	PROGRESS_INVALID = -2,
	PROGRESS_DEFAULT = -1,

	PROGRESS_NONE,
	PROGRESS_CHANGELEVEL,
	PROGRESS_SPAWNSERVER,
	PROGRESS_LOADWORLDMODEL,
	PROGRESS_CRCMAP,
	PROGRESS_CRCCLIENTDLL,
	PROGRESS_CREATENETWORKSTRINGTABLES,
	PROGRESS_PRECACHEWORLD,
	PROGRESS_CLEARWORLD,
	PROGRESS_LEVELINIT,
	PROGRESS_PRECACHE,
	PROGRESS_ACTIVATESERVER,
	PROGRESS_BEGINCONNECT,
	PROGRESS_SIGNONCHALLENGE,
	PROGRESS_SIGNONCONNECT,
	PROGRESS_SIGNONCONNECTED,
	PROGRESS_PROCESSSERVERINFO,
	PROGRESS_PROCESSSTRINGTABLE,
	PROGRESS_SIGNONNEW,
	PROGRESS_SENDCLIENTINFO,
	PROGRESS_SENDSIGNONDATA,
	PROGRESS_SIGNONSPAWN,
	PROGRESS_CREATEENTITIES,
	PROGRESS_FULLYCONNECTED,
	PROGRESS_PRECACHELIGHTING,
	PROGRESS_READYTOPLAY,
	PROGRESS_HIGHESTITEM,	// must be last item in list
};

abstract_class IEngineVGui
{
public:
	virtual					~IEngineVGui(void) { }

	virtual vgui::VPANEL	GetPanel(const VGuiPanel_t type) = 0;
	virtual vgui::VPANEL	GetRootPanel(const VGuiPanel_t type) = 0;

	virtual void			Unknown0() = 0;
	virtual bool			Unknown1() = 0; // ReturnFalse

	virtual bool			IsGameUIVisible() = 0;

	virtual void			ActivateGameUI() = 0;
	virtual void			HideGameUI() = 0;

	virtual void			Simulate() = 0;

	virtual bool			IsNotAllowedToHideGameUI() = 0;

	virtual void			SetRuiFuncs(void* const ruiFuncsStruct) = 0;

	virtual void			UpdateProgressBar(const LevelLoadingProgress_e progress) = 0;
	virtual void			Unknown3() = 0;
	virtual void			Unknown4() = 0;
};

#define VENGINE_VGUI_VERSION	"VEngineVGui001"

//#if defined(_STATIC_LINKED) && defined(CLIENT_DLL)
//namespace Client
//{
//	extern IEngineVGui* enginevgui;
//}
//#else
//extern IEngineVGui* enginevgui;
//#endif

#endif // IENGINEVGUI_H
