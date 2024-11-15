//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "tier1/strtools.h"
#include "engine/sys_engine.h"
#include "engine/sys_dll.h"
#include "engine/sys_dll2.h"
#include "engine/host_cmd.h"
#include "engine/traceinit.h"
#ifndef DEDICATED
#include "engine/sys_mainwind.h"
#include "inputsystem/inputsystem.h"
#include "vgui/vgui_baseui_interface.h"
#include "materialsystem/cmaterialsystem.h"
#include "windows/id3dx.h"
#include "client/vengineclient_impl.h"
#include "geforce/reflex.h"
#include "radeon/antilag.h"
#endif // !DEDICATED
#include "filesystem/filesystem.h"
constexpr char DFS_ENABLE_PATH[] = "/vpk/enable.txt";

//-----------------------------------------------------------------------------
// Figure out if we're running a Valve mod or not.
//-----------------------------------------------------------------------------
static bool IsValveMod(const char* pModName)
{
	return (Q_stricmp(pModName, "cstrike") == 0 ||
		Q_stricmp(pModName, "dod") == 0 ||
		Q_stricmp(pModName, "hl1mp") == 0 ||
		Q_stricmp(pModName, "tf") == 0 ||
		Q_stricmp(pModName, "hl2mp") == 0 ||
		Q_stricmp(pModName, "csgo") == 0);
}

//-----------------------------------------------------------------------------
// Figure out if we're running a Respawn mod or not.
//-----------------------------------------------------------------------------
static bool IsRespawnMod(const char* pModName)
{
	return (Q_stricmp(pModName, "r1") == 0 ||
		Q_stricmp(pModName, "r2") == 0 ||
		Q_stricmp(pModName, "r5") == 0);
}

//-----------------------------------------------------------------------------
// Initialize the VPK and file cache system
//-----------------------------------------------------------------------------
static void InitVPKSystem()
{
    char szCacheEnableFilePath[260]; // [rsp+20h] [rbp-118h] BYREF
    char bFixSlashes = FileSystem()->GetCurrentDirectory(szCacheEnableFilePath, sizeof(szCacheEnableFilePath)) ? szCacheEnableFilePath[0] : '\0';

    size_t nCachePathLen = strlen(szCacheEnableFilePath);
    size_t nCacheFileLen = sizeof(DFS_ENABLE_PATH)-1;

    if ((nCachePathLen + nCacheFileLen) < 0x104 || (nCacheFileLen = (sizeof(szCacheEnableFilePath)-1) - nCachePathLen, nCachePathLen != (sizeof(szCacheEnableFilePath)-1)))
    {
        strncat(szCacheEnableFilePath, DFS_ENABLE_PATH, nCacheFileLen)[sizeof(szCacheEnableFilePath)-1] = '\0';
        bFixSlashes = szCacheEnableFilePath[0];
    }
    if (bFixSlashes)
    {
        V_FixSlashes(szCacheEnableFilePath, '/');
    }
    if (!CommandLine()->CheckParm("-novpk") && FileSystem()->FileExists(szCacheEnableFilePath, nullptr))
    {
        FileSystem()->AddSearchPath(".", "MAIN", SearchPathAdd_t::PATH_ADD_TO_TAIL);
#ifndef DEDICATED
        FileSystem()->SetVPKCacheModeClient();
        FileSystem()->MountVPKFile("vpk/client_frontend.bsp");
#else // Dedicated runs server vpk's and must have 'vpk/mp_common.bsp' mounted.
        FileSystem()->SetVPKCacheModeServer();
        FileSystem()->MountVPKFile("vpk/server_mp_common.bsp");
#endif // !DEDICATED
    }
}

InitReturnVal_t CEngineAPI::VInit(CEngineAPI* pEngineAPI)
{
    return CEngineAPI__Init(pEngineAPI);
}

//-----------------------------------------------------------------------------
// Initialization, shutdown of a mod.
//-----------------------------------------------------------------------------
bool CEngineAPI::VModInit(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir)
{
    // Register new Pak Assets here!
    //RTech_RegisterAsset(0, 1, "", nullptr, nullptr, nullptr, CMemory(0x1660AD0A8).RCast<void**>(), 8, 8, 8, 0, 0xFFFFFFC);

	const bool results = CEngineAPI__ModInit(pEngineAPI, pModName, pGameDir);
	if (!IsValveMod(pModName) && !IsRespawnMod(pModName))
	{
#ifndef DEDICATED
		g_pEngineClient->SetRestrictServerCommands(true); // Restrict server commands.
		g_pEngineClient->SetRestrictClientCommands(true); // Restrict client commands.
#endif // !DEDICATED
	}

	return results;
}

//-----------------------------------------------------------------------------
// One-time setup, based on the initially selected mod
//-----------------------------------------------------------------------------
bool CEngineAPI::OnStartup(CEngineAPI* pEngineAPI, void* pInstance, const char* pStartupModName)
{
	const bool results =  CEngineAPI__OnStartup(pEngineAPI, pInstance, pStartupModName);
	return results;
}

//-----------------------------------------------------------------------------
// Sets startup info
//-----------------------------------------------------------------------------
void CEngineAPI::VSetStartupInfo(CEngineAPI* pEngineAPI, StartupInfo_t* pStartupInfo)
{
    if (*g_bTextMode)
    {
        return;
    }

    const size_t nBufLen = sizeof(pStartupInfo->m_szBaseDirectory);
    strncpy(g_szBaseDir, pStartupInfo->m_szBaseDirectory, nBufLen);

    g_pEngineParms->baseDirectory = g_szBaseDir;
    g_szBaseDir[nBufLen-1] = '\0';

    void** pCurrentInstance = &pEngineAPI->m_StartupInfo.m_pInstance;
    size_t nInstances = 6;
    do
    {
        pCurrentInstance += 16;
        uint64_t pInstance = *(_QWORD*)&pStartupInfo->m_pInstance;
        pStartupInfo = (StartupInfo_t*)((char*)pStartupInfo + 128);
        *((_QWORD*)pCurrentInstance - 8) = pInstance;
        *((_QWORD*)pCurrentInstance - 7) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[132];
        *((_QWORD*)pCurrentInstance - 6) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[148];
        *((_QWORD*)pCurrentInstance - 5) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[164];
        *((_QWORD*)pCurrentInstance - 4) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[180];
        *((_QWORD*)pCurrentInstance - 3) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[196];
        *((_QWORD*)pCurrentInstance - 2) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[212];
        *((_QWORD*)pCurrentInstance - 1) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[228];
        --nInstances;
    } while (nInstances);
    *(_QWORD*)pCurrentInstance = *(_QWORD*)&pStartupInfo->m_pInstance;
    *((_QWORD*)pCurrentInstance + 1) = *(_QWORD*)&pStartupInfo->m_szBaseDirectory[8];

    InitVPKSystem();

    v_TRACEINIT(NULL, "COM_InitFilesystem( m_StartupInfo.m_szInitialMod )", "COM_ShutdownFileSystem()");
    v_COM_InitFilesystem(pEngineAPI->m_StartupInfo.m_szInitialMod);

    *g_bTextMode = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngineAPI::PumpMessages()
{
#ifndef DEDICATED
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (in_syncRT->GetBool())
        (*g_fnSyncRTWithIn)();

    g_pInputSystem->PollInputState(v_UIEventDispatcher);
    g_pGame->DispatchAllStoredGameMessages();
#endif // !DEDICATED
}

#ifndef DEDICATED
//-----------------------------------------------------------------------------
// Purpose: force update NVIDIA Reflex Low Latency parameters
//-----------------------------------------------------------------------------
static void GFX_NVN_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData)
{
    GeForce_MarkLowLatencyParametersOutOfDate();
}

static void GFX_FFX_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData)
{
    const ConVar* pConVarRef = g_pCVar->FindVar(pConVar->GetName());
    Radeon_EnableLowLatencySDK(pConVarRef->GetBool());
}

static ConVar fps_max_low_latency("fps_max_low_latency", "0", FCVAR_RELEASE, "Frame rate limiter using Low Latency SDK. -1 indicates the use of desktop refresh. 0 is disabled.", true, -1.f, true, 295.f, GFX_NVN_Changed_f);

static ConVar gfx_nvnUseLowLatency("gfx_nvnUseLowLatency", "1", FCVAR_RELEASE | FCVAR_ARCHIVE, "Enables NVIDIA Reflex Low Latency SDK.", GFX_NVN_Changed_f);
static ConVar gfx_nvnUseLowLatencyBoost("gfx_nvnUseLowLatencyBoost", "0", FCVAR_RELEASE | FCVAR_ARCHIVE, "Enables NVIDIA Reflex Low Latency Boost.", GFX_NVN_Changed_f);

// NOTE: defaulted to 0 as it causes rubber banding on some hardware.
static ConVar gfx_nvnUseMarkersToOptimize("gfx_nvnUseMarkersToOptimize", "0", FCVAR_RELEASE, "Use NVIDIA Reflex Low Latency markers to optimize (requires Low Latency Boost to be enabled).", GFX_NVN_Changed_f);

static ConVar gfx_ffxUseLowLatency("gfx_ffxUseLowLatency", "1", FCVAR_RELEASE | FCVAR_ARCHIVE, "Enables AMD Anti-Lag 2 Low Latency SDK.", GFX_FFX_Changed_f);

static float NormalizeFrameRate(const float fpsMax)
{
    if (fpsMax != -1.0f)
        return fpsMax;

    const float globalFps = fps_max->GetFloat();

    // Make sure the global fps limiter is 'unlimited'
    // before we let the low-latency frame limiter cap
    // it to the desktop's refresh rate; not adhering
    // to this will result in a major performance drop.
    if (globalFps == 0.0f)
        return g_pGame->GetTVRefreshRate();
    else
        return 0.0f; // Don't let the low-latency SDK limit the frame rate.
}
#endif // !DEDICATED

void CEngineAPI::UpdateLowLatencyParameters()
{
#ifndef DEDICATED
    const bool bUseLowLatencyMode = gfx_nvnUseLowLatency.GetBool();
    const bool bUseLowLatencyBoost = gfx_nvnUseLowLatencyBoost.GetBool();
    const bool bUseMarkersToOptimize = gfx_nvnUseMarkersToOptimize.GetBool();

    const float fpsMax = NormalizeFrameRate(fps_max_low_latency.GetFloat());

    GeForce_UpdateLowLatencyParameters(D3D11Device(), bUseLowLatencyMode,
        bUseLowLatencyBoost, bUseMarkersToOptimize, fpsMax);
#endif // !DEDICATED
}

void CEngineAPI::RunLowLatencyFrame()
{
#ifndef DEDICATED
    if (GeForce_IsLowLatencySDKAvailable())
    {
        if (GeForce_HasPendingLowLatencyParameterUpdates())
        {
            UpdateLowLatencyParameters();
        }

        GeForce_RunLowLatencyFrame(D3D11Device());
    }

    if (Radeon_IsLowLatencySDKAvailable())
    {
        const float maxFps = NormalizeFrameRate(fps_max_low_latency.GetFloat());
        Radeon_RunLowLatencyFrame((unsigned int)maxFps);
    }
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEngineAPI::MainLoop()
{
#ifndef DEDICATED
    bool bRunLowLatency = false;
#endif // !DEDICATED

    // Main message pump
    while (true)
    {
        // Pump messages unless someone wants to quit
        if (g_pEngine->GetQuitting() != IEngine::QUIT_NOTQUITTING)
        {
            if (g_pEngine->GetQuitting() != IEngine::QUIT_TODESKTOP) {
                return true;
            }

            return false;
        }

#ifndef DEDICATED
        if (bRunLowLatency) {
            CEngineAPI::RunLowLatencyFrame();
            bRunLowLatency = false;
        }
        CEngineAPI::PumpMessages();
#endif // !DEDICATED

        if (g_pEngine->Frame())
        {
#ifndef DEDICATED
            // Only run low-latency if we ran an actual engine frame.
            bRunLowLatency = true;
#endif // !DEDICATED
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void VSys_Dll2::Detour(const bool bAttach) const
{
	DetourSetup(&CEngineAPI__Init, &CEngineAPI::VInit, bAttach);
	DetourSetup(&CEngineAPI__ModInit, &CEngineAPI::VModInit, bAttach);
	DetourSetup(&CEngineAPI__OnStartup, &CEngineAPI::OnStartup, bAttach);
	DetourSetup(&CEngineAPI__PumpMessages, &CEngineAPI::PumpMessages, bAttach);
	DetourSetup(&CEngineAPI__MainLoop, &CEngineAPI::MainLoop, bAttach);
	DetourSetup(&CEngineAPI__SetStartupInfo, &CEngineAPI::VSetStartupInfo, bAttach);
}
