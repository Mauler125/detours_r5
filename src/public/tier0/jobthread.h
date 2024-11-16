#ifndef JOBTHREAD_H
#define JOBTHREAD_H

typedef uint32_t JobID_t;
typedef uint8_t JobTypeID_t;
typedef uint32_t JobAffinity_t;

typedef bool(*JobHelpCallback_t)(__int64, _DWORD*, __int64, _QWORD*);

enum JobPriority_e : uint32_t
{
};

struct JobFifoLock_s
{
	int id;
	int depth;
	short tls[64];
};

struct JobContext_s
{
	void* callbackArg;  // Argument to job callback function.
	void* callbackFunc; // Job callback function.
	JobTypeID_t jobTypeId;
	bool field_11;
	__int16 field_12;
	int field_14;
	JobID_t jobId;
	int field_1C;
	int field_20;
	int field_24; // Bit fields?
	__int64 field_28;
	__int64 unknownMask;
	__int64 unknownInt;
};

typedef struct JobUserData_s
{
	JobUserData_s(int32_t si)
	{
		data.sint = si;
	}
	JobUserData_s(uint32_t ui)
	{
		data.uint = ui;
	}
	JobUserData_s(int64_t si)
	{
		data.sint = si;
	}
	JobUserData_s(uint64_t ui)
	{
		data.uint = ui;
	}
	JobUserData_s(double sa)
	{
		data.scal = sa;
	}
	JobUserData_s(void* pt)
	{
		data.ptr = pt;
	}

	union
	{
		int64_t sint;
		uint64_t uint;
		double scal;
		void* ptr;
	} data;
} JobUserData_t;

// Array size = 2048*sizeof(JobContext_s)
inline JobContext_s* job_JT_Context = nullptr;

extern bool JT_IsJobDone(const JobID_t jobId);
extern JobID_t JTGuts_AddJob(JobTypeID_t jobTypeId, JobID_t jobId, void* callbackFunc, void* callbackArg);
extern JobID_t JT_GetCurrentJob();


inline void(*JT_ParallelCall)(void);
inline void*(*JT_HelpWithAnything)(bool bShouldLoadPak);

inline bool(*JT_HelpWithJobTypes)(JobHelpCallback_t, JobUserData_t userData, __int64 a3, __int64 a4);
inline __int64(*JT_HelpWithJobTypesOrSleep)(JobHelpCallback_t, JobUserData_t userData, __int64 a3, __int64 a4, volatile signed __int64* a5, char a6);
inline __int64(*JT_WaitForJobAndOnlyHelpWithJobTypes)(JobID_t, uint64_t unkMask1, uint64_t unkMask2);

inline bool(*JT_AcquireFifoLockOrHelp)(struct JobFifoLock_s* pFifo);
inline void(*JT_ReleaseFifoLock)(struct JobFifoLock_s* pFifo);

inline void(*JT_EndJobGroup)(const JobID_t jobId);

inline unsigned int (*JT_AllocateJob)(); // Returns an index to the 'job_JT_Context' array
inline JobID_t(*JTGuts_AddJob_Internal)(JobTypeID_t jobTypeId, JobID_t jobId, void* callbackfunc, void* callbackArg, int jobIndex, JobContext_s* context);

///////////////////////////////////////////////////////////////////////////////
class VJobThread : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("JT_ParallelCall", JT_ParallelCall);

		LogFunAdr("JT_HelpWithAnything", JT_HelpWithAnything);
		LogFunAdr("JT_HelpWithJobTypes", JT_HelpWithJobTypes);
		LogFunAdr("JT_HelpWithJobTypesOrSleep", JT_HelpWithJobTypesOrSleep);
		LogFunAdr("JT_WaitForJobAndOnlyHelpWithJobTypes", JT_WaitForJobAndOnlyHelpWithJobTypes);

		LogFunAdr("JT_AcquireFifoLockOrHelp", JT_AcquireFifoLockOrHelp);
		LogFunAdr("JT_ReleaseFifoLock", JT_ReleaseFifoLock);

		LogFunAdr("JT_EndJobGroup", JT_EndJobGroup);
		LogFunAdr("JT_AllocateJob", JT_AllocateJob);
		LogFunAdr("JTGuts_AddJob_Internal", JTGuts_AddJob_Internal);
		LogVarAdr("job_JT_Context", job_JT_Context);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 48 89 58 08 48 89 78 10 55 48 8D 68 A1 48 81 EC ?? ?? ?? ?? 0F 29 70 E8 48 8D 1D ?? ?? ?? ??").GetPtr(JT_ParallelCall);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 80 3D ?? ?? ?? ?? ??").GetPtr(JT_HelpWithAnything);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 4C 89 4C 24 ?? 4C 89 44 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 60").GetPtr(JT_HelpWithJobTypes);
		g_GameDll.FindPatternSIMD("4C 89 4C 24 ?? 4C 89 44 24 ?? 48 89 54 24 ?? 48 89 4C 24 ?? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ??").GetPtr(JT_HelpWithJobTypesOrSleep);
		g_GameDll.FindPatternSIMD("48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 30 8B F9").GetPtr(JT_WaitForJobAndOnlyHelpWithJobTypes);
		g_GameDll.FindPatternSIMD("48 83 EC 08 65 48 8B 04 25 ?? ?? ?? ?? 4C 8B C1").GetPtr(JT_AcquireFifoLockOrHelp);
		g_GameDll.FindPatternSIMD("48 83 EC 28 44 8B 11").GetPtr(JT_ReleaseFifoLock);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 30 65 48 8B 04 25 ?? ?? ?? ?? BA ?? ?? ?? ??").GetPtr(JT_AllocateJob);
		g_GameDll.FindPatternSIMD("8B D1 48 8D 05 ?? ?? ?? ?? 81 E2 ?? ?? ?? ?? 48 C1 E2 06 48 03 D0 E9 ?? ?? ?? ??").GetPtr(JT_EndJobGroup);
		g_GameDll.FindPatternSIMD("48 89 74 24 ? 57 48 83 EC 20 0F B6 F1").GetPtr(JTGuts_AddJob_Internal);
	}
	virtual void GetVar(void) const
	{
		CMemory(JT_EndJobGroup).FindPattern("48 8D").ResolveRelativeAddressSelf(3, 7).GetPtr(job_JT_Context);
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // JOBTHREAD_H
