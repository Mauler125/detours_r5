#ifndef TEXTURESTREAMING_H
#define TEXTURESTREAMING_H
#include "public/rtech/istreamdb.h"

inline void(*v_StreamDB_Init)(const char* const pszLevelName);

inline StreamDB_s* s_streamDataBase;

///////////////////////////////////////////////////////////////////////////////
class VTextureStreaming : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("StreamDB_Init", v_StreamDB_Init);
		LogVarAdr("s_streamDataBase", s_streamDataBase);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9").GetPtr(v_StreamDB_Init);
	}
	virtual void GetVar(void) const
	{
		CMemory(v_StreamDB_Init).FindPattern("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).GetPtr(s_streamDataBase);
	}
	virtual void GetCon(void) const
	{ }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // TEXTURESTREAMING_H
