#ifndef GLOBAL_H
#define GLOBAL_H

//-------------------------------------------------------------------------
// ENGINE                                                                 |
extern ConVar* single_frame_shutdown_for_reload;
extern ConVar* old_gather_props;

extern ConVar* enable_debug_overlays;
extern ConVar* debug_draw_box_depth_test;

extern ConVar* developer;
extern ConVar* fps_max;
extern ConVar* fps_max_vsync;

#ifndef DEDICATED
extern ConVar* in_syncRT;
#endif // !DEDICATED

extern ConVar* base_tickinterval_sp;
extern ConVar* base_tickinterval_mp;

extern ConVar* staticProp_no_fade_scalar;
extern ConVar* staticProp_gather_size_weight;

extern ConVar* model_defaultFadeDistScale;
extern ConVar* model_defaultFadeDistMin;

extern ConVar* ip_cvar;
extern ConVar* hostname;
extern ConVar* hostip;
extern ConVar* hostport;

extern ConVar* host_hasIrreversibleShutdown;
extern ConVar* host_timescale;

extern ConVar* mp_gamemode;

#ifndef DEDICATED
extern ConVar* r_visualizetraces;
extern ConVar* r_visualizetraces_duration;

extern ConVar* stream_overlay;
extern ConVar* stream_overlay_mode;
extern ConVar* gpu_driven_tex_stream;
#endif // !DEDICATED
//-------------------------------------------------------------------------
// SHARED                                                                 |
extern ConVar* eula_version;
extern ConVar* eula_version_accepted;

extern ConVar* language_cvar;

extern ConVar* voice_noxplat;

extern ConVar* platform_user_id;

#ifndef DEDICATED
extern ConVar* name_cvar;
#endif // !DEDICATED

//-------------------------------------------------------------------------
// SERVER                                                                 |
#ifndef CLIENT_DLL
extern ConVar* ai_script_nodes_draw;

extern ConVar* sv_forceChatToTeamOnly;

extern ConVar* sv_single_core_dedi;

extern ConVar* sv_maxunlag;
extern ConVar* sv_lagpushticks;
extern ConVar* sv_clockcorrection_msecs;

extern ConVar* sv_updaterate_sp;
extern ConVar* sv_updaterate_mp;

extern ConVar* sv_showhitboxes;
extern ConVar* sv_stats;

extern ConVar* sv_voiceEcho;
extern ConVar* sv_voiceenable;
extern ConVar* sv_alltalk;

extern ConVar* sv_clampPlayerFrameTime;

extern ConVar* playerframetimekick_margin;
extern ConVar* playerframetimekick_decayrate;

extern ConVar* player_userCmdsQueueWarning;
extern ConVar* player_disallow_negative_frametime;

#endif // CLIENT_DLL
extern ConVar* sv_cheats;
extern ConVar* sv_visualizetraces;
extern ConVar* sv_visualizetraces_duration;
extern ConVar* bhit_enable;
//-------------------------------------------------------------------------
// CLIENT                                                                 |
#ifndef DEDICATED
extern ConVar* cl_threaded_bone_setup;

extern ConVar* origin_disconnectWhenOffline;
extern ConVar* discord_updatePresence;

extern ConVar* gamepad_custom_enabled;
extern ConVar* gamepad_custom_assist_on;
extern ConVar* gamepad_look_curve;
#endif // !DEDICATED
//-------------------------------------------------------------------------
// FILESYSTEM                                                             |
extern ConVar* fs_showAllReads;
//-------------------------------------------------------------------------
// NETCHANNEL                                                             |
extern ConVar* net_usesocketsforloopback;

extern ConVar* net_data_block_enabled;
extern ConVar* net_datablock_networkLossForSlowSpeed;
extern ConVar* net_compressDataBlock;

extern ConVar* net_showmsg;
extern ConVar* net_blockmsg;
extern ConVar* net_showpeaks;

extern ConVar ssl_verify_peer;
extern ConVar curl_timeout;
extern ConVar curl_debug;
//-------------------------------------------------------------------------
// RUI                                                                    |
#ifndef DEDICATED
extern ConVar* rui_defaultDebugFontFace;
#endif // !DEDICATED
//-------------------------------------------------------------------------
// MILES                                                                  |
#ifndef DEDICATED
extern ConVar* miles_language;
#endif

bool IsEULAUpToDate();

void ConVar_InitShipped(void);
void ConVar_PurgeShipped(void);
void ConVar_PurgeHostNames(void);
void ConCommand_InitShipped(void);
void ConCommand_PurgeShipped(void);

#endif // GLOBAL_H
