#ifndef RTECH_PAKALLOC_H
#define RTECH_PAKALLOC_H
#include "rtech/ipakfile.h"

extern void Pak_AlignSlabHeaders(PakFile_s* const pak, PakSlabDescriptor_s* const desc);
extern void Pak_AlignSlabData(PakFile_s* const pak, PakSlabDescriptor_s* const desc);
extern void Pak_CopyPagesToSlabs(PakFile_s* const pak, PakLoadedInfo_s* const loadedInfo, PakSlabDescriptor_s* const desc);

// something with sorting pages?
inline void (*sub_140442740)(PakAsset_s** assetEntries, PakAsset_s** assetEntry, __int64 idx, PakFile_s* pak);

///////////////////////////////////////////////////////////////////////////////
class V_PakAlloc : public IDetour
{
	virtual void GetAdr(void) const
	{
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 54 41 56 41 57 48 83 EC 40 48 8B C2 49 8B D9").GetPtr(sub_140442740);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};
///////////////////////////////////////////////////////////////////////////////

#endif // RTECH_PAKALLOC_H
