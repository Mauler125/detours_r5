
#include "core/stdafx.h"
#include "const.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "tier1/cmd.h"
#include "tier1/NetAdr.h"
#include "tier2/curlutils.h" // For initializing the curl cvars.
#include "completion.h"
#include "callback.h"
#include "global.h"


ConVar curl_debug("curl_debug", "0", FCVAR_DEVELOPMENTONLY, "Determines whether or not to enable curl debug logging.", "1 = curl logs; 0 (zero) = no logs");
ConVar curl_timeout("curl_timeout", "15", FCVAR_DEVELOPMENTONLY, "Maximum time in seconds a curl transfer operation could take.");
ConVar ssl_verify_peer("ssl_verify_peer", "1", FCVAR_DEVELOPMENTONLY, "Verify the authenticity of the peer's SSL certificate.", "1 = curl verifies; 0 (zero) = no verification");

//-----------------------------------------------------------------------------
// ENGINE                                                                     |
ConVar* single_frame_shutdown_for_reload   = nullptr;
ConVar* old_gather_props                   = nullptr;

ConVar* enable_debug_overlays              = nullptr;
ConVar* debug_draw_box_depth_test          = nullptr;

ConVar* developer                          = nullptr;
ConVar* fps_max                            = nullptr;
ConVar* fps_max_vsync                      = nullptr;

#ifndef DEDICATED
ConVar* in_syncRT                          = nullptr;
#endif // !DEDICATED

ConVar* base_tickinterval_sp               = nullptr;
ConVar* base_tickinterval_mp               = nullptr;

ConVar* staticProp_no_fade_scalar          = nullptr;
ConVar* staticProp_gather_size_weight      = nullptr;

ConVar* model_defaultFadeDistScale         = nullptr;
ConVar* model_defaultFadeDistMin           = nullptr;

ConVar* ip_cvar                            = nullptr;
ConVar* hostname                           = nullptr;
ConVar* hostip                             = nullptr;
ConVar* hostport                           = nullptr;

ConVar* host_hasIrreversibleShutdown       = nullptr;
ConVar* host_timescale                     = nullptr;

ConVar* mp_gamemode                        = nullptr;

#ifndef DEDICATED
ConVar* r_visualizetraces                  = nullptr;
ConVar* r_visualizetraces_duration         = nullptr;
#endif // !DEDICATED

ConVar* stream_overlay                     = nullptr;
ConVar* stream_overlay_mode                = nullptr;

ConVar* eula_version                       = nullptr;
ConVar* eula_version_accepted              = nullptr;

ConVar* language_cvar                      = nullptr;

ConVar* voice_noxplat                      = nullptr;

ConVar* platform_user_id                   = nullptr;

#ifndef DEDICATED
ConVar* name_cvar                          = nullptr;
#endif // !DEDICATED

//-----------------------------------------------------------------------------
// SERVER                                                                     |
#ifndef CLIENT_DLL
ConVar* ai_script_nodes_draw               = nullptr;

ConVar* sv_forceChatToTeamOnly             = nullptr;

ConVar* sv_single_core_dedi                = nullptr;

ConVar* sv_maxunlag                        = nullptr;
ConVar* sv_lagpushticks                    = nullptr;
ConVar* sv_clockcorrection_msecs           = nullptr;

ConVar* sv_updaterate_sp                   = nullptr;
ConVar* sv_updaterate_mp                   = nullptr;

ConVar* sv_showhitboxes                    = nullptr;
ConVar* sv_stats                           = nullptr;

ConVar* sv_voiceEcho                       = nullptr;
ConVar* sv_voiceenable                     = nullptr;
ConVar* sv_alltalk                         = nullptr;

ConVar* sv_clampPlayerFrameTime            = nullptr;

ConVar* playerframetimekick_margin         = nullptr;
ConVar* playerframetimekick_decayrate      = nullptr;

ConVar* player_userCmdsQueueWarning        = nullptr;
ConVar* player_disallow_negative_frametime = nullptr;

#endif // !CLIENT_DLL
ConVar* sv_cheats                          = nullptr;
ConVar* sv_visualizetraces                 = nullptr;
ConVar* sv_visualizetraces_duration        = nullptr;
ConVar* bhit_enable                        = nullptr;
//-----------------------------------------------------------------------------
// CLIENT                                                                     |
#ifndef DEDICATED
ConVar* cl_updaterate_mp                   = nullptr;

ConVar* cl_threaded_bone_setup             = nullptr;

ConVar* origin_disconnectWhenOffline       = nullptr;
ConVar* discord_updatePresence = nullptr;

ConVar* gamepad_custom_enabled             = nullptr;
ConVar* gamepad_custom_assist_on           = nullptr;
ConVar* gamepad_look_curve                 = nullptr;
#endif // !DEDICATED
//-----------------------------------------------------------------------------
// FILESYSTEM                                                                 |
ConVar* fs_showAllReads                    = nullptr;
//-----------------------------------------------------------------------------
// NETCHANNEL                                                                 |
ConVar* net_usesocketsforloopback;
ConVar* net_data_block_enabled             = nullptr;
ConVar* net_datablock_networkLossForSlowSpeed = nullptr;
ConVar* net_compressDataBlock              = nullptr;

ConVar* net_showmsg                        = nullptr;
ConVar* net_blockmsg                       = nullptr;
ConVar* net_showpeaks                      = nullptr;
//-----------------------------------------------------------------------------
// RUI                                                                        |
#ifndef DEDICATED
ConVar* rui_defaultDebugFontFace           = nullptr;
#endif // !DEDICATED
//-----------------------------------------------------------------------------
// MILES                                                                      |
#ifndef DEDICATED
ConVar* miles_language                     = nullptr;
#endif

//-----------------------------------------------------------------------------
// Purpose: initialize shipped ConVar's
//-----------------------------------------------------------------------------
void ConVar_InitShipped(void)
{
#ifndef CLIENT_DLL
	ai_script_nodes_draw             = g_pCVar->FindVar("ai_script_nodes_draw");
	bhit_enable                      = g_pCVar->FindVar("bhit_enable");
#endif // !CLIENT_DLL
	developer                        = g_pCVar->FindVar("developer");
	fps_max                          = g_pCVar->FindVar("fps_max");
	fps_max_vsync                    = g_pCVar->FindVar("fps_max_vsync");
	base_tickinterval_sp             = g_pCVar->FindVar("base_tickinterval_sp");
	base_tickinterval_mp             = g_pCVar->FindVar("base_tickinterval_mp");
	fs_showAllReads                  = g_pCVar->FindVar("fs_showAllReads");

	eula_version                     = g_pCVar->FindVar("eula_version");
	eula_version_accepted            = g_pCVar->FindVar("eula_version_accepted");

	language_cvar                    = g_pCVar->FindVar("language");
	voice_noxplat                    = g_pCVar->FindVar("voice_noxplat");
	platform_user_id                 = g_pCVar->FindVar("platform_user_id");
#ifndef DEDICATED
	name_cvar                        = g_pCVar->FindVar("name");
	cl_updaterate_mp                 = g_pCVar->FindVar("cl_updaterate_mp");
	cl_threaded_bone_setup           = g_pCVar->FindVar("cl_threaded_bone_setup");
#endif // !DEDICATED
	single_frame_shutdown_for_reload = g_pCVar->FindVar("single_frame_shutdown_for_reload");
	enable_debug_overlays            = g_pCVar->FindVar("enable_debug_overlays");
	debug_draw_box_depth_test        = g_pCVar->FindVar("debug_draw_box_depth_test");
	model_defaultFadeDistScale       = g_pCVar->FindVar("model_defaultFadeDistScale");
	model_defaultFadeDistMin         = g_pCVar->FindVar("model_defaultFadeDistMin");
#ifndef DEDICATED
	miles_language                   = g_pCVar->FindVar("miles_language");
	rui_defaultDebugFontFace         = g_pCVar->FindVar("rui_defaultDebugFontFace");
	in_syncRT                        = g_pCVar->FindVar("in_syncRT");
	r_visualizetraces                = g_pCVar->FindVar("r_visualizetraces");
	r_visualizetraces_duration       = g_pCVar->FindVar("r_visualizetraces_duration");
#endif // !DEDICATED
	staticProp_no_fade_scalar        = g_pCVar->FindVar("staticProp_no_fade_scalar");
	staticProp_gather_size_weight    = g_pCVar->FindVar("staticProp_gather_size_weight");
	stream_overlay                   = g_pCVar->FindVar("stream_overlay");
	stream_overlay_mode              = g_pCVar->FindVar("stream_overlay_mode");
	sv_cheats                        = g_pCVar->FindVar("sv_cheats");
	sv_visualizetraces               = g_pCVar->FindVar("sv_visualizetraces");
	sv_visualizetraces_duration      = g_pCVar->FindVar("sv_visualizetraces_duration");
	old_gather_props                 = g_pCVar->FindVar("old_gather_props");
#ifndef DEDICATED
	origin_disconnectWhenOffline     = g_pCVar->FindVar("origin_disconnectWhenOffline");
	discord_updatePresence           = g_pCVar->FindVar("discord_updatePresence");

	gamepad_custom_enabled           = g_pCVar->FindVar("gamepad_custom_enabled");
	gamepad_custom_assist_on         = g_pCVar->FindVar("gamepad_custom_assist_on");
	gamepad_look_curve               = g_pCVar->FindVar("gamepad_look_curve");
#endif // !DEDICATED
	mp_gamemode                      = g_pCVar->FindVar("mp_gamemode");
	ip_cvar                          = g_pCVar->FindVar("ip");
	hostname                         = g_pCVar->FindVar("hostname");
	hostip                           = g_pCVar->FindVar("hostip");
	hostport                         = g_pCVar->FindVar("hostport");
	host_hasIrreversibleShutdown     = g_pCVar->FindVar("host_hasIrreversibleShutdown");
	host_timescale                   = g_pCVar->FindVar("host_timescale");

	net_data_block_enabled           = g_pCVar->FindVar("net_data_block_enabled");
	net_compressDataBlock            = g_pCVar->FindVar("net_compressDataBlock");
	net_datablock_networkLossForSlowSpeed = g_pCVar->FindVar("net_datablock_networkLossForSlowSpeed");

	net_usesocketsforloopback        = g_pCVar->FindVar("net_usesocketsforloopback");

	net_showmsg = g_pCVar->FindVar("net_showmsg");
	net_blockmsg = g_pCVar->FindVar("net_blockmsg");
	net_showpeaks = g_pCVar->FindVar("net_showpeaks");
#ifndef CLIENT_DLL
	sv_stats = g_pCVar->FindVar("sv_stats");

	sv_maxunlag = g_pCVar->FindVar("sv_maxunlag");
	sv_lagpushticks = g_pCVar->FindVar("sv_lagpushticks");
	sv_clockcorrection_msecs = g_pCVar->FindVar("sv_clockcorrection_msecs");

	sv_updaterate_sp = g_pCVar->FindVar("sv_updaterate_sp");
	sv_updaterate_mp = g_pCVar->FindVar("sv_updaterate_mp");

	sv_showhitboxes = g_pCVar->FindVar("sv_showhitboxes");
	sv_forceChatToTeamOnly = g_pCVar->FindVar("sv_forceChatToTeamOnly");

	sv_single_core_dedi = g_pCVar->FindVar("sv_single_core_dedi");

	sv_voiceenable = g_pCVar->FindVar("sv_voiceenable");
	sv_voiceEcho = g_pCVar->FindVar("sv_voiceEcho");
	sv_alltalk = g_pCVar->FindVar("sv_alltalk");

	sv_clampPlayerFrameTime = g_pCVar->FindVar("sv_clampPlayerFrameTime");

	playerframetimekick_margin = g_pCVar->FindVar("playerframetimekick_margin");
	playerframetimekick_decayrate = g_pCVar->FindVar("playerframetimekick_decayrate");

	player_userCmdsQueueWarning = g_pCVar->FindVar("player_userCmdsQueueWarning");
	player_disallow_negative_frametime = g_pCVar->FindVar("player_disallow_negative_frametime");

	sv_updaterate_sp->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	sv_updaterate_mp->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	sv_showhitboxes->SetMin(-1); // Allow user to go over each entity manually without going out of bounds.
	sv_showhitboxes->SetMax(NUM_ENT_ENTRIES - 1);

	sv_forceChatToTeamOnly->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	sv_forceChatToTeamOnly->AddFlags(FCVAR_REPLICATED);

	sv_single_core_dedi->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	ai_script_nodes_draw->SetValue(-1);
	bhit_enable->SetValue(0);
#endif // !CLIENT_DLL
#ifndef DEDICATED
	cl_updaterate_mp->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	cl_threaded_bone_setup->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	rui_defaultDebugFontFace->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	origin_disconnectWhenOffline->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	discord_updatePresence->RemoveFlags(FCVAR_DEVELOPMENTONLY);
#endif // !DEDICATED
	fps_max->AddFlags(FCVAR_ARCHIVE);
	fps_max_vsync->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	base_tickinterval_sp->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	base_tickinterval_mp->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	mp_gamemode->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	// The base callback is for client builds only, must be replaced with the
	// dedicated server variant as the original one runs this through
	// CEngineClient::SetupGamemode(). The callback is effectively the same
	// with the exception that it calls SetupGamemode directly, and not
	// through an interface like CEngineClient.
	mp_gamemode->RemoveChangeCallback(mp_gamemode->GetChangeCallback(0), 0);
	mp_gamemode->InstallChangeCallback(MP_GameMode_Changed_f, nullptr, false);

	net_usesocketsforloopback->RemoveFlags(FCVAR_DEVELOPMENTONLY);
#ifndef DEDICATED
	language_cvar->InstallChangeCallback(LanguageChanged_f, nullptr, false);
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: unregister/disable extraneous ConVar's.
//-----------------------------------------------------------------------------
void ConVar_PurgeShipped(void)
{
#ifdef DEDICATED
	const char* pszToPurge[] =
	{
		"bink_materials_enabled",
		"communities_enabled",
		"community_frame_run",
		"ime_enabled",
		"origin_igo_mutes_sound_enabled",
		"twitch_shouldQuery",
		"voice_enabled",
	};

	for (size_t i = 0; i < SDK_ARRAYSIZE(pszToPurge); i++)
	{
		if (ConVar* pCVar = g_pCVar->FindVar(pszToPurge[i]))
		{
			pCVar->SetValue(0);
		}
	}
#endif // DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: clear all hostname ConVar's.
//-----------------------------------------------------------------------------
void ConVar_PurgeHostNames(void)
{
	const char* pszHostNames[] =
	{
		"assetdownloads_hostname",
		"communities_hostname",
		"matchmaking_hostname",
		"party_hostname",
		"persistence_hostname",
		"persistenceDef_hostname",
		"pin_telemetry_hostname",
		"publication_hostname",
		"serverReports_hostname",
		"skill_hostname",
		"speechtotext_hostname",
		"staticfile_hostname",
		"stats_hostname",
		"steamlink_hostname",
		"subscription_hostname",
		"users_hostname"
	};

	for (size_t i = 0; i < SDK_ARRAYSIZE(pszHostNames); i++)
	{
		if (ConVar* pCVar = g_pCVar->FindVar(pszHostNames[i]))
		{
			pCVar->SetValue(NET_IPV4_UNSPEC);
		}
	}
}

static ConCommand bhit("bhit", BHit_f, "Bullet-hit trajectory debug", FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL);

#ifndef DEDICATED
static ConCommand line("line", Line_f, "Draw a debug line", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
static ConCommand sphere("sphere", Sphere_f, "Draw a debug sphere", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
static ConCommand capsule("capsule", Capsule_f, "Draw a debug capsule", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
#endif //!DEDICATED

// TODO: move VPK building code to separate file and place this in 'packedstore.cpp'
static ConCommand fs_vpk_mount("fs_vpk_mount", VPK_Mount_f, "Mount a VPK file for FileSystem usage", FCVAR_DEVELOPMENTONLY);
static ConCommand fs_vpk_unmount("fs_vpk_unmount", VPK_Unmount_f, "Unmount a VPK file and clear its cache", FCVAR_DEVELOPMENTONLY);
static ConCommand fs_vpk_pack("fs_vpk_pack", VPK_Pack_f, "Pack a VPK file from current workspace", FCVAR_DEVELOPMENTONLY);
static ConCommand fs_vpk_unpack("fs_vpk_unpack", VPK_Unpack_f, "Unpack all files from a VPK file", FCVAR_DEVELOPMENTONLY);

//-----------------------------------------------------------------------------
// Purpose: shipped ConCommand initialization
//-----------------------------------------------------------------------------
void ConCommand_InitShipped(void)
{
	///------------------------------------------------------ [ CALLBACK SWAP ]
	//-------------------------------------------------------------------------
	// ENGINE DLL                                                             |
	ConCommand* changelevel = g_pCVar->FindCommand("changelevel");
	ConCommand* map = g_pCVar->FindCommand("map");
	ConCommand* map_background = g_pCVar->FindCommand("map_background");
	ConCommand* ss_map = g_pCVar->FindCommand("ss_map");
	ConCommand* migrateme = g_pCVar->FindCommand("migrateme");
	ConCommand* help = g_pCVar->FindCommand("help");
	ConCommand* convar_list = g_pCVar->FindCommand("convar_list");
	ConCommand* convar_differences = g_pCVar->FindCommand("convar_differences");
	ConCommand* convar_findByFlags = g_pCVar->FindCommand("convar_findByFlags");
#ifndef DEDICATED
	//-------------------------------------------------------------------------
	// MATERIAL SYSTEM
	ConCommand* mat_crosshair = g_pCVar->FindCommand("mat_crosshair"); // Patch callback function to working callback.
	//-------------------------------------------------------------------------
	// CLIENT DLL                                                             |
	ConCommand* give = g_pCVar->FindCommand("give");
	ConCommand* set = g_pCVar->FindCommand("set");
#endif // !DEDICATED

	help->m_fnCommandCallback = CVHelp_f;
	convar_list->m_fnCommandCallback = CVList_f;
	convar_differences->m_fnCommandCallback = CVDiff_f;
	convar_findByFlags->m_fnCommandCallback = CVFlag_f;
#ifndef CLIENT_DLL
	changelevel->m_fnCommandCallback = Host_Changelevel_f;
#endif // !CLIENT_DLL
	changelevel->m_fnCompletionCallback = Host_Changelevel_f_CompletionFunc;

	map->m_fnCompletionCallback = Host_Map_f_CompletionFunc;
	map_background->m_fnCompletionCallback = Host_Background_f_CompletionFunc;
	ss_map->m_fnCompletionCallback = Host_SSMap_f_CompletionFunc;

#ifndef DEDICATED
	mat_crosshair->m_fnCommandCallback = Mat_CrossHair_f;
	give->m_fnCompletionCallback = Game_Give_f_CompletionFunc;
	set->m_fnCommandCallback = Set_f;
#endif // !DEDICATED

	/// ------------------------------------------------------ [ FLAG REMOVAL ]
	//-------------------------------------------------------------------------
	if (!CommandLine()->CheckParm("-devsdk"))
	{
		const char* pszMaskedBases[] =
		{
#ifndef DEDICATED
			"connect",
			"connectAsSpectator",
			"connectWithKey",
			"silentconnect",
			"ping",
#endif // !DEDICATED
			"launchplaylist",
			"quit",
			"exit",
			"reload",
			"restart",
			"set",
			"status",
			"version",
		};

		for (size_t i = 0; i < SDK_ARRAYSIZE(pszMaskedBases); i++)
		{
			if (ConCommandBase* pCommandBase = g_pCVar->FindCommandBase(pszMaskedBases[i]))
			{
				pCommandBase->RemoveFlags(FCVAR_DEVELOPMENTONLY);
			}
		}

		convar_list->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		convar_differences->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		convar_findByFlags->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		help->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		migrateme->RemoveFlags(FCVAR_SERVER_CAN_EXECUTE);
		changelevel->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		map->RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_SERVER_CAN_EXECUTE);
		map_background->RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_SERVER_CAN_EXECUTE);
		ss_map->RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_SERVER_CAN_EXECUTE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: unregister extraneous ConCommand's.
//-----------------------------------------------------------------------------
void ConCommand_PurgeShipped(void)
{
#ifdef DEDICATED
	const char* pszCommandToRemove[] =
	{
		"bind",
		"bind_held",
		"bind_list",
		"bind_list_abilities",
		"bind_US_standard",
		"bind_held_US_standard",
		"unbind",
		"unbind_US_standard",
		"unbindall",
		"unbind_all_gamepad",
		"unbindall_ignoreGamepad",
		"unbind_batch",
		"unbind_held",
		"unbind_held_US_standard",
		"uiscript_reset",
		"getpos_bind",
		"connect",
		"silent_connect",
		"set",
		"ping",
		"gameui_activate",
		"gameui_hide",
		"weaponSelectOrdnance",
		"weaponSelectPrimary0",
		"weaponSelectPrimary1",
		"weaponSelectPrimary2",
		"+scriptCommand1",
		"-scriptCommand1",
		"+scriptCommand2",
		"-scriptCommand2",
		"+scriptCommand3",
		"-scriptCommand3",
		"+scriptCommand4",
		"-scriptCommand4",
		"+scriptCommand5",
		"-scriptCommand5",
		"+scriptCommand6",
		"-scriptCommand6",
		"+scriptCommand7",
		"-scriptCommand7",
		"+scriptCommand8",
		"-scriptCommand8",
		"+scriptCommand9",
		"-scriptCommand9",
	};

	for (size_t i = 0; i < SDK_ARRAYSIZE(pszCommandToRemove); i++)
	{
		ConCommandBase* pCommandBase = g_pCVar->FindCommandBase(pszCommandToRemove[i]);

		if (pCommandBase)
		{
			g_pCVar->UnregisterConCommand(pCommandBase);
		}
	}
#endif // DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: checks if the accepted EULA is up to date.
// Output : true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsEULAUpToDate()
{
#ifdef DEDICATED
	// Users of the dedicated servers are by default agreeing to the EULA
	// when using it. They can use it offline (not using the r5reloaded
	// master servers) to not accept it. The challenge of implementing
	// something that allows the dedi operator to agree to the EULA upon
	// launch is that it will break the automation of spinning up dedi
	// instances.
	return true;
#else
	return (eula_version_accepted->GetInt() == eula_version->GetInt());
#endif // DEDICATED
}
