//=============================================================================//
// 
// Purpose: server host manager
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//
#include "tier0/frametask.h"
#include "common/callback.h"
#include "rtech/playlists/playlists.h"
#include "engine/cmd.h"
#include "hostmanager.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CServerHostManager::CServerHostManager(void)
    : m_HostingStatus(HostStatus_e::NOT_HOSTING)
    , m_ServerVisibility(ServerVisibility_e::OFFLINE)
{
}

//-----------------------------------------------------------------------------
// Purpose: internal server launch handler
//-----------------------------------------------------------------------------
static void HostManager_HandleCommandInternal(const char* const map, const char* const mode, const bool changeLevel)
{
    Assert(!ThreadInServerFrameThread(), "Use server script GameRules_ChangeMap() instead!");

    Msg(eDLL_T::ENGINE, "Starting server with name: \"%s\" map: \"%s\" mode: \"%s\"\n",
        hostname->GetString(), map, mode);

    bool hasPendingMap = *g_pPlaylistMapToLoad != '\0';

    // NOTE: when the provided playlist is the same as the one we're currently
    // on, and there's already a pending map load request,  the game will run 
    // "map <mapName>" in Playlists_Parse, where the map name is dictated by
    // g_pPlaylistMapToLoad. If changelevel was specified, we have to null the
    // requested map here as to prevent Playlists_Parse from running the map
    // command on it, as we are going to run the changelevel command anyways.
    // Not doing this will result in running both map and changelevel commands.
    if (changeLevel && hasPendingMap)
    {
        *g_pPlaylistMapToLoad = '\0';
        hasPendingMap = false;
    }

    const bool samePlaylist = v_Playlists_Parse(mode);
    char commandBuf[512];

    if (!samePlaylist || !hasPendingMap)
    {
        snprintf(commandBuf, sizeof(commandBuf), "%s %s\n", changeLevel ? "changelevel" : "map", map);
        Cbuf_AddText(Cbuf_GetCurrentPlayer(), commandBuf, cmd_source_t::kCommandSrcCode);
    }

    snprintf(commandBuf, sizeof(commandBuf), "mp_gamemode %s\n", mode);
    Cbuf_AddText(Cbuf_GetCurrentPlayer(), commandBuf, cmd_source_t::kCommandSrcCode);
}

//-----------------------------------------------------------------------------
// Purpose: Launch server with given parameters
//-----------------------------------------------------------------------------
void CServerHostManager::LaunchServer(const char* const map, const char* const mode) const
{
    HostManager_HandleCommandInternal(map, mode, false);
}

//-----------------------------------------------------------------------------
// Purpose: Change level with given parameters
//-----------------------------------------------------------------------------
void CServerHostManager::ChangeLevel(const char* const map, const char* const mode) const
{
    HostManager_HandleCommandInternal(map, mode, true);
}

CServerHostManager g_ServerHostManager;
