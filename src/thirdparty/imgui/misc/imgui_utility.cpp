/*-----------------------------------------------------------------------------
 * _imgui_utility.cpp
 *-----------------------------------------------------------------------------*/

#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "tier0/memstd.h"
#include "tier1/keyvalues.h"
#include "filesystem/filesystem.h"
#include "thirdparty/imgui/misc/imgui_utility.h"

#define GAME_CONSOLE_KEY "Console"
#define GAME_BROWSER_KEY "Browser"

void ImGuiConfig::Load()
{
    const string svPath = Format(SDK_USER_CFG_PATH"%s", IMGUI_BIND_FILE);
    Msg(eDLL_T::MS, "Loading ImGui config file '%s'\n", svPath.c_str());

    FileSystem()->CreateDirHierarchy(SDK_USER_CFG_PATH, "PLATFORM"); // Create directory, so ImGui can load/save 'layout.ini'.

    KeyValues* pKeyMapKV = FileSystem()->LoadKeyValues(IFileSystem::TYPE_COMMON, svPath.c_str(), "PLATFORM");
    if (!pKeyMapKV)
    {
        return;
    }

    KeyValues* pConsoleKV = pKeyMapKV->FindKey(GAME_CONSOLE_KEY);
    if (pConsoleKV)
    {
        m_ConsoleConfig.m_nBind0 = pConsoleKV->GetInt("$primaryKey", VK_OEM_3);
        m_ConsoleConfig.m_nBind1 = pConsoleKV->GetInt("$secondaryKey", VK_F10);
    }

    KeyValues* pBrowserKV = pKeyMapKV->FindKey(GAME_BROWSER_KEY);
    if (pBrowserKV)
    {
        m_BrowserConfig.m_nBind0 = pBrowserKV->GetInt("$primaryKey", VK_INSERT);
        m_BrowserConfig.m_nBind1 = pBrowserKV->GetInt("$secondaryKey", VK_F11);
    }

    pKeyMapKV->DeleteThis();
}

void ImGuiConfig::Save()
{
    const string svPath = Format(SDK_USER_CFG_PATH"%s", IMGUI_BIND_FILE);
    Msg(eDLL_T::MS, "Saving ImGui config file '%s'\n", svPath.c_str());

    FileSystem()->CreateDirHierarchy(SDK_USER_CFG_PATH, "PLATFORM"); // Create directory, so ImGui can load/save 'layout.ini'.

    KeyValues kv("KeyMap");

    KeyValues* pConsoleKV = kv.FindKey(GAME_CONSOLE_KEY, true);
    pConsoleKV->SetInt("$primaryKey", m_ConsoleConfig.m_nBind0);
    pConsoleKV->SetInt("$secondaryKey", m_ConsoleConfig.m_nBind1);

    KeyValues* pBrowserKV = kv.FindKey(GAME_BROWSER_KEY, true);
    pBrowserKV->SetInt("$primaryKey", m_BrowserConfig.m_nBind0);
    pBrowserKV->SetInt("$secondaryKey", m_BrowserConfig.m_nBind1);

    CUtlBuffer uBuf(0i64, 0, CUtlBuffer::TEXT_BUFFER);

    kv.RecursiveSaveToFile(uBuf, 0);
    FileSystem()->WriteFile(svPath.c_str(), "PLATFORM", uBuf);
}

ImGuiStyle_t ImGuiConfig::InitStyle() const
{
    ImGuiStyle_t selected                 = ImGuiStyle_t::NONE;

    if (strcmp(CommandLine()->ParmValue("-imgui_theme", ""), "legacy") == 0)
    {
        selected = ImGuiStyle_t::LEGACY;
    }
    else if (strcmp(CommandLine()->ParmValue("-imgui_theme", ""), "modern") == 0)
    {
        selected = ImGuiStyle_t::MODERN;
    }
    else
    {
        selected = ImGuiStyle_t::DEFAULT;
    }

    ImGui_SetStyle(selected);
    return selected;
}

ImGuiConfig g_ImGuiConfig;
