#pragma once
#include "public/vgui/ienginevgui.h"
#include <engine/server/sv_main.h>
#include <vguimatsurface/MatSystemSurface.h>
#include "inputsystem/iinputsystem.h"

abstract_class IEngineVGuiInternal : public IEngineVGui
{
public:
	virtual int Init() = 0;
	virtual void Connect() = 0;
	virtual void Shutdown() = 0;
	virtual bool SetVGUIDirectories() = 0;
	virtual bool IsInitialized() const = 0;
	virtual CreateInterfaceFn GetGameUIFactory() = 0;

	virtual bool Key_Event(const InputEvent_t& event) = 0;
	virtual bool StartFixed_Event(const InputEvent_t& event) = 0;
	virtual bool StartMapped_Event(const InputEvent_t& event) = 0;

	virtual void			Unknown5() = 0;
	virtual void			Unknown6() = 0;

	virtual void Paint(const PaintMode_t mode) = 0;

	virtual void			Unknown7() = 0; // NullSub
	virtual void			Unknown8() = 0; // NullSub
	virtual void			Unknown9() = 0; // NullSub

	// level loading
	virtual void OnLevelLoadingStarted(const char* const levelName) = 0;
	virtual void OnLevelLoadingFinished() = 0;

	virtual void EnabledProgressBarForNextLoad() = 0;

	virtual void			Unknown10() = 0; // something with load movies?
	virtual void			Unknown11() = 0;
	virtual void			Unknown12() = 0;

	virtual void ShowErrorMessage() = 0;
	virtual void HideLoadingPlaque() = 0;
	virtual bool ShouldPause() = 0;

	virtual void			Unknown13() = 0;
	virtual void			Unknown14() = 0;
	virtual void			Unknown15() = 0;
	virtual void			Unknown16() = 0;
	virtual void			Unknown17() = 0;
	virtual void			Unknown18() = 0;
	virtual void			Unknown19() = 0;
	virtual void			Unknown20() = 0;
	virtual void			Unknown21() = 0;
	virtual void			Unknown22() = 0;
	virtual void			Unknown23() = 0;

	virtual void SetNotAllowedToHideGameUI(const bool bNotAllowedToHide) = 0;
	virtual void SetNotAllowedToShowGameUI(const bool bNotAllowedToShow) = 0;

	virtual void			Unknown24() = 0;
	virtual void			Unknown25() = 0;
	virtual void			Unknown26() = 0;
	virtual void			Unknown27() = 0;
	virtual void			Unknown28() = 0;
	virtual void			Unknown29() = 0;
	virtual void			Unknown30() = 0;
	virtual void			Unknown31() = 0; // NullSub
};

class CEngineVGui : public IEngineVGuiInternal
{
public:
	static int VPaint(CEngineVGui* const thisptr, const PaintMode_t mode);
};

/* ==== CENGINEVGUI ===================================================================================================================================================== */
inline int(*CEngineVGui__Paint)(CEngineVGui* const thisptr, const PaintMode_t mode);
inline void*(*CEngineVGui__RenderStart)(CMatSystemSurface* pMatSystemSurface);
inline void*(*CEngineVGui__RenderEnd)(void);

inline InputEventCallback_t v_UIEventDispatcher = nullptr; // Points to 'CGame::DispatchInputEvent()'
inline CEngineVGui* g_pEngineVGui = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VEngineVGui : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CEngineVGui::Paint", CEngineVGui__Paint);
		LogFunAdr("CEngineVGui::RenderStart", CEngineVGui__RenderStart);
		LogFunAdr("CEngineVGui::RenderEnd", CEngineVGui__RenderEnd);
		LogFunAdr("UIEventDispatcher", v_UIEventDispatcher);
		LogVarAdr("g_pEngineVGui", g_pEngineVGui);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("41 55 41 56 48 83 EC 78 44 8B EA").GetPtr(CEngineVGui__Paint);
		g_GameDll.FindPatternSIMD("40 53 57 48 81 EC ?? ?? ?? ?? 48 8B F9").GetPtr(CEngineVGui__RenderStart);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B 0D ?? ?? ?? ?? C6 05 ?? ?? ?? ?? ?? 48 8B 01").GetPtr(CEngineVGui__RenderEnd);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 40 48 63 01").GetPtr(v_UIEventDispatcher);
	}
	virtual void GetVar(void) const
	{
		g_pEngineVGui = g_GameDll.FindPatternSIMD("48 8B C4 48 89 48 08 48 89 50 10 4C 89 40 18 4C 89 48 20 53 57 48 81 EC ?? ?? ?? ?? 48 8B D9 48 8D 78 10 E8 ?? ?? ?? ?? 48 89 7C 24 ?? 48 8D 54 24 ?? 33 FF 4C 8B CB 41 B8 ?? ?? ?? ?? 48 89 7C 24 ?? 48 8B 08 48 83 C9 01 E8 ?? ?? ?? ?? 85 C0 48 8D 54 24 ??")
			.FindPatternSelf("48 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngineVGui*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
