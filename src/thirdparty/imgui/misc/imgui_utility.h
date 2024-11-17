#pragma once
#include "imgui_style.h"

constexpr char IMGUI_BIND_FILE[] = "keymap.vdf";

class ImGuiConfig
{
public:
    struct
    {
        int m_nBind0 = VK_OEM_3;
        int m_nBind1 = VK_PRIOR;
    } m_ConsoleConfig;

    struct
    {
        int m_nBind0 = VK_F10;
        int m_nBind1 = VK_NEXT;
    } m_BrowserConfig;

    void Load();
    void Save();
    ImGuiStyle_t InitStyle() const;
};

extern ImGuiConfig g_ImGuiConfig;
