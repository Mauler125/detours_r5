#pragma once
#include "tier1/cmd.h"
#include "sys_dll.h"

//-------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------
class CClient;

struct EngineParms_t
{
	char* baseDirectory;
	char* modName;
	char* rootGameName;
	unsigned int memSizeAvailable;
};

extern EngineParms_t* g_pEngineParms;

/* ==== HOST ============================================================================================================================================================ */
inline void(*v_Host_Init)();
#ifndef DEDICATED
inline void(*v_Host_Init_DuringVideo)(bool* bDedicated);
inline void(*v_Host_Init_PostVideo)(bool* bDedicated);
inline void(*v_Host_SetupUIMaterials)();
#endif // !DEDICATED
inline void(*v_Host_Shutdown)();
inline bool(*v_Host_NewGame)(char* pszMapName, char* pszMapGroup, bool bLoadGame, char bBackground, LARGE_INTEGER PerformanceCount);
inline void(*v_Host_Disconnect)(bool bShowMainMenu);
inline bool(*v_Host_ChangeLevel)(bool bLoadFromSavedGame, const char* pszMapName, const char* pszMapGroup);
inline void (*v_Host_Status_PrintClient)(CClient* client, bool bShowAddress, void (*print) (const char* fmt, ...));

inline int(*v_SetLaunchOptions)(const CCommand& args);
inline bool(*v_DFS_InitializeFeatureFlagDefinitions)(const char* pszFeatureFlags);

extern EngineParms_t* g_pEngineParms;

///////////////////////////////////////////////////////////////////////////////
class VHostCmd : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Host_Init", v_Host_Init);
#ifndef DEDICATED
		LogFunAdr("Host_Init_DuringVideo", v_Host_Init_DuringVideo);
		LogFunAdr("Host_Init_PostVideo", v_Host_Init_PostVideo);
		LogFunAdr("Host_SetupUIMaterials", v_Host_SetupUIMaterials);
#endif // !DEDICATED
		LogFunAdr("Host_Shutdown", v_Host_Shutdown);
		LogFunAdr("Host_Disconnect", v_Host_Disconnect);
		LogFunAdr("Host_NewGame", v_Host_NewGame);
		LogFunAdr("Host_ChangeLevel", v_Host_ChangeLevel);
		LogFunAdr("Host_Status_PrintClient", v_Host_Status_PrintClient);
		LogFunAdr("SetLaunchOptions", v_SetLaunchOptions);

		LogFunAdr("DFS_InitializeFeatureFlagDefinitions", v_DFS_InitializeFeatureFlagDefinitions);
		LogVarAdr("g_pEngineParms", g_pEngineParms);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("88 4C 24 08 53 55 56 57 48 83 EC 68").GetPtr(v_Host_Init);
#ifndef DEDICATED
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B D9").GetPtr(v_Host_Init_DuringVideo);
		g_GameDll.FindPatternSIMD("48 8B C4 41 56 48 81 EC ?? ?? ?? ?? 45 33 F6").GetPtr(v_Host_Init_PostVideo);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 8D 3D").GetPtr(v_Host_SetupUIMaterials);
#endif // !DEDICATED
		g_GameDll.FindPatternSIMD("48 8B C4 ?? 41 54 41 55 48 81 EC 70 04 ?? ?? F2 0F 10 05 ?? ?? ?? 0B").GetPtr(v_Host_NewGame);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 0F B6 D9").GetPtr(v_Host_Disconnect);
		g_GameDll.FindPatternSIMD("40 56 57 41 56 48 81 EC ?? ?? ?? ??").GetPtr(v_Host_ChangeLevel);
		g_GameDll.FindPatternSIMD("48 8B C4 48 83 EC ?? 80 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 8B 15 ?? ?? ?? ??").GetPtr(v_Host_Shutdown);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 60 48 8B A9 ?? ?? ?? ??").GetPtr(v_Host_Status_PrintClient);

		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 83 EC 20 48 8B 1D ?? ?? ?? ?? 48 8B E9 48 85 DB").GetPtr(v_SetLaunchOptions);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 40 38 3D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B CE").FollowNearCallSelf().GetPtr(v_DFS_InitializeFeatureFlagDefinitions);
	}
	virtual void GetVar(void) const
	{
		g_pEngineParms = CMemory(CModAppSystemGroup__Main).FindPattern("48 8B", CMemory::Direction::DOWN, 100).ResolveRelativeAddress(0x3, 0x7).RCast<EngineParms_t*>();
		g_pEngineParms = CMemory(CModAppSystemGroup__Main).FindPattern("4C 8B", CMemory::Direction::DOWN, 100).ResolveRelativeAddress(0x3, 0x7).RCast<EngineParms_t*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
