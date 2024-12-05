/******************************************************************************
-------------------------------------------------------------------------------
File   : IBrowser.cpp
Date   : 09:06:2021
Author : Sal
Purpose: Implements the in-game server browser front-end
-------------------------------------------------------------------------------
History:
- 09:06:2021   21:07 : Created by Sal
- 25:07:2021   14:26 : Implement private servers connect dialog and password field

******************************************************************************/

#include "core/stdafx.h"
#include "core/init.h"
#include "core/resource.h"
#include "tier0/fasttimer.h"
#include "tier0/frametask.h"
#include "tier0/commandline.h"
#include "windows/id3dx.h"
#include "windows/console.h"
#include "windows/resource.h"
#include "engine/net.h"
#include "engine/cmd.h"
#include "engine/cmodel_bsp.h"
#include "engine/host_state.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // CLIENT_DLL
#include "engine/client/clientstate.h"
#include "networksystem/serverlisting.h"
#include "networksystem/pylon.h"
#include "networksystem/hostmanager.h"
#include "networksystem/listmanager.h"
#include "rtech/playlists/playlists.h"
#include "common/callback.h"
#include "gameui/IBrowser.h"
#include "public/edict.h"
#include "game/shared/vscript_shared.h"
#include "game/server/gameinterface.h"

static ConCommand togglebrowser("togglebrowser", CBrowser::ToggleBrowser_f, "Show/hide the server browser", FCVAR_CLIENTDLL | FCVAR_RELEASE);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBrowser::CBrowser(void)
    : m_reclaimFocusOnTokenField(false)
    , m_queryNewListNonRecursive(false)
    , m_queryGlobalBanList(true)
    , m_hostMessageColor(1.00f, 1.00f, 1.00f, 1.00f)
    , m_hiddenServerMessageColor(0.00f, 1.00f, 0.00f, 1.00f)
{
    m_surfaceLabel = "Server Browser";

    memset(m_serverTokenTextBuf, '\0', sizeof(m_serverTokenTextBuf));
    memset(m_serverAddressTextBuf, '\0', sizeof(m_serverAddressTextBuf));
    memset(m_serverNetKeyTextBuf, '\0', sizeof(m_serverNetKeyTextBuf));

    m_lockedIconDataResource = GetModuleResource(IDB_PNG2);

    m_levelName = "mp_lobby";
    m_gameMode = "dev_default";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBrowser::~CBrowser(void)
{
    Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBrowser::Init(void)
{
    SetStyleVar(927.f, 524.f, -500.f, 50.f);

    bool ret = LoadTextureBuffer(reinterpret_cast<unsigned char*>(m_lockedIconDataResource.m_pData), int(m_lockedIconDataResource.m_nSize),
        &m_lockedIconShaderResource, &m_lockedIconDataResource.m_nWidth, &m_lockedIconDataResource.m_nHeight);

    IM_ASSERT(ret && m_lockedIconShaderResource);
    return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBrowser::Shutdown(void)
{
    if (m_lockedIconShaderResource)
    {
        m_lockedIconShaderResource->Release();
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the main browser front-end
//-----------------------------------------------------------------------------
void CBrowser::RunFrame(void)
{
    // Uncomment these when adjusting the theme or layout.
    {
        //ImGui::ShowStyleEditor();
        //ImGui::ShowDemoWindow();
    }

    if (!m_initialized)
    {
        Init();
        m_initialized = true;
    }

    Animate();
    RunTask();

    int baseWindowStyleVars = 0;
    ImVec2 minBaseWindowRect;

    if (m_surfaceStyle == ImGuiStyle_t::MODERN)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 8.f, 10.f }); baseWindowStyleVars++;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha);                 baseWindowStyleVars++;

        minBaseWindowRect = ImVec2(621.f, 532.f);
    }
    else
    {
        minBaseWindowRect = m_surfaceStyle == ImGuiStyle_t::LEGACY
            ? ImVec2(619.f, 526.f)
            : ImVec2(618.f, 524.f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 6.f, 6.f });  baseWindowStyleVars++;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha);                 baseWindowStyleVars++;

        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);              baseWindowStyleVars++;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, minBaseWindowRect);       baseWindowStyleVars++;

    if (m_activated && m_reclaimFocus) // Reclaim focus on window apparition.
    {
        ImGui::SetNextWindowFocus();
        m_reclaimFocus = false;
    }

    DrawSurface();
    ImGui::PopStyleVar(baseWindowStyleVars);
}

//-----------------------------------------------------------------------------
// Purpose: runs tasks for the browser while not being drawn 
//-----------------------------------------------------------------------------
void CBrowser::RunTask()
{
    static bool bInitialized = false;
    static CFastTimer timer;

    if (!bInitialized)
    {
        timer.Start();
        bInitialized = true;
    }

    if (timer.GetDurationInProgress().GetSeconds() > pylon_host_update_interval.GetFloat())
    {
        UpdateHostingStatus();
        timer.Start();
    }

    if (m_activated)
    {
        if (m_queryNewListNonRecursive)
        {
            m_queryNewListNonRecursive = false;
            RefreshServerList();
        }
    }
    else // Refresh server list the next time 'm_activated' evaluates to true.
    {
        m_reclaimFocusOnTokenField = true;
        m_queryNewListNonRecursive = true;
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the server browser's main surface
// Output : true if a frame has been drawn, false otherwise
//-----------------------------------------------------------------------------
bool CBrowser::DrawSurface(void)
{
    if (!ImGui::Begin(m_surfaceLabel, &m_activated, ImGuiWindowFlags_None, &ResetInput))
    {
        ImGui::End();
        return false;
    }

    ImGui::BeginTabBar("CompMenu");
    if (ImGui::BeginTabItem("Browsing"))
    {
        DrawBrowserPanel();
        ImGui::EndTabItem();
    }
#ifndef CLIENT_DLL
    if (ImGui::BeginTabItem("Hosting"))
    {
        DrawHostPanel();
        ImGui::EndTabItem();
    }
#endif // !CLIENT_DLL

    ImGui::EndTabBar();
    ImGui::End();

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: draws the server browser section
//-----------------------------------------------------------------------------
void CBrowser::DrawBrowserPanel(void)
{
    ImGui::BeginGroup();
    m_serverBrowserTextFilter.Draw();
    ImGui::SameLine();

    if (ImGui::Button("Refresh"))
    {
        m_serverListMessage.clear();
        RefreshServerList();
    }

    ImGui::EndGroup();
    ImGui::TextColored(ImVec4(1.00f, 0.00f, 0.00f, 1.00f), "%s", m_serverListMessage.c_str());
    ImGui::Separator();

    int frameStyleVars = 0; // Eliminate borders around server list table.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 1.f, 0.f });  frameStyleVars++;

    const float fFooterHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

    if (ImGui::BeginTable("##ServerBrowser_ServerListTable", 6, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, { 0, -fFooterHeight }))
    {
        if (m_surfaceStyle == ImGuiStyle_t::MODERN)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8.f, 0.f }); frameStyleVars++;
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4.f, 0.f }); frameStyleVars++;
        }

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 25);
        ImGui::TableSetupColumn("Map", ImGuiTableColumnFlags_WidthStretch, 20);
        ImGui::TableSetupColumn("Playlist", ImGuiTableColumnFlags_WidthStretch, 10);
        ImGui::TableSetupColumn("Players", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableSetupColumn("Port", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 5);

        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        g_ServerListManager.m_Mutex.Lock();
        vector<const NetGameServer_t*> filteredServers;

        // Filter the server list first before running it over the ImGui list
        // clipper, if we do this within the clipper, clipper.Step() will fail
        // as the calculation for the remainder will be off.
        for (int i = 0; i < g_ServerListManager.m_vServerList.size(); i++)
        {
            const NetGameServer_t& server = g_ServerListManager.m_vServerList[i];

            const char* pszHostName = server.name.c_str();
            const char* pszHostMap = server.map.c_str();
            const char* pszPlaylist = server.playlist.c_str();

            if (m_serverBrowserTextFilter.PassFilter(pszHostName)
                || m_serverBrowserTextFilter.PassFilter(pszHostMap)
                || m_serverBrowserTextFilter.PassFilter(pszPlaylist))
            {
                filteredServers.push_back(&server);
            }
        }

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(filteredServers.size()));

        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                const NetGameServer_t* const server = filteredServers[i];

                const char* pszHostName = server->name.c_str();
                const char* pszHostMap = server->map.c_str();
                const char* pszPlaylist = server->playlist.c_str();

                char pszHostPort[32];
                sprintf(pszHostPort, "%d", server->port);

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszHostName);

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszHostMap);

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszPlaylist);

                ImGui::TableNextColumn();
                ImGui::Text("%s", Format("%3d/%3d", server->numPlayers, server->maxPlayers).c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszHostPort);

                ImGui::TableNextColumn();
                string svConnectBtn = "Connect##";
                svConnectBtn.append(server->name + server->address + server->map);

                if (ImGui::Button(svConnectBtn.c_str()))
                {
                    g_ServerListManager.ConnectToServer(server->address, server->port, server->netKey);
                }
            }
        }

        filteredServers.clear();
        g_ServerListManager.m_Mutex.Unlock();

        ImGui::PopStyleVar(frameStyleVars);
        ImGui::EndTable();
    }
    else
    {
        ImGui::PopStyleVar(frameStyleVars);
    }

    ImGui::Separator();

    const ImVec2 regionAvail = ImGui::GetContentRegionAvail();
    const ImGuiStyle& style = ImGui::GetStyle();

    // 4 elements means 3 spacings between items, this has to be subtracted
    // from the remaining available region to get correct results on all
    // window padding values!!!
    const float itemWidth = (regionAvail.x - (3 * style.ItemSpacing.x)) / 4;

    ImGui::PushItemWidth(itemWidth);
    {
        ImGui::InputTextWithHint("##ServerBrowser_ServerCon", "Server address and port", m_serverAddressTextBuf, sizeof(m_serverAddressTextBuf));

        ImGui::SameLine();
        ImGui::InputTextWithHint("##ServerBrowser_ServerKey", "Encryption key", m_serverNetKeyTextBuf, sizeof(m_serverNetKeyTextBuf));

        ImGui::SameLine();
        if (ImGui::Button("Connect", ImVec2(itemWidth, ImGui::GetFrameHeight())))
        {
            if (m_serverAddressTextBuf[0])
            {
                g_ServerListManager.ConnectToServer(m_serverAddressTextBuf, m_serverNetKeyTextBuf);
            }
        }

        ImGui::SameLine();

        // NOTE: -9 to prevent the last button from clipping/colliding with the
        // window drag handle! -9 makes the distance between the handle and the
        // last button equal as that of the developer console.
        if (ImGui::Button("Private servers", ImVec2(itemWidth - 9, ImGui::GetFrameHeight())))
        {
            ImGui::OpenPopup("Private Server");
        }

        HiddenServersModal();
    }

    ImGui::PopItemWidth();
}

//-----------------------------------------------------------------------------
// Purpose: refreshes the server browser list with available servers
//-----------------------------------------------------------------------------
void CBrowser::RefreshServerList(void)
{
    Msg(eDLL_T::CLIENT, "Refreshing server list with matchmaking host '%s'\n", pylon_matchmaking_hostname.GetString());

    // Thread the request, and let the main thread assign status message back
    std::thread request([&]
        {
            std::string serverListMessage;
            size_t numServers;
            g_ServerListManager.RefreshServerList(serverListMessage, numServers);

            g_TaskQueue.Dispatch([&, serverListMessage]
                {
                    SetServerListMessage(serverListMessage.c_str());
                }, 0);
        }
    );

    request.detach();
}

//-----------------------------------------------------------------------------
// Purpose: draws the hidden private server modal
//-----------------------------------------------------------------------------
void CBrowser::HiddenServersModal(void)
{
    float modalWindowHeight; // Set the padding accordingly for each theme.
    switch (m_surfaceStyle)
    {
    case ImGuiStyle_t::LEGACY:
        modalWindowHeight = 207.f;
        break;
    case ImGuiStyle_t::MODERN:
        modalWindowHeight = 214.f;
        break;
    default:
        modalWindowHeight = 206.f;
        break;
    }

    int modalStyleVars = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(408.f, modalWindowHeight));    modalStyleVars++;

    bool isModalStillOpen = true;
    if (ImGui::BeginPopupModal("Private Server", &isModalStillOpen, ImGuiWindowFlags_NoResize))
    {
        ImGui::SetWindowSize(ImVec2(408.f, modalWindowHeight), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f)); // Override the style color for child bg.

        ImGui::BeginChild("##HiddenServersConnectModal_IconParent", ImVec2(float(m_lockedIconDataResource.m_nWidth), float(m_lockedIconDataResource.m_nHeight)));
        ImGui::Image(m_lockedIconShaderResource, ImVec2(float(m_lockedIconDataResource.m_nWidth), float(m_lockedIconDataResource.m_nHeight))); // Display texture.
        ImGui::EndChild();

        ImGui::PopStyleColor(); // Pop the override for the child bg.

        ImGui::SameLine();
        ImGui::Text("Enter token to connect");

        const ImVec2 contentRegionMax = ImGui::GetContentRegionAvail();
        ImGui::PushItemWidth(contentRegionMax.x); // Override item width.

        const bool hitEnter = ImGui::InputTextWithHint("##HiddenServersConnectModal_TokenInput", "Token (required)", 
            m_serverTokenTextBuf, sizeof(m_serverTokenTextBuf), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::PopItemWidth();

        if (m_reclaimFocusOnTokenField)
        {
            ImGui::SetKeyboardFocusHere(-1); // -1 means previous widget.
            m_reclaimFocusOnTokenField = false;
        }

        ImGui::Dummy(ImVec2(contentRegionMax.x, 19.f)); // Place a dummy, basically making space inserting a blank element.

        ImGui::TextColored(m_hiddenServerMessageColor, "%s", m_hiddenServerRequestMessage.c_str());
        ImGui::Separator();

        if (ImGui::Button("Connect", ImVec2(contentRegionMax.x, 24)) || hitEnter)
        {
            m_hiddenServerRequestMessage.clear();
            m_reclaimFocusOnTokenField = true;

            if (m_serverTokenTextBuf[0])
            {
                NetGameServer_t server;
                const bool result = g_MasterServer.GetServerByToken(server, m_hiddenServerRequestMessage, m_serverTokenTextBuf); // Send token connect request.

                if (result && !server.name.empty())
                {
                    g_ServerListManager.ConnectToServer(server.address, server.port, server.netKey); // Connect to the server
                    m_hiddenServerRequestMessage = Format("Found server: %s", server.name.c_str());
                    m_hiddenServerMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    if (m_hiddenServerRequestMessage.empty())
                    {
                        m_hiddenServerRequestMessage = "Unknown error.";
                    }
                    else // Display error message.
                    {
                        m_hiddenServerRequestMessage = Format("Error: %s", m_hiddenServerRequestMessage.c_str());
                    }
                    m_hiddenServerMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
            }
            else
            {
                m_hiddenServerRequestMessage = "Token is required.";
                m_hiddenServerMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            }
        }

        if (ImGui::Button("Close", ImVec2(contentRegionMax.x, 24)))
        {
            m_hiddenServerRequestMessage.clear();
            m_reclaimFocusOnTokenField = true;

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
        ImGui::PopStyleVar(modalStyleVars);
    }
    else if (!isModalStillOpen)
    {
        m_hiddenServerRequestMessage.clear();
        m_reclaimFocusOnTokenField = true;

        ImGui::PopStyleVar(modalStyleVars);
    }
    else
    {
        ImGui::PopStyleVar(modalStyleVars);
    }
}

void CBrowser::HandleInvalidFields(const bool offline)
{
    if (!offline && m_serverName.empty())
    {
        m_hostMessage = "Server name is required.";
        m_hostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    }
    else if (m_gameMode.empty())
    {
        m_hostMessage = "Game mode is required.";
        m_hostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    }
    else if (m_levelName.empty())
    {
        m_hostMessage = "Level name is required.";
        m_hostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the host section
//-----------------------------------------------------------------------------
void CBrowser::DrawHostPanel(void)
{
#ifndef CLIENT_DLL
    if (ImGui::InputTextWithHint("##ServerHost_ServerName", "Server name (required)", &m_serverName))
    {
        hostname->SetValue(m_serverName.c_str());
    }
    
    if (ImGui::InputTextWithHint("##ServerHost_ServerDesc", "Server description (optional)", &m_serverDescription))
    {
        hostdesc.SetValue(m_serverDescription.c_str());
    }

    ImGui::Spacing();

    if (ImGui::BeginCombo("Mode", m_gameMode.c_str()))
    {
        for (const CUtlString& playlist : g_vecAllPlaylists)
        {
            const char* const cachedPlaylists = playlist.String();

            if (ImGui::Selectable(cachedPlaylists, 
                playlist.IsEqual_CaseInsensitive(m_gameMode.c_str())))
            {
                m_gameMode = cachedPlaylists;
            }
        }

        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Map", m_levelName.c_str()))
    {
        g_InstalledMapsMutex.Lock();

        for (const CUtlString& mapName : g_InstalledMaps)
        {
            const char* const cachedMapName = mapName.String();

            if (ImGui::Selectable(cachedMapName,
                mapName.IsEqual_CaseInsensitive(m_levelName.c_str())))
            {
                m_levelName = cachedMapName;
            }
        }

        g_InstalledMapsMutex.Unlock();
        ImGui::EndCombo();
    }

    m_queryGlobalBanList = sv_globalBanlist.GetBool(); // Sync toggle with 'sv_globalBanlist'.
    if (ImGui::Checkbox("Load global banned list", &m_queryGlobalBanList))
    {
        sv_globalBanlist.SetValue(m_queryGlobalBanList);
    }

    ImGui::Text("Server visibility");

    if (ImGui::SameLine(); ImGui::RadioButton("offline", g_ServerHostManager.GetVisibility() == ServerVisibility_e::OFFLINE))
    {
        g_ServerHostManager.SetVisibility(ServerVisibility_e::OFFLINE);
    }
    if (ImGui::SameLine(); ImGui::RadioButton("hidden", g_ServerHostManager.GetVisibility() == ServerVisibility_e::HIDDEN))
    {
        g_ServerHostManager.SetVisibility(ServerVisibility_e::HIDDEN);
    }
    if (ImGui::SameLine(); ImGui::RadioButton("public", g_ServerHostManager.GetVisibility() == ServerVisibility_e::PUBLIC))
    {
        g_ServerHostManager.SetVisibility(ServerVisibility_e::PUBLIC);
    }

    ImGui::TextColored(m_hostMessageColor, "%s", m_hostMessage.c_str());
    if (!m_hostToken.empty())
    {
        ImGui::InputText("##ServerHost_HostToken", &m_hostToken, ImGuiInputTextFlags_ReadOnly);
    }

    ImGui::Spacing();

    const ImVec2 contentRegionMax = ImGui::GetContentRegionAvail();

    const bool serverActive = g_pServer->IsActive();
    const bool clientActive = g_pClientState->IsActive();

    const bool isOffline = g_ServerHostManager.GetVisibility() == ServerVisibility_e::OFFLINE;
    const bool hasName = isOffline ? true : !m_serverName.empty();

    if (!g_pHostState->m_bActiveGame)
    {
        if (ImGui::Button("Start server", ImVec2(contentRegionMax.x, 32)))
        {
            m_hostMessage.clear();

            if (hasName && !m_levelName.empty() && !m_gameMode.empty())
            {
                g_ServerHostManager.LaunchServer(m_levelName.c_str(), m_gameMode.c_str()); // Launch server.
            }
            else
            {
                HandleInvalidFields(isOffline);
            }
        }

        if (ImGui::Button("Reload playlist", ImVec2(contentRegionMax.x, 32)))
        {
            v_Playlists_Download_f();
            Playlists_SDKInit(); // Re-Init playlist.
        }

        if (ImGui::Button("Reload banlist", ImVec2(contentRegionMax.x, 32)))
        {
            g_BanSystem.LoadList();
        }
    }
    else
    {
        if (ImGui::Button("Stop server", ImVec2(contentRegionMax.x, 32)))
        {
            ProcessCommand("LeaveMatch"); // TODO: use script callback instead.
            g_pHostState->m_iNextState = HostStates_t::HS_GAME_SHUTDOWN;
        }

        if (ImGui::Button("Change level", ImVec2(contentRegionMax.x, 32)))
        {
            if (!m_levelName.empty() && !m_gameMode.empty())
            {
                g_ServerHostManager.ChangeLevel(m_levelName.c_str(), m_gameMode.c_str());
            }
            else
            {
                HandleInvalidFields(isOffline);

                m_hostMessage = "Failed to change level: 'levelname' was empty.";
                m_hostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            }
        }

        if (serverActive)
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Rebuild AI network", ImVec2(contentRegionMax.x, 32)))
            {
                ProcessCommand("BuildAINFile");
            }

            if (ImGui::Button("Reload NavMesh", ImVec2(contentRegionMax.x, 32)))
            {
                ProcessCommand("navmesh_hotswap");
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Reparse AI settings", ImVec2(contentRegionMax.x, 32)))
            {
                Msg(eDLL_T::ENGINE, "Reparsing AI data on %s\n", clientActive ? "server and client" : "server");
                ProcessCommand("aisettings_reparse");

                if (g_pClientState->IsActive())
                {
                    ProcessCommand("aisettings_reparse_client");
                }
            }

            if (ImGui::Button("Reparse Weapon settings", ImVec2(contentRegionMax.x, 32)))
            {
                Msg(eDLL_T::ENGINE, "Reparsing weapon data on %s\n", clientActive ? "server and client" : "server");
                ProcessCommand("weapon_reparse");
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Reparse all scripts", ImVec2(contentRegionMax.x, 32)))
    {
        Msg(eDLL_T::ENGINE, "Reparsing all scripts on %s\n", "server and client");

        // NOTE: the following are already called during "reload" or "reconnect".
        //"aisettings_reparse"
        //"aisettings_reparse_client"

        //"damagedefs_reparse"
        //"damagedefs_reparse_client"

        //"playerSettings_reparse"
        //"fx_impact_reparse"

        ProcessCommand("ReloadAimAssistSettings");
        ProcessCommand("reload_localization");

        ProcessCommand("playlist_reload");
        ProcessCommand("banlist_reload");

        ProcessCommand("weapon_reparse");

        // Recompile all UI scripts
        ProcessCommand("uiscript_reset");

        if (serverActive)
        {
            // If we hit this code path, we are connected to a listen server,
            // reconnect to it to recompile all server and client side scripts.
            ProcessCommand("reload");
        }
        else if (clientActive)
        {
            // If we hit this code path, we are connected to a remote server,
            // reconnect to it to recompile all client side scripts.
            ProcessCommand("reconnect");
        }
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: updates the host status
//-----------------------------------------------------------------------------
void CBrowser::UpdateHostingStatus(void)
{
#ifndef CLIENT_DLL
    assert(g_pHostState && g_pCVar);

    const HostStatus_e hostStatus = (g_ServerHostManager.GetVisibility() != ServerVisibility_e::OFFLINE && g_pServer->IsActive())
        ? HostStatus_e::HOSTING 
        : HostStatus_e::NOT_HOSTING;

    g_ServerHostManager.SetHostStatus(hostStatus); // Are we hosting a server?

    switch (hostStatus)
    {
    case HostStatus_e::NOT_HOSTING:
    {
        if (!m_hostToken.empty())
        {
            m_hostToken.clear();
        }

        if (ImGui::ColorConvertFloat4ToU32(m_hostMessageColor) == // Only clear if this is green (a valid hosting message).
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.00f, 1.00f, 0.00f, 1.00f)))
        {
            m_hostMessage.clear();
            m_hostMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        }

        break;
    }
    case HostStatus_e::HOSTING:
    {
        if (*g_nServerRemoteChecksum == NULL) // Check if script checksum is valid yet.
        {
            break;
        }

        const ServerVisibility_e serverVisibility = g_ServerHostManager.GetVisibility();

        if (serverVisibility == ServerVisibility_e::OFFLINE)
        {
            break;
        }

        const NetGameServer_t netGameServer
        {
            hostname->GetString(),
            hostdesc.GetString(),
            serverVisibility == ServerVisibility_e::PUBLIC,
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

        SendHostingPostRequest(netGameServer);
        break;
    }
    default:
        break;
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: sends the hosting POST request to the comp server and installs the
//          host data on the server browser
// Input  : &gameServer - 
//-----------------------------------------------------------------------------
void CBrowser::SendHostingPostRequest(const NetGameServer_t& gameServer)
{
#ifndef CLIENT_DLL
    std::thread request([&, gameServer]
        {
            string hostRequestMessage;
            string hostToken;
            string hostIp;

            const bool result = g_MasterServer.PostServerHost(hostRequestMessage, hostToken, hostIp, gameServer);

            g_TaskQueue.Dispatch([&, result, hostRequestMessage, hostToken, hostIp]
                {
                    InstallHostingDetails(result, hostRequestMessage.c_str(), hostToken.c_str(), hostIp);
                }, 0);
        }
    );
    request.detach();
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: installs the host data on the server browser
// Input  : postFailed - 
//          *hostMessage - 
//          *hostToken - 
//          &hostIp - 
//-----------------------------------------------------------------------------
void CBrowser::InstallHostingDetails(const bool postFailed, const char* const hostMessage, const char* const hostToken, const string& hostIp)
{
#ifndef CLIENT_DLL
    m_hostMessage = hostMessage;
    m_hostToken = hostToken;

    if (!hostIp.empty())
    {
        g_ServerHostManager.SetHostIP(hostIp);
    }

    if (postFailed)
    {
        m_hostMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        stringstream ssMessage;
        ssMessage << "Broadcasting";
        if (!m_hostToken.empty())
        {
            ssMessage << ": share the following token for clients to connect: ";
        }

        m_hostMessage = ssMessage.str();
    }
    else
    {
        m_hostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: processes submitted commands
// Input  : *pszCommand - 
//-----------------------------------------------------------------------------
void CBrowser::ProcessCommand(const char* pszCommand) const
{
    Cbuf_AddText(Cbuf_GetCurrentPlayer(), pszCommand, cmd_source_t::kCommandSrcCode);
}

//-----------------------------------------------------------------------------
// Purpose: toggles the server browser
//-----------------------------------------------------------------------------
void CBrowser::ToggleBrowser_f()
{
    g_Browser.m_activated ^= true;
    ResetInput(); // Disable input to game when browser is drawn.
}

CBrowser g_Browser;
