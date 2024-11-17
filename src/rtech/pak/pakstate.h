#ifndef RTECH_PAKSTATE_H
#define RTECH_PAKSTATE_H
#include "tier0/tslist.h"
#include "tier0/jobthread.h"
#include "launcher/launcher.h"

#include "rtech/ipakfile.h"

struct PakLoadFuncs_s
{
	void* Initialize; // Returns the pak handle of the patch master RPak once initialized.
	void* RegisterAsset;
	char unknown0[8];
	PakHandle_t(*LoadAsync)(const char* pakFileName, CAlignedMemAlloc* allocator, int nIdx, bool bUnk);
	void* LoadAsyncAndWait;
	void (*UnloadAsync)(PakHandle_t handle);
	void* UnloadAsyncAndWait;
	char unknown2[16];
	void* Func7;
	void* Func8;
	PakStatus_e(*WaitAsync)(PakHandle_t handle, void* finishCallback);
	void* Func10;
	void* Func11;
	void* FindByGUID;
	void* FindByName;
	char unknown3[8];
	void* Func14;
	void* Func15;
	void* Func16;
	void* Func17;
	void* Func18;
	void* IncrementStreamingAssetCount;
	void* DecrementStreamingAssetCount;
	void* IsFullStreamingInstall;
	char unknown4[48];
	int (*OpenAsyncFile)(const char* const fileName, int logLevel, size_t* const outFileSize);
	void (*CloseAsyncFile)(short fileHandle);
	void* Func24;
	void* Func25;
	void* ReadAsyncFile;
	void* ReadAsyncFileWithUserData;
	uint8_t(*CheckAsyncRequest)(int idx, size_t* const bytesProcessed, const char** const statusMsg);
	uint8_t(*WaitAndCheckAsyncRequest)(int idx, size_t* const bytesProcessed, const char** const statusMsg);
	void* WaitForAsyncFileRead;
	void* Func31;
	void* Func32;
	void* Func33;
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
