#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "networksystem/serverlisting.h"
#include "networksystem/pylon.h"
#include "thirdparty/imgui/misc/imgui_utility.h"

#include "imgui_surface.h"

class CBrowser : public CImguiSurface
{
public:
    CBrowser(void);
    virtual ~CBrowser(void);

    virtual bool Init(void);
    virtual void Shutdown(void);

    virtual void RunFrame(void);
    void RunTask(void);

    virtual bool DrawSurface(void);

    void DrawBrowserPanel(void);
    void RefreshServerList(void);

    void HiddenServersModal(void);

    void HandleInvalidFields(const bool offline);
    void DrawHostPanel(void);

    void UpdateHostingStatus(void);
    void InstallHostingDetails(const bool postFailed, const char* const hostMessage, const char* const hostToken, const string& hostIp);
    void SendHostingPostRequest(const NetGameServer_t& gameServer);

    void ProcessCommand(const char* pszCommand) const;

public:
    // Command callbacks
    static void ToggleBrowser_f();

private:
    inline void SetServerListMessage(const char* const message) { m_serverListMessage = message; };
    inline void SetHostMessage(const char* const message) { m_hostMessage = message; }
    inline void SetHostToken(const char* const message) { m_hostToken = message; }

private:
    bool m_reclaimFocusOnTokenField;
    bool m_queryNewListNonRecursive; // When set, refreshes the server list once the next frame.
    bool m_queryGlobalBanList;
    char m_serverTokenTextBuf[128];
    char m_serverAddressTextBuf[128];
    char m_serverNetKeyTextBuf[45];

    ID3D11ShaderResourceView* m_lockedIconShaderResource;
    MODULERESOURCE m_lockedIconDataResource;

    ////////////////////
    //   Server List  //
    ////////////////////
    ImGuiTextFilter m_serverBrowserTextFilter;
    string m_serverListMessage;

    ////////////////////
    //   Host Server  //
    ////////////////////
    string m_hostMessage;
    string m_hostToken;
    ImVec4 m_hostMessageColor;

    ////////////////////
    // Private Server //
    ////////////////////
    string m_hiddenServerRequestMessage;
    ImVec4 m_hiddenServerMessageColor;

    string m_serverName;
    string m_serverDescription;

    string m_levelName;
    string m_gameMode;
};

extern CBrowser g_Browser;
#endif