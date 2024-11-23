#pragma once

// Change this each time the settings format has changed.
#define SDK_LAUNCHER_VERSION 3

// Uncomment this line to compile the launcher for dedicated server builds.
//#define DEDI_LAUNCHER

#define GAME_CFG_PATH "platform\\cfg\\system\\"
#define DEFAULT_WINDOW_CLASS_NAME "Respawn001"
#define LAUNCHER_SETTING_FILE "launcher.vdf"

// Gigabytes.
// TODO: obtain these from the repo...
#define MIN_REQUIRED_DISK_SPACE 35
#define MIN_REQUIRED_DISK_SPACE_OPT 55 // Optional textures

#define QUERY_TIMEOUT 15
#define BASE_PLATFORM_DIR "platform\\"
#define DEFAULT_DEPOT_DOWNLOAD_DIR BASE_PLATFORM_DIR "depot\\"

// Files that need to be installed during launcher restart,
// have to go here!!
#define RESTART_DEPOT_DOWNLOAD_DIR DEFAULT_DEPOT_DOWNLOAD_DIR "temp\\"

#define DEPOT_MANIFEST_FILE "manifest_patch.json"
#define DEPOT_MANIFEST_FILE_PATH BASE_PLATFORM_DIR DEPOT_MANIFEST_FILE

// TODO: these should be obtained dynamically!!!
#define GAME_DEPOT_VENDOR "https://api.github.com/repos/SlaveBuild/N1094_CL456479/releases"
#define SDK_DEPOT_VENDOR "https://api.github.com/repos/Mauler125/r5sdk/releases"

#define SDK_LAUNCHER_LOG_DIR BASE_PLATFORM_DIR"logs\\tools\\sdk_launcher"

//-----------------------------------------------------------------------------
// Launch and inject specified dll based on launch mode
//-----------------------------------------------------------------------------
enum class eLaunchMode : int
{
    LM_NONE = -1,
    LM_GAME_DEV,
    LM_GAME,
    LM_SERVER_DEV,
    LM_SERVER,
    LM_CLIENT_DEV,
    LM_CLIENT,
};
