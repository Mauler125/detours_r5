//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/memstd.h"
#include "tier0/jobthread.h"
#include "tier1/fmtstr.h"
#include "tier1/keyvalues.h"
#include "tier2/fileutils.h"
#include "engine/sys_dll2.h"
#include "engine/host_cmd.h"
#include "engine/cmodel_bsp.h"

#include "rtech/rson.h"
#include "rtech/pak/pakstate.h"
#include "rtech/pak/pakparse.h"
#include "rtech/pak/paktools.h"
#include "rtech/pak/pakstream.h"

#include "vpklib/packedstore.h"
#include "datacache/mdlcache.h"
#include "filesystem/filesystem.h"
#ifndef DEDICATED
#include "client/clientstate.h"
#endif // !DEDICATED

CUtlVector<CUtlString> g_InstalledMaps;
CFmtStrN<MAX_MAP_NAME> s_CurrentLevelName;

static CustomPakData_s s_customPakData;
static KeyValues* s_pLevelSetKV = nullptr;

//-----------------------------------------------------------------------------
// Purpose: load a custom pak and add it to the list
//-----------------------------------------------------------------------------
PakHandle_t CustomPakData_s::LoadAndAddPak(const char* const pakFile)
{
    if (numHandles >= MAX_CUSTOM_PAKS)
    {
        Error(eDLL_T::ENGINE, NO_ERROR, "Tried to load pak '%s', but already reached the limit of %d!\n", pakFile, MAX_CUSTOM_PAKS);
        return PAK_INVALID_HANDLE;
    }

    const PakHandle_t pakId = g_pakLoadApi->LoadAsync(pakFile, AlignedMemAlloc(), 4, 0);

    // failure, don't add; return the invalid handle.
    if (pakId == PAK_INVALID_HANDLE)
        return pakId;

    handles[numHandles++] = pakId;
    return pakId;
}

//-----------------------------------------------------------------------------
// Purpose: unload a custom pak
// NOTE   : the array must be kept contiguous; this means that the last pak in
//          the array should always be unloaded fist!
//-----------------------------------------------------------------------------
void CustomPakData_s::UnloadAndRemovePak(const int index)
{
    const PakHandle_t pakId = handles[index];
    assert(pakId != PAK_INVALID_HANDLE); // invalid handles should not be inserted

    g_pakLoadApi->UnloadAsync(pakId);
    handles[index] = PAK_INVALID_HANDLE;

    numHandles--;
}

//-----------------------------------------------------------------------------
// Purpose: preload a custom pak; this keeps it available throughout the
//          duration of the process, unless manually removed by user.
//-----------------------------------------------------------------------------
PakHandle_t CustomPakData_s::PreloadAndAddPak(const char* const pakFile)
{
    // this must never be called after a non-preloaded pak has been added!
    // preloaded paks must always appear before custom user requested paks
    // due to the unload order: user-requested -> preloaded -> sdk -> core.
    assert(handles[CustomPakData_s::PAK_TYPE_COUNT+numPreload] == PAK_INVALID_HANDLE);

    const PakHandle_t pakId = LoadAndAddPak(pakFile);

    if (pakId != PAK_INVALID_HANDLE)
        numPreload++;

    return pakId;
}

//-----------------------------------------------------------------------------
// Purpose: unloads all non-preloaded custom pak handles
//-----------------------------------------------------------------------------
void CustomPakData_s::UnloadAndRemoveNonPreloaded()
{
    // Preloaded paks should not be unloaded here, but only right before sdk /
    // engine paks are unloaded. Only unload user requested and level settings
    // paks from here. Unload them in reverse order, the last pak loaded should
    // be the first one to be unloaded.
    for (int n = numHandles-1; n >= CustomPakData_s::PAK_TYPE_COUNT + numPreload; n--)
    {
        UnloadAndRemovePak(n);
    }
}

//-----------------------------------------------------------------------------
// Purpose: unloads all preloaded custom pak handles
//-----------------------------------------------------------------------------
void CustomPakData_s::UnloadAndRemovePreloaded()
{
    // Unload them in reverse order, the last pak loaded should be the first
    // one to be unloaded.
    for (; numPreload > 0; numPreload--)
    {
        UnloadAndRemovePak(CustomPakData_s::PAK_TYPE_COUNT + (numPreload-1));
    }
}

//-----------------------------------------------------------------------------
// Purpose: loads the base SDK pak file by type
//-----------------------------------------------------------------------------
PakHandle_t CustomPakData_s::LoadBasePak(const char* const pakFile, const PakType_e type)
{
    const PakHandle_t pakId = g_pakLoadApi->LoadAsync(pakFile, AlignedMemAlloc(), 4, 0);

    // the file is most likely missing
    assert(pakId != PAK_INVALID_HANDLE);
    handles[type] = pakId;

    return pakId;
}

//-----------------------------------------------------------------------------
// Purpose: unload the SDK base pak file by type
//-----------------------------------------------------------------------------
void CustomPakData_s::UnloadBasePak(const PakType_e type)
{
    const PakHandle_t pakId = handles[type];

    // only unload if it was actually successfully loaded
    if (pakId != PAK_INVALID_HANDLE)
    {
        g_pakLoadApi->UnloadAsync(pakId);
        handles[type] = PAK_INVALID_HANDLE;
    }
}

//-----------------------------------------------------------------------------
// Purpose: checks if level has changed
// Input  : *pszLevelName - 
// Output : true if level name deviates from previous level
//-----------------------------------------------------------------------------
bool Mod_LevelHasChanged(const char* const pszLevelName)
{
    return (V_strcmp(pszLevelName, s_CurrentLevelName.String()) != NULL);
}

//-----------------------------------------------------------------------------
// Purpose: gets all installed maps
//-----------------------------------------------------------------------------
void Mod_GetAllInstalledMaps()
{
    CUtlVector<CUtlString> fileList;
    AddFilesToList(fileList, "vpk", "vpk", nullptr, '/');

    std::cmatch regexMatches;
    AUTO_LOCK(g_InstalledMapsMutex);

    g_InstalledMaps.Purge(); // Clear current list.

    FOR_EACH_VEC(fileList, i)
    {
        const CUtlString& filePath = fileList[i];
        const char* pFileName = strrchr(filePath.Get(), '/')+1;

        // Should always point right in front of the last
        // slash, as the files are loaded from 'vpk/'.
        Assert(pFileName);

        std::regex_search(pFileName, regexMatches, g_VpkDirFileRegex);

        if (!regexMatches.empty())
        {
            const std::sub_match<const char*>& match = regexMatches[2];

            if (match.compare("frontend") == 0)
                continue; // Frontend contains no BSP's.

            else if (match.compare("mp_common") == 0)
            {
                if (!g_InstalledMaps.HasElement("mp_lobby"))
                    g_InstalledMaps.AddToTail("mp_lobby");

                continue; // Common contains mp_lobby.
            }
            else
            {
                const string mapName = match.str();

                if (!g_InstalledMaps.HasElement(mapName.c_str()))
                    g_InstalledMaps.AddToTail(mapName.c_str());
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the load job for given pak id is finished
//-----------------------------------------------------------------------------
static bool Mod_IsPakLoadFinished(const PakHandle_t pakId)
{
    if (pakId == PAK_INVALID_HANDLE)
        return true;

    const PakLoadedInfo_s* const pli = Pak_GetPakInfo(pakId);

    if (pli->handle != pakId)
        return false;

    const PakStatus_e stat = pli->status;

    if (stat != PakStatus_e::PAK_STATUS_LOADED && 
        stat != PakStatus_e::PAK_STATUS_ERROR)
        return false;

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the load job for custom pak batch for given common
//          pak is finished
//-----------------------------------------------------------------------------
static bool CustomPakData_IsPakLoadFinished(const CommonPakData_s::PakType_e commonType)
{
    switch (commonType)
    {
    case CommonPakData_s::PakType_e::PAK_TYPE_UI_GM:
#ifndef DEDICATED
        return Mod_IsPakLoadFinished(s_customPakData.handles[CustomPakData_s::PakType_e::PAK_TYPE_UI_SDK]);
#else // Dedicated doesn't load UI paks.
        return true;
#endif // DEDICATED
    case CommonPakData_s::PakType_e::PAK_TYPE_COMMON:
        return true;
    case CommonPakData_s::PakType_e::PAK_TYPE_COMMON_GM:
        return Mod_IsPakLoadFinished(s_customPakData.handles[CustomPakData_s::PakType_e::PAK_TYPE_COMMON_SDK]);
    case CommonPakData_s::PakType_e::PAK_TYPE_LOBBY:
        // Check for preloaded paks at this stage (loaded from preload.rson).
        for (int i = 0, n = s_customPakData.numPreload; i < n; i++)
        {
            if (!Mod_IsPakLoadFinished(s_customPakData.handles[CustomPakData_s::PAK_TYPE_COUNT + i]))
                return false;
        }
        break;
    case CommonPakData_s::PakType_e::PAK_TYPE_LEVEL:
        // Check for extra level paks at this stage (loaded from <levelname>.kv).
        for (int i = CustomPakData_s::PAK_TYPE_COUNT + s_customPakData.numPreload, n = s_customPakData.numHandles; i < n; i++)
        {
            if (!Mod_IsPakLoadFinished(s_customPakData.handles[i]))
                return false;
        }
        break;
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: processes queued pak files
//-----------------------------------------------------------------------------
static void Mod_QueuedPakCacheFrame()
{
#ifndef DEDICATED
    bool bUnconnected = !(*g_pClientState_Shifted)->IsConnected();
#else // !DEDICATED
    bool bUnconnected = true; // Always true for dedicated.
#endif

    bool startFromFirst = false;

    if (Pak_StreamingDownloadFinished() && Pak_GetNumStreamableAssets() && bUnconnected)
    {
        *g_pPakPrecacheJobFinished = false;
        startFromFirst = true;
    }
    else if (*g_pPakPrecacheJobFinished)
    {
        return;
    }

    if (!FileSystem()->ResetItemCache() || *g_pNumPrecacheItemsMTVTF)
    {
        return;
    }

    const char** pPakName = &g_commonPakData[0].basePakName;
    int i;

    for (i = 0; i < 5; ++i)
    {
        if (*((_BYTE*)pPakName - 268))
            break;

        const char* pakName = g_commonPakData[i].pakName;
        const int64_t v4 = *pPakName - pakName;

        int v5;
        int v6;

        do
        {
            v5 = (unsigned __int8)pakName[v4];
            v6 = (unsigned __int8)*pakName - v5;
            if (v6)
                break;

            ++pakName;
        } while (v5);

        if (v6)
            break;

        pPakName += 35;
    }

    int startIndex = 0;

    if (!startFromFirst)
        startIndex = i; // start from last pre-cached

    const int numToProcess = startIndex;

    if (startIndex < CommonPakData_s::PAK_TYPE_COUNT)
    {
        bool keepLoaded = false;
        int numLeftToProcess = 4;
        CommonPakData_s* data = &g_commonPakData[4];

        do
        {
            if (*data->pakName)
            {
                PakLoadedInfo_s* const pakInfo = Pak_GetPakInfo(data->pakId);
                PakStatus_e status;

                data->keepLoaded = true;

                if (pakInfo->handle == data->pakId)
                {
                    status = pakInfo->status;
                    keepLoaded = data->keepLoaded;
                }
                else
                {
                    status = PAK_STATUS_INVALID_PAKHANDLE;
                    keepLoaded = true;
                }

                if (!keepLoaded || status == PAK_STATUS_LOADED)
                {
                    // SDK pak files must be unloaded before the engine pak files,
                    // as we use assets within engine pak files.
                    switch (numLeftToProcess)
                    {
#ifndef DEDICATED
                    case CommonPakData_s::PakType_e::PAK_TYPE_UI_GM:
                        s_customPakData.UnloadBasePak(CustomPakData_s::PakType_e::PAK_TYPE_UI_SDK);
                        break;
#endif // !DEDICATED

                    case CommonPakData_s::PakType_e::PAK_TYPE_COMMON:
                        g_StudioMdlFallbackHandler.Clear();
                        break;

                    case CommonPakData_s::PakType_e::PAK_TYPE_COMMON_GM:
                        s_customPakData.UnloadBasePak(CustomPakData_s::PakType_e::PAK_TYPE_COMMON_SDK);
                        break;

                    default:
                        break;
                    }

                    if (numLeftToProcess == CommonPakData_s::PakType_e::PAK_TYPE_LEVEL)
                    {
                        Mod_UnloadLevelPaks(); // Unload mod pak files.

                        if (s_pLevelSetKV)
                        {
                            // Delete current level settings if we drop all paks..
                            s_pLevelSetKV->DeleteThis();
                            s_pLevelSetKV = nullptr;
                        }
                    }

                    g_pakLoadApi->UnloadAsync(data->pakId);

                    if (numLeftToProcess == CommonPakData_s::PakType_e::PAK_TYPE_LOBBY)
                    {
                        Mod_UnloadPreloadedPaks();
                        s_customPakData.basePaksLoaded = false;
                    }
                }

                if (status && (unsigned int)(status - 13) > 1)
                    return;

                data->keepLoaded = false;
                data->pakName[0] = '\0';

                data->pakId = PAK_INVALID_HANDLE;
            }
            --numLeftToProcess;
            --data;
        } while (numLeftToProcess >= numToProcess);
    }

    *g_pPakPrecacheJobFinished = true;
    CommonPakData_s* commonData = g_commonPakData;

    int it = 0;

    char* name;
    char* nameIt;

    while (true)
    {
        name = g_commonPakData[it].pakName;
        nameIt = name;
        char c;
        int v20;
        do
        {
            c = (unsigned __int8)nameIt[(unsigned __int64)(commonData->basePakName - (const char*)name)];
            v20 = (unsigned __int8)*nameIt - c;
            if (v20)
                break;

            ++nameIt;
        } while (c);

        if (!v20)
            goto CHECK_LOAD_STATUS;

        V_strncpy(name, commonData->basePakName, MAX_PATH);

        if (*commonData->pakName)
            break;

        commonData->pakId = PAK_INVALID_HANDLE;
    LOOP_AGAIN_OR_FINISH:

        ++it;
        ++commonData;
        if (it >= 5)
        {
            if (*g_pPakPrecacheJobFinished)
            {
                __int64 pMTVFTaskItem = *g_pMTVFTaskItem;
                if (pMTVFTaskItem)
                {
                    if (!*(_BYTE*)(pMTVFTaskItem + 4))
                    {
                        JobFifoLock_s* const pakFifoLock = &g_pakGlobals->fifoLock;

                        if (g_pakGlobals->hasPendingUnloadJobs || g_pakGlobals->loadedPakCount != g_pakGlobals->requestedPakCount)
                        {
                            if (!JT_AcquireFifoLockOrHelp(pakFifoLock)
                                && !JT_HelpWithJobTypes(g_pPakFifoLockWrapper, pakFifoLock, -1i64, 0i64))
                            {
                                JT_HelpWithJobTypesOrSleep(g_pPakFifoLockWrapper, pakFifoLock, -1i64, 0i64, 0i64, 1);
                            }

                            Mod_UnloadPendingAndPrecacheRequestedPaks();

                            if (ThreadInMainThread())
                            {
                                if (*g_bPakFifoLockAcquiredInMainThread)
                                {
                                    *g_bPakFifoLockAcquiredInMainThread = false;
                                    JT_ReleaseFifoLock(pakFifoLock);
                                }
                            }

                            JT_ReleaseFifoLock(pakFifoLock);

                            pMTVFTaskItem = *g_pMTVFTaskItem;
                        }

                        FileSystem()->ResetItemCacheSize(256);
                        FileSystem()->PrecacheTaskItem(pMTVFTaskItem);
                    }
                }
            }
            return;
        }
    }

    if (it == CommonPakData_s::PakType_e::PAK_TYPE_LOBBY)
    {
        Mod_PreloadPaks();
        s_customPakData.basePaksLoaded = true;
    }

    commonData->pakId = g_pakLoadApi->LoadAsync(name, AlignedMemAlloc(), 4, 0);

    if (it == CommonPakData_s::PakType_e::PAK_TYPE_LEVEL)
    {
        Mod_LoadLevelPaks(s_CurrentLevelName.String());
        s_customPakData.levelResourcesLoaded = true;
    }

#ifndef DEDICATED
    if (it == CommonPakData_s::PakType_e::PAK_TYPE_UI_GM)
        s_customPakData.LoadBasePak("ui_sdk.rpak", CustomPakData_s::PakType_e::PAK_TYPE_UI_SDK);
    else
#endif // !DEDICATED
    if (it == CommonPakData_s::PakType_e::PAK_TYPE_COMMON_GM)
        s_customPakData.LoadBasePak("common_sdk.rpak", CustomPakData_s::PakType_e::PAK_TYPE_COMMON_SDK);

CHECK_LOAD_STATUS:

    if (!Mod_IsPakLoadFinished(commonData->pakId) || !CustomPakData_IsPakLoadFinished(CommonPakData_s::PakType_e(it)))
        *g_pPakPrecacheJobFinished = false;

    goto LOOP_AGAIN_OR_FINISH;
}

//-----------------------------------------------------------------------------
// Purpose: preload paks in list and keeps them active throughout level changes
//-----------------------------------------------------------------------------
void Mod_PreloadPaks()
{
    static const char* const preloadFile = "paks/preload.rson";
    bool parseFailure = false;

    RSON::Node_t* const rson = RSON::LoadFromFile(preloadFile, "GAME", &parseFailure);

    if (!rson)
    {
        if (parseFailure)
            Error(eDLL_T::ENGINE, EXIT_FAILURE, "%s: failure parsing file '%s'\n", __FUNCTION__, preloadFile);
        else
        {
            Warning(eDLL_T::ENGINE, "%s: could not load file '%s'\n", __FUNCTION__, preloadFile);
            return; // No preload file, thus no error. Warn and return out.
        }
    }

    static const char* const arrayName = "Paks";
    const RSON::Field_t* const key = rson->FindKey(arrayName);

    if (!key)
        Error(eDLL_T::ENGINE, EXIT_FAILURE, "%s: missing array key \"%s\" in file '%s'\n", __FUNCTION__, arrayName, preloadFile);

    if ((key->m_Node.m_Type != (RSON::eFieldType::RSON_ARRAY | RSON::eFieldType::RSON_STRING)) &&
        (key->m_Node.m_Type != (RSON::eFieldType::RSON_ARRAY | RSON::eFieldType::RSON_VALUE)))
    {
        Error(eDLL_T::ENGINE, EXIT_FAILURE, "%s: expected an array of strings in file '%s'\n", __FUNCTION__, preloadFile);
    }

    for (int i = 0; i < key->m_Node.m_nValueCount; i++)
    {
        const RSON::Value_t* const value = key->m_Node.GetArrayValue(i);
        s_customPakData.PreloadAndAddPak(value->pszString);
    }

    RSON_Free(rson, AlignedMemAlloc());
    AlignedMemAlloc()->Free(rson);
}

//-----------------------------------------------------------------------------
// Purpose: unloads all preloaded paks
//-----------------------------------------------------------------------------
void Mod_UnloadPreloadedPaks()
{
    s_customPakData.UnloadAndRemovePreloaded();
}

//-----------------------------------------------------------------------------
// Purpose: load assets for level with fifolock.
// Input  : *szLevelName - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
void Mod_LoadPakForMap(const char* const pszLevelName)
{
	if (Mod_LevelHasChanged(pszLevelName))
        s_customPakData.levelResourcesLoaded = false;

	s_CurrentLevelName = pszLevelName;

	// Dedicated should not load loadscreens.
#ifndef DEDICATED
	v_Mod_LoadPakForMap(pszLevelName);
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: loads the level settings file, returns current if level hasn't changed.
// Input  : *pszLevelName - 
// Output : KeyValues*
//-----------------------------------------------------------------------------
KeyValues* Mod_GetLevelSettings(const char* const pszLevelName)
{
    if (s_pLevelSetKV)
    {
        // If we didn't change the level, return the current one
        if (s_customPakData.levelResourcesLoaded)
            return s_pLevelSetKV;

        s_pLevelSetKV->DeleteThis();
    }

    char szPathBuffer[MAX_PATH];
    snprintf(szPathBuffer, sizeof(szPathBuffer), "scripts/levels/settings/%s.kv", pszLevelName);

    s_pLevelSetKV = FileSystem()->LoadKeyValues(IFileSystem::TYPE_LEVELSETTINGS, szPathBuffer, "GAME");
    return s_pLevelSetKV;
}

//-----------------------------------------------------------------------------
// Purpose: loads paks specified inside the level settings file
// Input  : *pszLevelName - 
//-----------------------------------------------------------------------------
void Mod_LoadLevelPaks(const char* const pszLevelName)
{
    KeyValues* const pSettingsKV = Mod_GetLevelSettings(pszLevelName);

    if (!pSettingsKV)
        return;

    KeyValues* const pPakListKV = pSettingsKV->FindKey("PakList");

    if (!pPakListKV)
        return;

    char szPathBuffer[MAX_PATH];

    for (KeyValues* pSubKey = pPakListKV->GetFirstSubKey(); pSubKey != nullptr; pSubKey = pSubKey->GetNextKey())
    {
        if (!pSubKey->GetBool())
            continue;

        snprintf(szPathBuffer, sizeof(szPathBuffer), "%s.rpak", pSubKey->GetName());
        const PakHandle_t nPakId = s_customPakData.LoadAndAddPak(szPathBuffer);

        if (nPakId == PAK_INVALID_HANDLE)
            Error(eDLL_T::ENGINE, NO_ERROR, "%s: unable to load pak '%s' results '%d'\n", __FUNCTION__, szPathBuffer, nPakId);
    }
}

//-----------------------------------------------------------------------------
// Purpose: unloads all paks loaded by the level settings file
//-----------------------------------------------------------------------------
void Mod_UnloadLevelPaks()
{
    s_customPakData.UnloadAndRemoveNonPreloaded();

    g_StudioMdlFallbackHandler.ClearBadModelHandleCache();
    g_StudioMdlFallbackHandler.ClearSuppresionList();

    // The old gather props is set if a model couldn't be
    // loaded properly. If we unload level assets, we just
    // enable the new implementation again and re-evaluate
    // on the next level load. If we load a missing/bad
    // model again, we toggle the old implementation as
    // otherwise the fallback models won't render; the new
    // gather props solution does not attempt to obtain
    // studio hardware data on bad mdl handles. See
    // 'GatherStaticPropsSecondPass_PreInit()' for details.
    g_StudioMdlFallbackHandler.DisableLegacyGatherProps();
}

void VModel_BSP::Detour(const bool bAttach) const
{
	DetourSetup(&v_Mod_LoadPakForMap, &Mod_LoadPakForMap, bAttach);
	DetourSetup(&v_Mod_QueuedPakCacheFrame, &Mod_QueuedPakCacheFrame, bAttach);
}
