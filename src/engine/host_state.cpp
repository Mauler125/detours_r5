//=============================================================================//
//
// Purpose: Runs the state machine for the host & server.
//
//=============================================================================//
// host_state.cpp:.
//
/////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "tier0/jobthread.h"
#include "tier0/commandline.h"
#include "tier0/fasttimer.h"
#include "tier0/frametask.h"
#include "tier1/cvar.h"
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "datacache/mdlcache.h"
#ifndef CLIENT_DLL
#include "engine/server/sv_rcon.h"
#include "engine/server/server.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "engine/client/cl_rcon.h"
#include "engine/client/cl_main.h"
#include "engine/client/clientstate.h"
#endif // DEDICATED
#include "engine/cmd.h"
#include "engine/net.h"
#include "engine/gl_screen.h"
#include "engine/host.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/sys_engine.h"
#include "engine/modelloader.h"
#include "engine/cmodel_bsp.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#include "rtech/liveapi/liveapi.h"
#endif // !CLIENT_DLL
#include "rtech/stryder/stryder.h"
#include "rtech/playlists/playlists.h"
#ifndef DEDICATED
#include "vgui/vgui_baseui_interface.h"
#include "client/vengineclient_impl.h"
#include "gameui/imgui_system.h"
#endif // DEDICATED
#include "networksystem/pylon.h"
#ifndef CLIENT_DLL
#include "networksystem/bansystem.h"
#include "networksystem/hostmanager.h"
#endif // !CLIENT_DLL
#include "networksystem/listmanager.h"
#include "public/edict.h"
#ifndef CLIENT_DLL
#include "game/server/gameinterface.h"
#endif // !CLIENT_DLL
#include "game/shared/vscript_shared.h"

#ifndef CLIENT_DLL
static ConVar host_statusRefreshRate("host_statusRefreshRate", "0.5", FCVAR_RELEASE, "Host status refresh rate (seconds).", true, 0.f, false, 0.f);

static ConVar host_autoReloadRate("host_autoReloadRate", "0", FCVAR_RELEASE, "Time in seconds between each auto-reload (disabled if null).");
static ConVar host_autoReloadRespectGameState("host_autoReloadRespectGameState", "0", FCVAR_RELEASE, "Check the game state before proceeding to auto-reload (don't reload in the middle of a match).");
#endif // !CLIENT_DLL

ConVar hostdesc("hostdesc", "", FCVAR_RELEASE, "Host game server description.");

#ifdef DEDICATED
//-----------------------------------------------------------------------------
// Purpose: Send keep alive request to Pylon Master Server.
// Output : Returns true on success, false otherwise.
//-----------------------------------------------------------------------------
static void HostState_KeepAlive()
{
	if (!g_pServer->IsActive() || !sv_pylonVisibility.GetBool()) // Check for active game.
	{
		return;
	}

	const NetGameServer_t gameServer
	{
		hostname->GetString(),
		hostdesc.GetString(),
		sv_pylonVisibility.GetInt() == ServerVisibility_e::HIDDEN,
		g_pHostState->m_levelName,
		v_Playlists_GetCurrent(),
		hostip->GetString(),
		hostport->GetInt(),
		g_pNetKey->GetBase64NetKey(),
		*g_nServerRemoteChecksum,
		SDK_VERSION,
		g_pServer->GetNumClients(),
		gpGlobals->maxClients,
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()
			).count()
	};

	std::thread request([&, gameServer]
		{
			string errorMsg;
			string hostToken;
			string hostIp;

			const bool result = g_MasterServer.PostServerHost(errorMsg, hostToken, hostIp, gameServer);

			// Apply the data the next frame
			g_TaskQueue.Dispatch([result, errorMsg, hostToken, hostIp]
				{
					if (!result)
					{
						if (!errorMsg.empty() && g_ServerHostManager.GetCurrentError().compare(errorMsg) != NULL)
						{
							g_ServerHostManager.SetCurrentError(errorMsg);
							Error(eDLL_T::SERVER, NO_ERROR, "%s\n", errorMsg.c_str());
						}
					}
					else // Attempt to log the token, if there is one.
					{
						if (!hostToken.empty() && g_ServerHostManager.GetCurrentToken().compare(hostToken) != NULL)
						{
							g_ServerHostManager.SetCurrentToken(hostToken);
							Msg(eDLL_T::SERVER, "Published server with token: %s'%s%s%s'\n",
								g_svReset, g_svGreyB,
								hostToken.c_str(), g_svReset);
						}
					}

					if (hostIp.length() != 0)
						g_ServerHostManager.SetHostIP(hostIp);

				}, 0);
		}
	);

	request.detach();
}
#endif // DEDICATED

#ifndef CLIENT_DLL
void HostState_HandleAutoReload()
{
	if (host_autoReloadRate.GetBool())
	{
		if (gpGlobals->curTime > host_autoReloadRate.GetFloat())
		{
			// We should respect the game state, and the game isn't finished yet so
			// don't reload the server now.
			if (host_autoReloadRespectGameState.GetBool() && !g_hostReloadState)
				return;

			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "reload\n", cmd_source_t::kCommandSrcCode);
		}
	}
}
#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: state machine's main processing loop
//-----------------------------------------------------------------------------
void CHostState::FrameUpdate(CHostState* pHostState, double flCurrentTime, float flFrameTime)
{
	static bool bInitialized = false;
	static bool bResetIdleName = false;
	if (!bInitialized)
	{
		g_pHostState->Setup();
		bInitialized = true;
	}

	g_pHostState->Think();
#ifndef CLIENT_DLL
	RCONServer()->RunFrame();
#endif // !CLIENT_DLL
#ifndef DEDICATED
	RCONClient()->RunFrame();
#endif // !DEDICATED

	// Disable "warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable"
#pragma warning(push)
#pragma warning(disable : 4611)
	if (setjmp(*host_abortserver))
	{
		g_pHostState->Init();
		return;
	}
#pragma warning(pop)
	else
	{
#ifndef CLIENT_DLL
		*g_bAbortServerSet = true;
#endif // !CLIENT_DLL
		while (true)
		{
			Cbuf_Execute();

			const HostStates_t oldState = g_pHostState->m_iCurrentState;
			switch (g_pHostState->m_iCurrentState)
			{
			case HostStates_t::HS_NEW_GAME:
			{
				g_pHostState->State_NewGame();
				break;
			}
			case HostStates_t::HS_CHANGE_LEVEL_SP:
			{
				g_pHostState->State_ChangeLevelSP();
				break;
			}
			case HostStates_t::HS_CHANGE_LEVEL_MP:
			{
				g_pHostState->State_ChangeLevelMP();
				break;
			}
			case HostStates_t::HS_RUN:
			{
				if (!g_pHostState->m_bActiveGame)
				{
					if (bResetIdleName)
					{
						g_pHostState->ResetLevelName();
						bResetIdleName = false;
					}
				}
				else // Reset idle name the next non-active frame.
				{
					bResetIdleName = true;
				}

				CHostState__State_Run(&g_pHostState->m_iCurrentState, flCurrentTime, flFrameTime);
				break;
			}
			case HostStates_t::HS_GAME_SHUTDOWN:
			{
				Msg(eDLL_T::ENGINE, "%s: Shutdown host game\n", __FUNCTION__);
				CHostState__State_GameShutDown(g_pHostState);
				break;
			}
			case HostStates_t::HS_RESTART:
			{
				Msg(eDLL_T::ENGINE, "%s: Restarting state machine\n", __FUNCTION__);
#ifndef DEDICATED
				v_CL_EndMovie();
#endif // !DEDICATED
				v_Stryder_SendOfflineRequest(); // We have hostnames nulled anyway.
				g_pEngine->SetNextState(IEngine::DLL_RESTART);
				break;
			}
			case HostStates_t::HS_SHUTDOWN:
			{
				Msg(eDLL_T::ENGINE, "%s: Shutdown state machine\n", __FUNCTION__);
#ifndef DEDICATED
				v_CL_EndMovie();
#endif // !DEDICATED
				v_Stryder_SendOfflineRequest(); // We have hostnames nulled anyway.
				g_pEngine->SetNextState(IEngine::DLL_CLOSE);
				break;
			}
			default:
			{
				break;
			}
			}

			// only do a single pass at HS_RUN per frame. All other states loop until they reach HS_RUN 
			if (oldState == HostStates_t::HS_RUN && (g_pHostState->m_iNextState != HostStates_t::HS_LOAD_GAME || !single_frame_shutdown_for_reload->GetBool()))
				break;

			// shutting down
			if (oldState == HostStates_t::HS_SHUTDOWN ||
				oldState == HostStates_t::HS_RESTART)
				break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: state machine initialization
//-----------------------------------------------------------------------------
void CHostState::Init(void)
{
	if (m_iNextState != HostStates_t::HS_SHUTDOWN)
	{
		if (m_iNextState == HostStates_t::HS_GAME_SHUTDOWN)
		{
			CHostState__State_GameShutDown(this);
		}
		else
		{
			m_iCurrentState = HostStates_t::HS_RUN;
			if (m_iNextState != HostStates_t::HS_SHUTDOWN || !single_frame_shutdown_for_reload->GetInt())
				m_iNextState = HostStates_t::HS_RUN;
		}
	}
	m_flShortFrameTime = 1.0f;
	m_bActiveGame = false;
	m_bRememberLocation = false;
	m_bBackgroundLevel = false;
	m_bWaitingForConnection = false;
	m_levelName[0] = 0;
	m_landMarkName[0] = 0;
	m_mapGroupName[0] = 0;
	m_bSplitScreenConnect = false;
	m_bGameHasShutDownAndFlushedMemory = true;
	m_vecLocation.Init();
	m_angLocation.Init();
	m_iServerState = HostStates_t::HS_NEW_GAME;
}

//-----------------------------------------------------------------------------
// Purpose: state machine setup
//-----------------------------------------------------------------------------
void CHostState::Setup(void) 
{
	g_pHostState->LoadConfig();
#ifndef CLIENT_DLL
	g_BanSystem.LoadList();
#endif // !CLIENT_DLL
	ConVar_PurgeHostNames();

#ifndef CLIENT_DLL
	LiveAPISystem()->Init();
#endif // !CLIENT_DLL

	if (net_useRandomKey.GetBool())
	{
		NET_GenerateKey();
	}
#if !defined (DEDICATED) && !defined (CLIENT_DLL)
	// Parallel processing of 'C_BaseAnimating::SetupBones()' is currently
	// not supported on listen servers running the local client due to an
	// engine bug specific to S3 that still needs to be addressed. Remove
	// this once the issue has been solved:
	if (cl_threaded_bone_setup->GetBool())
	{
		cl_threaded_bone_setup->SetValue(false);
	}
#endif // !DEDICATED && !CLIENT_DLL

	ResetLevelName();
}

//-----------------------------------------------------------------------------
// Purpose: think
//-----------------------------------------------------------------------------
void CHostState::Think(void) const
{
#ifndef CLIENT_DLL
	static bool bInitialized = false;
	static CFastTimer statsTimer;
	static CFastTimer banListTimer;
#ifdef DEDICATED
	static CFastTimer pylonTimer;
#endif // DEDICATED

	if (!bInitialized) // Initialize clocks.
	{
		statsTimer.Start();
		banListTimer.Start();
#ifdef DEDICATED
		pylonTimer.Start();
#endif // DEDICATED
		bInitialized = true;
	}

	HostState_HandleAutoReload();

	if (statsTimer.GetDurationInProgress().GetSeconds() > host_statusRefreshRate.GetFloat())
	{
		SetConsoleTitleA(Format("%s - %d/%d Players (%s on %s) - %d%% Server CPU (%.3f msec on frame %d)",
			hostname->GetString(), g_pServer->GetNumClients(),
			gpGlobals->maxClients, v_Playlists_GetCurrent(), m_levelName,
			static_cast<int>(g_pServer->GetCPUUsage() * 100.0f), (g_pEngine->GetFrameTime() * 1000.0f),
			g_pServer->GetTick()).c_str());

		statsTimer.Start();
	}
	if (sv_globalBanlist.GetBool() &&
		banListTimer.GetDurationInProgress().GetSeconds() > sv_banlistRefreshRate.GetFloat())
	{
		SV_CheckClientsForBan();
		banListTimer.Start();
	}
#ifdef DEDICATED
	if (pylonTimer.GetDurationInProgress().GetSeconds() > sv_pylonRefreshRate.GetFloat())
	{
		HostState_KeepAlive();
		pylonTimer.Start();
	}
#endif // DEDICATED
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: load and execute configuration files
//-----------------------------------------------------------------------------
void CHostState::LoadConfig(void) const
{
	if (CommandLine()->ParmValue("-launcher", 0) < 1) // Launcher level 1 indicates everything is handled from the commandline/launcher.
	{
		if (!CommandLine()->CheckParm("-devsdk"))
		{
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec system/autoexec.cfg\n", cmd_source_t::kCommandSrcCode);
#ifndef CLIENT_DLL
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec system/autoexec_server.cfg\n", cmd_source_t::kCommandSrcCode);
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec tools/rcon_server.cfg\n", cmd_source_t::kCommandSrcCode);
#endif //!CLIENT_DLL
#ifndef DEDICATED
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec system/autoexec_client.cfg\n", cmd_source_t::kCommandSrcCode);
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec tools/rcon_client.cfg\n", cmd_source_t::kCommandSrcCode);
#endif // !DEDICATED
		}
		else // Development configs.
		{
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec system/autoexec_dev.cfg\n", cmd_source_t::kCommandSrcCode);
#ifndef CLIENT_DLL
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec system/autoexec_server_dev.cfg\n", cmd_source_t::kCommandSrcCode);
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec tools/rcon_server_dev.cfg\n", cmd_source_t::kCommandSrcCode);
#endif //!CLIENT_DLL
#ifndef DEDICATED
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec system/autoexec_client_dev.cfg\n", cmd_source_t::kCommandSrcCode);
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec tools/rcon_client_dev.cfg\n", cmd_source_t::kCommandSrcCode);
#endif // !DEDICATED
		}
		if (CommandLine()->CheckParm("-offline"))
		{
#ifndef CLIENT_DLL
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec system/offline_server.cfg\n", cmd_source_t::kCommandSrcCode);
#endif //!CLIENT_DLL
#ifndef DEDICATED
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec system/offline_client.cfg\n", cmd_source_t::kCommandSrcCode);
#endif // !DEDICATED
		}
#ifndef CLIENT_DLL
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec liveapi.cfg\n", cmd_source_t::kCommandSrcCode);
#endif //!CLIENT_DLL
#ifndef DEDICATED
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec bind.cfg\n", cmd_source_t::kCommandSrcCode);
#endif // !DEDICATED
	}
}

//-----------------------------------------------------------------------------
// Purpose: set state machine
// Input  : newState  - 
//          clearNext - 
//-----------------------------------------------------------------------------
void CHostState::SetState(const HostStates_t newState)
{
	m_iCurrentState = newState;

	// If our next state isn't a shutdown, or its a forced shutdown then set
	// next state to run.
	if (m_iNextState != HostStates_t::HS_SHUTDOWN ||
		!host_hasIrreversibleShutdown->GetBool())
	{
		m_iNextState = newState;
	}
}

//-----------------------------------------------------------------------------
// Purpose: shutdown active game
//-----------------------------------------------------------------------------
void CHostState::GameShutDown(void)
{
	if (m_bActiveGame)
	{
#ifndef CLIENT_DLL
		g_pServerGameDLL->GameShutdown();
#endif // !CLIENT_DLL
		m_bActiveGame = false;
		ResetLevelName();
	}
}

//-----------------------------------------------------------------------------
// Purpose: initialize new game
//-----------------------------------------------------------------------------
void CHostState::State_NewGame(void)
{
	Msg(eDLL_T::ENGINE, "%s: Loading level: '%s'\n", __FUNCTION__, g_pHostState->m_levelName);

	LARGE_INTEGER time{};

#ifndef CLIENT_DLL
	const bool bSplitScreenConnect = m_bSplitScreenConnect;
	m_bSplitScreenConnect = false;

	if (!g_pServerGameClients) // Init Game if it ain't valid.
	{
		SV_InitGameDLL();
	}
#endif // !CLIENT_DLL

#ifndef CLIENT_DLL
	if (!CModelLoader__Map_IsValid(g_pModelLoader, m_levelName) // Check if map is valid and if we can start a new game.
		|| !v_Host_NewGame(m_levelName, nullptr, m_bBackgroundLevel, bSplitScreenConnect, time) || !g_pServerGameClients)
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "%s: Level not valid\n", __FUNCTION__);
#ifndef DEDICATED
		SCR_EndLoadingPlaque();
#endif // !DEDICATED
		GameShutDown();
	}
#endif // !CLIENT_DLL

	SetState(HostStates_t::HS_RUN);
}

//-----------------------------------------------------------------------------
// Purpose: change singleplayer level
//-----------------------------------------------------------------------------
void CHostState::State_ChangeLevelSP(void)
{
	Msg(eDLL_T::ENGINE, "%s: Changing singleplayer level to: '%s'\n", __FUNCTION__, m_levelName);
	m_flShortFrameTime = 1.5; // Set frame time.

	if (CModelLoader__Map_IsValid(g_pModelLoader, m_levelName)) // Check if map is valid and if we can start a new game.
	{
		v_Host_ChangeLevel(true, m_levelName, m_mapGroupName); // Call change level as singleplayer level.
	}
	else
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "%s: Unable to find level: '%s'\n", __FUNCTION__, m_levelName);
	}

	// Set current state to run.
	SetState(HostStates_t::HS_RUN);
}

//-----------------------------------------------------------------------------
// Purpose: change multiplayer level
//-----------------------------------------------------------------------------
void CHostState::State_ChangeLevelMP(void)
{
	Msg(eDLL_T::ENGINE, "%s: Changing multiplayer level to: '%s'\n", __FUNCTION__, m_levelName);
	m_flShortFrameTime = 0.5; // Set frame time.

#ifndef CLIENT_DLL
	g_pServerGameDLL->LevelShutdown();
#endif // !CLIENT_DLL
	if (CModelLoader__Map_IsValid(g_pModelLoader, m_levelName)) // Check if map is valid and if we can start a new game.
	{
#ifndef DEDICATED
		g_pEngineVGui->EnabledProgressBarForNextLoad();
#endif // !DEDICATED
		v_Host_ChangeLevel(false, m_levelName, m_mapGroupName); // Call change level as multiplayer level.
	}
	else
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "%s: Unable to find level: '%s'\n", __FUNCTION__, m_levelName);
	}

	// Set current state to run.
	SetState(HostStates_t::HS_RUN);
}

//-----------------------------------------------------------------------------
// Purpose: resets the level name
//-----------------------------------------------------------------------------
void CHostState::ResetLevelName(void)
{
	static const char* szNoMap = "no_map";
	Q_snprintf(const_cast<char*>(m_levelName), sizeof(m_levelName), "%s", szNoMap);
}

void VHostState::Detour(const bool bAttach) const
{
	DetourSetup(&CHostState__FrameUpdate, &CHostState::FrameUpdate, bAttach);
}

///////////////////////////////////////////////////////////////////////////////
CHostState* g_pHostState = nullptr;

#ifndef CLIENT_DLL
bool g_hostReloadState = false;
#endif // !CLIENT_DLL
