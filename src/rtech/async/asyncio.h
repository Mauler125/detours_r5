#ifndef RTECH_ASYNCIO_H
#define RTECH_ASYNCIO_H
#include "rtech/iasync.h"
#include "rtech/rstdlib.h"

struct AsyncHandleTracker_s
{
	int slot;
	int state;
	HANDLE handle;
};

struct AsyncHandleStatus_s
{
	enum Status_e: uint8_t
	{
		// the file is still pending, or being read at this moment
		FS_ASYNC_PENDING = 0,

		// the file is ready to be used
		FS_ASYNC_READY,

		// there was an error while reading the file
		FS_ASYNC_ERROR,

		// async read operations were canceled
		FS_ASYNC_CANCELLED
	};

	int slot;
	int unk2;
	int unk3;
	int unk4;
	__int64 unk5;

	// pointer to user defined data
	void* userData;

	void* unkFunctionPointer;
	void* unkStatusPointer;
	int unk6;
	int unkFlag0;
	__int16 unkFlag2;
	__int16 unkFlag3;
	__int16 unk8;
	Status_e readStatus;
};
static_assert(sizeof(AsyncHandleStatus_s) == 0x40);

extern int FS_OpenAsyncFile(const char* const filePath, const int logChannel, size_t* const fileSizeOut);
extern void FS_CloseAsyncFile(const int fileHandle);

inline int(*v_FS_OpenAsyncFile)(const char* const filePath, const int logChannel, size_t* const outFileSize);
inline void(*v_FS_CloseAsyncFile)(const int fileHandle);

inline int(*v_FS_ReadAsyncFile)(const int fileHandle, __int64 readOffset, unsigned __int64 readSize, void* a4, void* a5, void* a6, int a7);
inline uint8_t(*v_FS_CheckAsyncRequest)(AsyncHandleStatus_s* pakStatus, size_t* bytesProcessed, const char** stateString);

inline AsyncHandleTracker_s* g_pAsyncFileSlots;  // bufSize=1024*sizeof(FileHandleTracker_t).
inline RHashMap_MT* g_pAsyncFileSlotMgr;         // Manages 'g_pakFileSlots'.
inline AsyncHandleStatus_s* g_pAsyncStatusSlots; // bufSize=256*sizeof(PakStatus_t).
inline RHashMap_MT* g_pAsyncStatusSlotMgr;       // Manages 'g_pakStatusSlots'.

class V_AsyncIO : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("FS_OpenAsyncFile", v_FS_OpenAsyncFile);
		LogFunAdr("FS_CloseAsyncFile", v_FS_CloseAsyncFile);

		LogFunAdr("FS_ReadAsyncFile", v_FS_ReadAsyncFile);
		LogFunAdr("FS_CheckAsyncRequest", v_FS_CheckAsyncRequest);

		LogVarAdr("g_asyncFileSlots", g_pAsyncFileSlots);
		LogVarAdr("g_asyncFileSlotMgr", g_pAsyncFileSlotMgr);

		LogVarAdr("g_asyncStatusSlots", g_pAsyncStatusSlots);
		LogVarAdr("g_asyncStatusSlotMgr", g_pAsyncStatusSlotMgr);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 89 85 08 01 ?? ??").FollowNearCallSelf().GetPtr(v_FS_OpenAsyncFile);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 8B D9 48 8D 35 ?? ?? ?? ??").GetPtr(v_FS_CloseAsyncFile);

		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 4C 89 64 24 ?? 41 55 41 56 41 57 48 83 EC 20 8B C1").GetPtr(v_FS_ReadAsyncFile);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 20 0F B6 79").GetPtr(v_FS_CheckAsyncRequest);
	}
	virtual void GetVar(void) const
	{
		const CMemory streamDbBase = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 54 41 56 41 57 48 83 EC 40 48 8B E9");

		g_pAsyncFileSlots = streamDbBase.Offset(0x70).FindPatternSelf("4C 8D", CMemory::Direction::DOWN, 512, 1).ResolveRelativeAddress(0x3, 0x7).RCast<AsyncHandleTracker_s*>();
		g_pAsyncFileSlotMgr = streamDbBase.Offset(0x70).FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddress(0x3, 0x7).RCast<RHashMap_MT*>();

		g_pAsyncStatusSlots = g_GameDll.FindPatternSIMD("0F B6 C9 48 8D 05 ?? ?? ?? ??").FindPatternSelf("48 8D 05").ResolveRelativeAddress(0x3, 0x7).RCast<AsyncHandleStatus_s*>();
		g_pAsyncStatusSlotMgr = streamDbBase.Offset(0x190).FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddress(0x3, 0x7).RCast<RHashMap_MT*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};


#endif // !ASYNCIO_H
