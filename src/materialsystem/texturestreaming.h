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

struct MaterialGlue_s;
struct TextureAsset_s;

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

enum TextureStreamMode_e : uint8
{
	TSM_OPMODE_LEGACY_PICMIP = 0,
	TSM_OPMODE_DYNAMIC,
	TSM_OPMODE_ALL,
	TSM_OPMODE_NONE,
	TSM_OPMODE_PAUSED,
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
	StreamDB_Header_s header;
	StreamDB_ResidentPage_s* residentPages;
	MaterialGlue_s** materials;
	StreamDB_Material_s* materialInfo;
	int64 maxResidentPageSize;
	StreamDB_PageState_s pageStates[4];
	bool unk_320;
	char gap_321[3];
	TextureStreamMode_e texStreamMode;
	int picMip;
	float streamBspBucketBias;
	float streamBspDistScale;
	uint64 highPriorityMemoryBudget;
	uint32 streamBspCellX;
	uint32 streamBspCellY;
	int loadedLinkedTextureCount;
	int totalMipLevelCount;
	int loadedMipLevelCount;
	int unk_34;
	int64 usedStreamingMemory;
	int64 totalStreamingMemory;
	int thisFrame;
	int unk_50;
	Vector3D streamBspCameraPos;
	float streamBspHalfFovX;
	float streamBspViewWidth;
	TextureAsset_s* streamableTextures[4];
};

enum TextureStreamMemory_e
{
	TML_TRACKER_UNFREE,

	TML_TRACKER_UNKNOWN_1, // Appears unused by the retail runtime.
	TML_TRACKER_UNKNOWN_2, // Appears unused by the retail runtime.

	TML_TRACKER_UNUSABE,

	// Not a memory tracker!
	STREAMING_TEXTURES_MEMORY_LATENCY_FRAME_COUNT,
};

inline void(*v_StreamDB_Init)(const char* const pszLevelName);

inline void(*TextureStreamMgr_GetStreamOverlay)(const char* const mode, char* const buf, const size_t bufSize);
inline const char* (*TextureStreamMgr_DrawStreamOverlayToInterface)(void* thisptr, uint8_t* a2, void* unused, void* debugOverlayIface);

inline ssize_t* g_textureStreamMemoryUsed = nullptr; // array size = STREAMING_TEXTURES_MEMORY_LATENCY_FRAME_COUNT.
inline ssize_t* g_textureStreamMemoryTarget = nullptr; // pointer to single size var.

inline TextureStreamMgr_s* s_textureStreamMgr;

///////////////////////////////////////////////////////////////////////////////
class VTextureStreaming : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("StreamDB_Init", v_StreamDB_Init);

		LogFunAdr("TextureStreamMgr_GetStreamOverlay", TextureStreamMgr_GetStreamOverlay);
		LogFunAdr("TextureStreamMgr_DrawStreamOverlayToInterface", TextureStreamMgr_DrawStreamOverlayToInterface);

		LogVarAdr("g_textureStreamMemoryUsed", g_textureStreamMemoryUsed);
		LogVarAdr("g_textureStreamMemoryTarget", g_textureStreamMemoryTarget);

		LogVarAdr("s_textureStreamMgr", s_textureStreamMgr);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9").GetPtr(v_StreamDB_Init);

		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 80 7C 24 ?? ?? 0F 84 ?? ?? ?? ?? 48 89 9C 24 ?? ?? ?? ??").FollowNearCallSelf().GetPtr(TextureStreamMgr_GetStreamOverlay);
		g_GameDll.FindPatternSIMD("41 56 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 C6 02 ??").GetPtr(TextureStreamMgr_DrawStreamOverlayToInterface);
	}
	virtual void GetVar(void) const
	{
		CMemory(TextureStreamMgr_DrawStreamOverlayToInterface).Offset(0x2D).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr(g_textureStreamMemoryUsed);
		CMemory(TextureStreamMgr_DrawStreamOverlayToInterface).Offset(0x1C).FindPatternSelf("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr(g_textureStreamMemoryTarget);

		CMemory(v_StreamDB_Init).FindPattern("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).GetPtr(s_textureStreamMgr);
	}
	virtual void GetCon(void) const
	{ }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // TEXTURESTREAMING_H
