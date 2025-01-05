#ifndef TEXTURESTREAMING_H
#define TEXTURESTREAMING_H
#include "public/rtech/istreamdb.h"

inline void(*v_StreamDB_Init)(const char* const pszLevelName);

struct TextureStreamMgr_s
{
	bool initialised;
	bool hasResidentPages;
	char filePath[260]; // size=MAX_PATH.
	char gap_105[2];
	int fileHandle; // STBSP file handle.
	char gap_10b[4];
	char* stringBuffer;
	StreamDBHeader_s header;
	ST_ResidentPage* residentPages;
	MaterialGlue_t** materials;
	ST_Material* materialInfo;
	int64 maxResidentPageSize;
	StreamingDBPageState_s pageStates[4];
	int picMip;
	float streamBspBucketBias;
	float streamBspDistScale;
	__int64 unk_338;
	uint32 streamBspCellX;
	uint32 streamBspCellY;
	int loadedLinkedTextureCount;
	int totalMipLevelCount;
	int loadedMipLevelCount;
	int unk_34;
	int64 usedStreamingMemory;
	int64 totalStreamingMemory;
	int unk_48;
	int unk_50;
	Vector3D streamBspCameraPos;
	float streamBspHalfFovX;
	float streamBspViewWidth;
	TextureAsset_t* streamableTextures[4];
};

inline TextureStreamMgr_s* s_textureStreamMgr;

///////////////////////////////////////////////////////////////////////////////
class VTextureStreaming : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("StreamDB_Init", v_StreamDB_Init);
		LogVarAdr("s_textureStreamMgr", s_textureStreamMgr);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9").GetPtr(v_StreamDB_Init);
	}
	virtual void GetVar(void) const
	{
		CMemory(v_StreamDB_Init).FindPattern("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).GetPtr(s_textureStreamMgr);
	}
	virtual void GetCon(void) const
	{ }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // TEXTURESTREAMING_H
