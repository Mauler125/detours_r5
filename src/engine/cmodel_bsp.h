#pragma once
#include "tier0/jobthread.h"
#include "rtech/ipakfile.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class KeyValues;

//-----------------------------------------------------------------------------
// this structure contains handles and names to the base pak files the engine
// loads for a level, this is used for load/unload management during level
// changes or engine shutdown
//-----------------------------------------------------------------------------
struct CommonPakData_t
{
	enum EPakType
	{
		// the UI pak assigned to the current gamemode (range in GameMode_t)
		PAK_TYPE_UI_GM = 0,
		PAK_TYPE_COMMON,

		// the base pak assigned to the current gamemode (range in GameMode_t)
		PAK_TYPE_COMMON_GM,
		PAK_TYPE_LOBBY,

		// NOTE: this one is assigned to the name of the level, the prior ones are
		// all static!
		PAK_TYPE_LEVEL,

		// the total number of pak files to watch and manage
		PAK_TYPE_COUNT
	};

	CommonPakData_t()
	{
		Reset();
	}

	void Reset()
	{
		pakId = PAK_INVALID_HANDLE;
		keepLoaded = false;
		basePakName = nullptr;

		memset(pakName, '\0', sizeof(pakName));
	}

	PakHandle_t pakId;
	bool keepLoaded;

	// the pak name that's being requested to be loaded for this particular slot
	char pakName[MAX_PATH];

	// the actual base pak name, like "common_pve.rpak" as set when this array is
	// being initialized
	const char* basePakName;
};

//-----------------------------------------------------------------------------
// this structure contains handles and names to the custom pak files that are
// loaded with the settings KV for that level, these paks are loaded after the
// common paks are loaded, but unloaded before the common paks are unloaded
//-----------------------------------------------------------------------------
struct CustomPakData_t
{
	enum EPakType
	{
		// the pak that loads after CommonPakData_t::PAK_TYPE_UI_GM has loaded, and
		// unloads before CommonPakData_t::PAK_TYPE_UI_GM gets unloaded
		PAK_TYPE_UI_SDK = 0,

		// the pak that loads after CommonPakData_t::PAK_TYPE_COMMON_GM has loaded,
		// and unloads before CommonPakData_t::PAK_TYPE_COMMON_GM gets unloaded
		PAK_TYPE_COMMON_SDK,

		// the total number of base SDK pak files
		PAK_TYPE_COUNT
	};

	enum
	{
		// the absolute max number of custom paks, note that the engine's limit
		// could still be reached before this number as game scripts and other
		// code still loads paks such as gladiator cards or load screens
		MAX_CUSTOM_PAKS = (PAK_MAX_LOADED_PAKS - CommonPakData_t::PAK_TYPE_COUNT)
	};

	CustomPakData_t()
	{
		for (size_t i = 0; i < V_ARRAYSIZE(handles); i++)
		{
			handles[i] = PAK_INVALID_HANDLE;
		}

		// the first # handles are reserved for base SDK paks
		numHandles = PAK_TYPE_COUNT;
		numPreload = 0;

		levelResourcesLoaded = false;
		basePaksLoaded = false;
	}

	PakHandle_t LoadAndAddPak(const char* const pakFile);
	PakHandle_t PreloadAndAddPak(const char* const pakFile);

	void UnloadAndRemoveNonPreloaded();
	void UnloadAndRemovePreloaded();

	PakHandle_t LoadBasePak(const char* const pakFile, const EPakType type);
	void UnloadBasePak(const EPakType type);

private:
	void UnloadAndRemovePak(const int index);

public:
	// Pak handles that have been loaded with the level
	// from within the level settings KV (located in
	// scripts/levels/settings/*.kv). On level unload,
	// each pak listed in this vector gets unloaded.
	PakHandle_t handles[MAX_CUSTOM_PAKS];

	int numHandles;
	int numPreload;

	bool levelResourcesLoaded;
	bool basePaksLoaded;
};

// array size = CommonPakData_t::PAK_TYPE_COUNT
inline CommonPakData_t* g_commonPakData;

inline void(*v_Mod_LoadPakForMap)(const char* szLevelName);
inline void(*v_Mod_QueuedPakCacheFrame)(void);

inline int32_t * g_pNumPrecacheItemsMTVTF;
inline bool* g_pPakPrecacheJobFinished;

inline void(*Mod_UnloadPendingAndPrecacheRequestedPaks)(void);

extern CUtlVector<CUtlString> g_InstalledMaps;
extern CThreadMutex g_InstalledMapsMutex;

void Mod_PreloadPaks();
void Mod_UnloadPreloadedPaks();

bool Mod_LevelHasChanged(const char* pszLevelName);
void Mod_GetAllInstalledMaps();
KeyValues* Mod_GetLevelSettings(const char* pszLevelName);
void Mod_LoadLevelPaks(const char* pszLevelName);
void Mod_UnloadLevelPaks(void);


///////////////////////////////////////////////////////////////////////////////
class VModel_BSP : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Mod_LoadPakForMap", v_Mod_LoadPakForMap);
		LogFunAdr("Mod_QueuedPakCacheFrame", v_Mod_QueuedPakCacheFrame);

		LogFunAdr("Mod_UnloadPendingAndPrecacheRequestedPaks", Mod_UnloadPendingAndPrecacheRequestedPaks);

		LogVarAdr("g_numPrecacheItemsMTVTF", g_pNumPrecacheItemsMTVTF);
		LogVarAdr("g_pakPrecacheJobFinished", g_pPakPrecacheJobFinished);

		LogVarAdr("g_commonPakData", g_commonPakData);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? 0F B6 05 ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? 84 C0").GetPtr(v_Mod_LoadPakForMap);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC ?? F3 0F 10 05 ?? ?? ?? ?? 32 DB").GetPtr(v_Mod_QueuedPakCacheFrame);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 33 ED 48 8D 35 ?? ?? ?? ?? 48 39 2D ?? ?? ?? ??").GetPtr(Mod_UnloadPendingAndPrecacheRequestedPaks);
	}
	virtual void GetVar(void) const
	{
		g_pNumPrecacheItemsMTVTF = CMemory(v_Mod_QueuedPakCacheFrame).FindPattern("8B 05").ResolveRelativeAddressSelf(0x2, 0x6).RCast<int32_t*>();
		g_pPakPrecacheJobFinished = CMemory(v_Mod_QueuedPakCacheFrame).Offset(0x20).FindPatternSelf("88 1D").ResolveRelativeAddressSelf(0x2, 0x6).RCast<bool*>();

		CMemory(v_Mod_QueuedPakCacheFrame).Offset(0xA0).FindPatternSelf("48 8D 2D").ResolveRelativeAddressSelf(0x3, 0x7).GetPtr(g_commonPakData);
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
