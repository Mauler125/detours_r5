#ifndef RTECH_PAKSTREAM_H
#define RTECH_PAKSTREAM_H
#include "rtech/ipakfile.h"
#include "pakstate.h"

extern void Pak_OpenAssociatedStreamingFiles(PakLoadedInfo_s* const loadedInfo, PakLoadedInfo_s::StreamingInfo_t& streamInfo,
    const uint16_t fileNamesBufSize, const PakStreamSet_e set);

extern void Pak_EnableEmbeddedStreamingData(PakLoadedInfo_s* const loadedInfo, PakLoadedInfo_s::StreamingInfo_t& streamInfo);
extern void Pak_LoadStreamingData(PakLoadedInfo_s* const loadedInfo);

// the current download progress of optional streaming assets
inline float* g_pStreamingDownloadProgress = nullptr;

// NOTE: must use these when incrementing asset counts !!!
inline void Pak_IncrementStreamingAssetCount() { ThreadInterlockedIncrement64(&g_pakGlobals->numOptStreamableAssets); }
inline void Pak_DecrementStreamingAssetCount() { ThreadInterlockedDecrement64(&g_pakGlobals->numOptStreamableAssets); }

inline int64_t Pak_GetNumStreamableAssets() { return g_pakGlobals->numOptStreamableAssets; }

inline float Pak_GetStreamingDownloadProgress() { return *g_pStreamingDownloadProgress; }
inline bool Pak_StreamingDownloadFinished()     { return Pak_GetStreamingDownloadProgress() == 1.0f; }

inline bool Pak_StreamingEnabled() { return g_pakGlobals->useStreamingSystem != NULL; }

class V_PakStream : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_streamingDownloadProgress", g_pStreamingDownloadProgress);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_GameDll.FindPatternSIMD("F3 0F 10 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC 48 89 5C 24 ?? 57 48 81 EC ?? ?? ?? ??")
			.ResolveRelativeAddress(0x4, 0x8).GetPtr(g_pStreamingDownloadProgress);
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};

#endif // RTECH_PAKSTREAM_H
