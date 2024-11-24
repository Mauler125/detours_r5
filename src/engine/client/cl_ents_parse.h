#ifndef CL_ENTS_PARSE_H
#define CL_ENTS_PARSE_H
#include "engine/shared/ents_shared.h"

inline bool(*v_CL_CopyExistingEntity)(CEntityReadInfo* const u, unsigned int* const iClass, bool* const pbError);

bool CL_CopyExistingEntity(CEntityReadInfo* const u, unsigned int* const iClass, bool* const pbError);
///////////////////////////////////////////////////////////////////////////////
class V_CL_Ents_Parse : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CL_CopyExistingEntity", v_CL_CopyExistingEntity);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 70 4C 63 51 28").GetPtr(v_CL_CopyExistingEntity);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const
	{
		DetourSetup(&v_CL_CopyExistingEntity, &CL_CopyExistingEntity, bAttach);
	}
};
///////////////////////////////////////////////////////////////////////////////

#endif // !CL_ENTS_PARSE_H
