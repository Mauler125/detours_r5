#pragma once
#include "tier1/IConVar.h"

/* ==== CFPSPANEL ======================================================================================================================================================= */
inline CMemory p_CFPSPanel_Paint;
inline auto CFPSPanel_Paint = p_CFPSPanel_Paint.RCast<ConVar* (*)(void* thisptr)>();

void CFPSPanel_Attach();
void CFPSPanel_Detach();

///////////////////////////////////////////////////////////////////////////////
class VFPSPanel : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CFPSPanel::Paint                     : {:#18x} |\n", p_CFPSPanel_Paint.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CFPSPanel_Paint = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x55\x56\x41\x00\x48\x8D\xA8\x00\xFD\xFF\xFF\x48\x81\xEC\x80"), "xxxxxx?xxx?xxxxxxx");
		CFPSPanel_Paint = p_CFPSPanel_Paint.RCast<ConVar* (*)(void*)>(); /*48 8B C4 55 56 41 ?? 48 8D A8 ?? FD FF FF 48 81 EC 80*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VFPSPanel);
