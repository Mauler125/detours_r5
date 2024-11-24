#ifndef CL_ENTS_PARSE_H
#define CL_ENTS_PARSE_H
#include "engine/shared/ents_shared.h"

inline bool(*v_CL_CopyNewEntity)(CEntityReadInfo* const u, unsigned int* const iClass, const int iSerialNum, bool* const pbError);
inline bool(*v_CL_CopyExistingEntity)(CEntityReadInfo* const u, unsigned int* const iClass, bool* const pbError);

bool CL_CopyNewEntity(CEntityReadInfo* const u, unsigned int* const iClass, const int iSerialNum, bool* const pbError);
bool CL_CopyExistingEntity(CEntityReadInfo* const u, unsigned int* const iClass, bool* const pbError);
///////////////////////////////////////////////////////////////////////////////
class V_CL_Ents_Parse : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CL_CopyNewEntity", v_CL_CopyExistingEntity);
		LogFunAdr("CL_CopyExistingEntity", v_CL_CopyExistingEntity);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 55 53 41 54 41 55 41 57 48 8D AC 24").GetPtr(v_CL_CopyNewEntity);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 70 4C 63 51 28").GetPtr(v_CL_CopyExistingEntity);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const
	{
		DetourSetup(&v_CL_CopyNewEntity, &CL_CopyNewEntity, bAttach);
		DetourSetup(&v_CL_CopyExistingEntity, &CL_CopyExistingEntity, bAttach);
	}
};
///////////////////////////////////////////////////////////////////////////////

#endif // !CL_ENTS_PARSE_H
