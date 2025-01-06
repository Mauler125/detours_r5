//=============================================================================//
// 
// Purpose: texture streaming and runtime management
// 
//-----------------------------------------------------------------------------
// Some of these structs are based on the presentation held by the developer of
// the texture streaming system in Titanfall 2 and Apex Legends, see the links:
// - https://www.gdcvault.com/play/1024418/Efficient-Texture-Streaming-in-Titanfall
// - https://www.youtube.com/watch?v=q0aKNGH8WbA
//=============================================================================//

#ifndef TEXTURESTREAMING_H
#define TEXTURESTREAMING_H
#include "public/rtech/istreamdb.h"

struct TextureStreamMgr_Task_s
{
	TextureAsset_s* textureAsset;

	// The mip level count to load or drop.
	uint8 mipLevelCount;
	char padding[3];

	// The 'cost vs benefit' metric used to partially sort the task list to get
	// the best and worst 16 textures.
	float metric;
};

struct TextureStreamMgr_TaskList_s
{
	// STBSP async file handle and index to the current page.
	int fileHandle;
	int pageIndex;

	// Whether we should update the current page state.
	bool updatePageState;
	int padding;

	// Offset to the page in the STBSP to read up to size bytes.
	uint64 pageOffset;
	uint64 pageSize;

	// - loadBegin points to the first texture load task.
	// - loadEnd points to the last texture load task.
	// - loadLimit points to the absolute end of the load task buffer.
	TextureStreamMgr_Task_s* loadBegin;
	TextureStreamMgr_Task_s* loadEnd;
	TextureStreamMgr_Task_s* loadLimit;

	// - dropBegin points to the first texture drop task.
	// - dropEnd points to the last texture drop task.
	// - dropLimit points to the absolute end of the drop task buffer.
	TextureStreamMgr_Task_s* dropBegin;
	TextureStreamMgr_Task_s* dropEnd;
	TextureStreamMgr_Task_s* dropLimit;
};

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
	TextureAsset_s* streamableTextures[4];
};

inline void(*v_StreamDB_Init)(const char* const pszLevelName);

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
