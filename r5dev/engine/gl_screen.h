#pragma once

///////////////////////////////////////////////////////////////////////////////
inline CMemory SCR_BeginLoadingPlaque;

///////////////////////////////////////////////////////////////////////////////
inline bool* scr_drawloading = nullptr;
inline bool* scr_engineevent_loadingstarted = nullptr;

void SCR_EndLoadingPlaque(void);

///////////////////////////////////////////////////////////////////////////////
class VGL_Screen : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: SCR_BeginLoadingPlaque               : {:#18x} |\n", SCR_BeginLoadingPlaque.GetPtr());
		spdlog::debug("| VAR: scr_drawloading                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(scr_drawloading));
		spdlog::debug("| VAR: scr_engineevent_loadingstarted       : {:#18x} |\n", reinterpret_cast<uintptr_t>(scr_engineevent_loadingstarted));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		SCR_BeginLoadingPlaque = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x0F\x29\x74\x24\x00\x48\x8B\xF9"), "xxxx?xxxx?xxxxxxxxx?xxx");
		// 0x14022A4A0 // 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 0F 29 74 24 ? 48 8B F9 //
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		SCR_BeginLoadingPlaque = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x38\x0F\x29\x74\x24\x00\x48\x89\x5C\x24\x00"), "xxxxxxxx?xxxx?");
		// 0x14022A4A0 // 48 83 EC 38 0F 29 74 24 ? 48 89 5C 24 ? //
#endif
	}
	virtual void GetVar(void) const
	{
		scr_drawloading = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x0F\xB6\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x83\xEC\x28"), "xxx????xxxxxxxxxxxxx")
			.ResolveRelativeAddressSelf(0x3, 0x7).RCast<bool*>();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		scr_engineevent_loadingstarted = SCR_BeginLoadingPlaque.Offset(0x130).FindPatternSelf("C6 05 ?? ?? ?? ?? 01", CMemory::Direction::DOWN).ResolveRelativeAddress(0x2, 0x7).RCast<bool*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		scr_engineevent_loadingstarted = SCR_BeginLoadingPlaque.Offset(0x60).FindPatternSelf("C6 05 ?? ?? ?? ?? 01", CMemory::Direction::DOWN).ResolveRelativeAddress(0x2, 0x7).RCast<bool*>();
#endif

	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VGL_Screen);
