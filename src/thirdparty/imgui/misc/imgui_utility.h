#pragma once
#include "imgui_style.h"

constexpr char IMGUI_BIND_FILE[] = "keymap.vdf";

class ImGuiConfig
{
public:
    struct
    {
        int m_nBind0 = VK_OEM_3;
        int m_nBind1 = VK_F10;
    } m_ConsoleConfig;

    struct
    {
        int m_nBind0 = VK_INSERT;
        int m_nBind1 = VK_F11;
    } m_BrowserConfig;

    void Load();
    void Save();
    ImGuiStyle_t InitStyle() const;

    inline bool KeyUsed(const int key) const
    {
        return (key == m_ConsoleConfig.m_nBind0 || key == m_ConsoleConfig.m_nBind1)
            || (key == m_BrowserConfig.m_nBind0 || key == m_BrowserConfig.m_nBind1);
    };
};

extern ImGuiConfig g_ImGuiConfig;
