#ifndef RTECH_PAKSTATE_H
#define RTECH_PAKSTATE_H
#include "tier0/tslist.h"
#include "tier0/jobthread.h"
#include "launcher/launcher.h"

#include "rtech/async/asyncio.h"
#include "rtech/ipakfile.h"

struct PakLoadFuncs_s
{
	// initializes the global states for RPak files and async reads.
	// returns the pak handle of the patch master RPak once initialized.
	// currently the system only knows 2 modes; 0 = asset streaming
	// disabled, 1 = asset streaming enabled
	PakHandle_t (*Initialize)(const int mode);

	// register a specific asset with an unique fourcc identifier. the
	// code will run the loadHandler when it encounters this asset in
	// the pak file. when the asset is unloaded, the unload handler
	// for this asset will be called. when a pak has been loaded that
	// happens to contain an asset that was loaded in a prior pakfile,
	// the patch handler for this asset type will be called which
	// allows for patching the asset with the one from the new pak.
	// - the class size should contain both the header size and the
	//   rest of the class itself
	void (*const RegisterAsset)(const uint32 assetType, const uint32 assetVersion, const char* const assetTypeName,
						  const void* const loadHandler, const void* const unloadHandler, const void* const patchHandler,
						  CAlignedMemAlloc* const allocator, const uint32 headerSize, const uint32 classSize,
						  const uint32 alignment, const JobPriority_e jobPriority, const JobAffinity_t jobAffinity);

	char unused0[8];

	// load a pakfile asynchronously, returns the pakid on success,
	// PAK_INVALID_HANDLE on failure
	PakHandle_t(*const LoadAsync)(const char* const fileName, CAlignedMemAlloc* const allocator,
		const int logChannel, bool bUnk);

	// this method does the same as LoadAsync, however it will lock and wait 
	// until the requested pak has been loaded. during the lock and wait, the
	// function will help with other pending or active load/unload jobs
	PakHandle_t(*const LoadAsyncAndWait)(const char* const fileName, CAlignedMemAlloc* const allocator,
		const int logChannel, void(*const finishCallback));

	// unload a specific pakfile asynchronously by pakid
	void (*UnloadAsync)(const PakHandle_t handle);

	// this method does the same as UnloadAsync, however it will lock and wait
	// until the requested pak has been unloaded. during the lock and wait, the
	// function will help with other pending or active load/unload jobs
	void (*UnloadAsyncAndWait)(const PakHandle_t handle);

	char unused1[16];

	// set the current thread sync function and returns the previously installed
	// thread sync function
	void* (*SetThreadSyncFunc)(void(* const callback));

	// help with other allocated load/unload jobs
	void (*HelpWithPendingRequests)();

	// lock and wait for a specific pakfile to load by pakid, and help with other
	// allocated jobs in the mean time. returns false if status is PAK_STATUS_ERROR
	bool (*WaitForAsyncLoad)(const PakHandle_t handle, void(* const finishCallback));

	// lock and wait for a specific pakfile to unload by pakid, and help with other
	// allocated jobs in the mean time
	void (*WaitForAsyncUnload)(const PakHandle_t handle);

	// get an exported function of a dynamic library attached to this pakfile.
	// i.e. we have ui.rpak which has the flag PAK_HEADER_FLAGS_HAS_MODULE set,
	// and has an associated ui.dll module loaded, from which we want to get
	// the address of the export "rui_woman_face_02"
	void* (*GetModuleFunction)(const PakHandle_t handle, const char* const procName);

	// get the asset data pointer by asset guid
	void* (*FindAssetByGUID)(const PakGuid_t guid);

	// get the asset data pointer by asset name
	void* (*FindAssetByName)(const char* const name);

	char unused2[8];

	// get the asset guid by asset data pointer
	PakGuid_t (*FindGUIDByAsset)(const void* const asset);

	void* Func15; // Unknown
	void* Func16; // Unknown

	// open either the mandatory or optional embedded streaming file
	int (*OpenEmbeddedStreamingFile)(const PakHandle_t handle, const PakStreamSet_e set);

	// checks if either the mandatory or optional streaming system is disabled
	// for pakfile by streaming set; a pakfile could have it enabled for
	// the mandatory streaming set while having it disabled for the optional set
	bool (*IsStreamingDisabledForSet)(const PakHandle_t handle, const PakStreamSet_e set);

	// atomically increment/decrement total streaming asset count shared across
	// all loaded pakfiles
	void (*IncrementStreamingAssetCount);
	void (*DecrementStreamingAssetCount);

	// returns whether the runtime has optional streaming assets loaded
	bool (*HasOptionalStreamingAssetsLoaded)();

	char unused3[48];

	// open a specific file for asynchronous read, and return the file handle
	int (*OpenAsyncFile)(const char* const fileName, const int logChannel, size_t* const outFileSize);

	// close an opened file by its handle
	void (*CloseAsyncFile)(const int fileHandle);

	// atomically increments AsyncFileHandleTracker_s::state; some refcounting? see [r5apex.exe+438831h]
	// for atomic exchange before unload. needs more reversing work
	void* Func24;

	// always set to nullptr
	void* Func25;

	// asynchronously read an opened file by its handle, starting from specified
	// offset until specified read size. the read data will be written into 
	// the provided read buffer. asynchronous reads are carved into chunks of 
	// PAK_READ_DATA_CHUNK_SIZE, to allow the system to cancel out at any point
	// to create resources for more important tasks, e.g. we are trying to read
	// a texture, but there is also a request for loading an audio sample from
	// the banks, and audio has higher priority so cancel out
	int (*ReadAsyncFile)(const int fileHandle, const size_t readOffset, const size_t readSize, void* const readBuffer, const int unk7);

	// same as ReadAsyncFile, but takes 2 extra pointers, but the usage thereof has
	// not been located. debug only?
	int (*ReadAsyncFileWithUserData)(const int fileHandle, const size_t readOffset, const size_t readSize, const void* const readBuffer,
		const void* const unk5, const void* const unk6, const int unk7);

	// poll for the current async read status, see AsyncHandleStatus_s::Status_e
	// for possible return statuses
	AsyncHandleStatus_s::Status_e(*CheckAsyncRequest)(const int fileSlot, size_t* const bytesProcessed, const char** const statusMsg);

	// same as CheckAsyncRequest, but locks and waits until the read has been
	// finished. unlike other locking functions, this function does not help
	// with other allocated jobs
	AsyncHandleStatus_s::Status_e(*WaitAndCheckAsyncRequest)(const int fileSlot, size_t* const bytesProcessed, const char** const statusMsg);

	// lock and waits until the read has been finished. unlike other locking
	// functions, this function does not help with other allocated jobs
	void (*WaitForAsyncRequest)(const int fileSlot);

	// cancel an async read request and release the handle
	void (*CancelAsyncRequestAndRelease)(const int fileSlot);

	// cancel an async pak read request and release the handle
	void (*CancelLoadRequest)(const int fileSlot);

	// get the main async io worked thread
	void (*GetAsyncIOWorkerThread)(HANDLE* const outThreadHandle);
};

inline PakGlobalState_s* g_pakGlobals;
extern PakLoadFuncs_s* g_pakLoadApi;

inline JobHelpCallback_t g_pPakFifoLockWrapper; // Pointer to functor that takes the global pak fifolock as argument.

// if this is set, JT_ReleaseFifoLock has to be called
// twice as the depth goes up to the thread that
// acquired the lock + the main thread
inline bool* g_bPakFifoLockAcquiredInMainThread;

///////////////////////////////////////////////////////////////////////////////
class V_PakState : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pakGlobals", g_pakGlobals);
		LogVarAdr("g_pakLoadApi", g_pakLoadApi);

		LogVarAdr("g_pakFifoLockWrapper", g_pPakFifoLockWrapper);
		LogVarAdr("g_bPakFifoLockAcquiredInMainThread", g_bPakFifoLockAcquiredInMainThread);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pakGlobals = g_GameDll.FindPatternSIMD("48 8D 1D ?? ?? ?? ?? 45 8D 5A 0E").ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakGlobalState_s*>();
		g_pakLoadApi = CMemory(v_LauncherMain).Offset(0x820).FindPatternSelf("48 89").ResolveRelativeAddressSelf(0x3, 0x7).RCast<PakLoadFuncs_s*>();

		const CMemory jtBase(JT_HelpWithAnything);

		g_pPakFifoLockWrapper = jtBase.Offset(0x1BC).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<JobHelpCallback_t>();
		g_bPakFifoLockAcquiredInMainThread = jtBase.Offset(0x50).FindPatternSelf("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};
///////////////////////////////////////////////////////////////////////////////


#endif // RTECH_PAKSTATE_H
