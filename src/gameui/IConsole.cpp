/******************************************************************************
-------------------------------------------------------------------------------
File   : IConsole.cpp
Date   : 15:06:2021
Author : Kawe Mazidjatari
Purpose: Implements the in-game console front-end
-------------------------------------------------------------------------------
History:
- 15:06:2021 | 14:56 : Created by Kawe Mazidjatari
- 07:08:2021 | 15:22 : Multi-thread 'CommandExecute' operations to prevent deadlock in render thread
- 07:08:2021 | 15:25 : Fix a race condition that occurred when detaching the 'CommandExecute' thread

******************************************************************************/

#include "core/stdafx.h"
#include "core/init.h"
#include "core/resource.h"
#include "tier0/frametask.h"
#include "tier0/commandline.h"
#include "windows/id3dx.h"
#include "windows/console.h"
#include "windows/resource.h"
#include "engine/cmd.h"
#include "gameui/IConsole.h"
#include "imgui_system.h"

//-----------------------------------------------------------------------------
// Console variables
//-----------------------------------------------------------------------------
static ConVar con_max_lines("con_max_lines", "1024", FCVAR_DEVELOPMENTONLY | FCVAR_ACCESSIBLE_FROM_THREADS, "Maximum number of lines in the console before cleanup starts", true, 1.f, false, 0.f);
static ConVar con_max_history("con_max_history", "512", FCVAR_DEVELOPMENTONLY, "Maximum number of command submission items before history cleanup starts", true, 0.f, false, 0.f);

static ConVar con_suggest_limit("con_suggest_limit", "128", FCVAR_DEVELOPMENTONLY, "Maximum number of suggestions the autocomplete window will show for the console", true, 0.f, false, 0.f);
static ConVar con_suggest_helptext("con_suggest_helptext", "1", FCVAR_RELEASE, "Show CommandBase help text in autocomplete window");

static ConVar con_autocomplete_window_textures("con_autocomplete_window_textures", "1", FCVAR_RELEASE, "Show help textures in autocomplete window");
static ConVar con_autocomplete_window_width("con_autocomplete_window_width", "0", FCVAR_RELEASE, "The maximum width of the console's autocomplete window", true, 0.f, false, 0.f);
static ConVar con_autocomplete_window_height("con_autocomplete_window_height", "217.5", FCVAR_RELEASE, "The maximum height of the console's autocomplete window", true, 0.f, false, 0.f);

//-----------------------------------------------------------------------------
// Console commands
//-----------------------------------------------------------------------------
static ConCommand toggleconsole("toggleconsole", CConsole::ToggleConsole_f, "Show/hide the developer console.", FCVAR_CLIENTDLL | FCVAR_RELEASE);

static ConCommand con_history("con_history", CConsole::LogHistory_f, "Shows the developer console submission history", FCVAR_CLIENTDLL | FCVAR_RELEASE);
static ConCommand con_removeline("con_removeline", CConsole::RemoveLine_f, "Removes a range of lines from the developer console", FCVAR_CLIENTDLL | FCVAR_RELEASE);
static ConCommand con_clearlines("con_clearlines", CConsole::ClearLines_f, "Clears all lines from the developer console", FCVAR_CLIENTDLL | FCVAR_RELEASE);
static ConCommand con_clearhistory("con_clearhistory", CConsole::ClearHistory_f, "Clears all submissions from the developer console history", FCVAR_CLIENTDLL | FCVAR_RELEASE);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::CConsole(void) 
    : m_loggerLabel("LoggingRegion")
    , m_historyPos(ConAutoCompletePos_e::kPark)
    , m_suggestPos(ConAutoCompletePos_e::kPark)
    , m_scrollBackAmount(0)
    , m_selectBackAmount(0)
    , m_selectedSuggestionTextLen(0)
    , m_lastFrameScrollPos(0.f, 0.f)
    , m_inputTextBufModified(false)
    , m_canAutoComplete(false)
    , m_autoCompleteActive(false)
    , m_autoCompletePosMoved(false)
{
    m_surfaceLabel = "Console";

    memset(m_inputTextBuf, '\0', sizeof(m_inputTextBuf));
    snprintf(m_summaryTextBuf, sizeof(m_summaryTextBuf), "%zu history items", m_vecHistory.size());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::~CConsole(void)
{
    Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: game console initialization
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CConsole::Init(void)
{
    SetStyleVar(1200, 524, -1000, 50);
    return LoadFlagIcons();
}

//-----------------------------------------------------------------------------
// Purpose: game console shutdown
//-----------------------------------------------------------------------------
void CConsole::Shutdown(void)
{
    for (MODULERESOURCE& flagIcon : m_vecFlagIcons)
    {
        if (flagIcon.m_idIcon)
        {
            flagIcon.m_idIcon->Release();
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: game console main render loop
//-----------------------------------------------------------------------------
void CConsole::RunFrame(void)
{
    // Uncomment these when adjusting the theme or layout.
    {
        //ImGui::ShowStyleEditor();
        //ImGui::ShowDemoWindow();
    }

    /**************************
     * BASE PANEL SETUP       *
     **************************/
    if (!m_initialized)
    {
        Init();
        m_initialized = true;
    }

    Animate();

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
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, minBaseWindowRect);       baseWindowStyleVars++;

    const bool drawn = DrawSurface();
    ImGui::PopStyleVar(baseWindowStyleVars);

    // If we didn't draw the console, don't draw the suggest panel
    if (!drawn)
        return;

    /**************************
     * SUGGESTION PANEL SETUP *
     **************************/
    if (RunAutoComplete())
    {
        if (m_surfaceStyle == ImGuiStyle_t::MODERN)
        {
            const ImGuiStyle& style = ImGui::GetStyle();
            m_autoCompleteWindowPos.y = m_autoCompleteWindowPos.y + style.WindowPadding.y + 1.5f;
        }

        ImGui::SetNextWindowPos(m_autoCompleteWindowPos);
        ImGui::SetNextWindowSize(m_autoCompleteWindowRect);

        int autoCompleteStyleVars = 0;

        // NOTE: 68 is the minimum width of the autocomplete window as this
        // leaves enough space to show the flag and the first 4 characters
        // of the suggestion. 37 is the minimum height as anything lower
        // will truncate the first element in the autocomplete window.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(68, 37));  autoCompleteStyleVars++;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);         autoCompleteStyleVars++;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha);             autoCompleteStyleVars++;

        DrawAutoCompletePanel();

        ImGui::PopStyleVar(autoCompleteStyleVars);
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the console's main surface
// Output : true if a frame has been drawn, false otherwise
//-----------------------------------------------------------------------------
bool CConsole::DrawSurface(void)
{
    if (!ImGui::Begin(m_surfaceLabel, &m_activated, ImGuiWindowFlags_None, &ResetInput))
    {
        ImGui::End();
        return false;
    }

    m_mainWindow = ImGui::GetCurrentWindow();

    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr);

    ///////////////////////////////////////////////////////////////////////
    ImGui::Separator();
    if (ImGui::BeginPopup("Options"))
    {
        DrawOptionsPanel();
    }
    if (ImGui::Button("Options"))
    {
        ImGui::OpenPopup("Options");
    }

    ImGui::SameLine();

    // Reserve enough left-over height and width for 1 separator + 1 input text
    const float footerWidthReserve = style.ItemSpacing.y + ImGui::GetWindowWidth();
    m_colorTextLogger.GetFilter().Draw("Filter (inc,-exc)", footerWidthReserve - 350);

    ImGui::Separator();

    ///////////////////////////////////////////////////////////////////////
    if (!m_colorTextLogger.IsScrolledToBottom() && m_scrollBackAmount > 0)
    {
        const ImGuiID windowId = m_mainWindow->GetID(m_loggerLabel);

        char windowName[128];
        snprintf(windowName, sizeof(windowName), "%s/%s_%08X", m_surfaceLabel, m_loggerLabel, windowId);

        ImGui::SetWindowScrollY(windowName, m_lastFrameScrollPos.y - m_scrollBackAmount * fontSize.y);
    }
    m_scrollBackAmount = 0;

    // Reserve enough left-over height for 2 text elements.
    float footerHeightReserve = ImGui::GetFrameHeight() * 2;
    ImGuiChildFlags loggerFlags = ImGuiChildFlags_None;

    const bool isLegacyStyle = m_surfaceStyle == ImGuiStyle_t::LEGACY;
    int numLoggerStyleVars = 0;

    if (isLegacyStyle)
    {
        loggerFlags |= ImGuiChildFlags_Border;

        // Eliminate padding around logger child. This padding gets added when
        // ImGuiChildFlags_Border flag gets set.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 1.f, 1.f }); numLoggerStyleVars++;

        // if we use the legacy theme, also account for one extra space as the
        // legacy theme has an extra separator at the bottom of the logger.
        footerHeightReserve += style.ItemSpacing.y;
    }

    const static int colorLoggerWindowFlags = 
        ImGuiWindowFlags_NoMove                    |
        ImGuiWindowFlags_HorizontalScrollbar       |
        ImGuiWindowFlags_NoNavInputs               |
        ImGuiWindowFlags_OverlayHorizontalScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha); numLoggerStyleVars++;
    ImGui::BeginChild(m_loggerLabel, ImVec2(0, -footerHeightReserve), loggerFlags, colorLoggerWindowFlags);

    // NOTE: scoped so the mutex releases after we have rendered.
    // this is currently the only place the color logger is used
    // during the drawing of the base panel.
    {
        AUTO_LOCK(m_colorTextLoggerMutex);
        m_colorTextLogger.Render();
    }

    m_lastFrameScrollPos = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());

    ImGui::EndChild();

    if (numLoggerStyleVars)
        ImGui::PopStyleVar(numLoggerStyleVars);

    // The legacy theme also has a spacer here.
    if (isLegacyStyle)
        ImGui::Separator();

    ImGui::Text("%s", m_summaryTextBuf);

    const std::function<void(void)> fnHandleInput = [&](void)
    {
        if (m_inputTextBuf[0])
        {
            ProcessCommand(m_inputTextBuf);
            ResetAutoCompleteData();

            m_inputTextBufModified = true;
        }

        BuildSummaryText("");
        m_reclaimFocus = true;
    };

    ///////////////////////////////////////////////////////////////////////
    const static int inputTextFieldFlags =
        ImGuiInputTextFlags_EnterReturnsTrue       |
        ImGuiInputTextFlags_CallbackCompletion     |
        ImGuiInputTextFlags_CallbackHistory        |
        ImGuiInputTextFlags_CallbackAlways         |
        ImGuiInputTextFlags_CallbackCharFilter     |
        ImGuiInputTextFlags_CallbackEdit           |
        ImGuiInputTextFlags_AutoCaretEnd;

    ImGui::PushItemWidth(footerWidthReserve - 80);
    if (ImGui::InputText("##input", m_inputTextBuf, IM_ARRAYSIZE(m_inputTextBuf), inputTextFieldFlags, &TextEditCallbackStub, reinterpret_cast<void*>(this)))
    {
        // If we selected something in the suggestions window, create the
        // command from that instead
        if (m_suggestPos > ConAutoCompletePos_e::kPark)
        {
            DetermineInputTextFromSelectedSuggestion(m_vecSuggest[m_suggestPos], m_selectedSuggestionText);
            BuildSummaryText(m_selectedSuggestionText.c_str());

            m_inputTextBufModified = true;
            m_reclaimFocus = true;
        }
        else
        {
            fnHandleInput();
        }
    }

    // Auto-focus input field on window apparition.
    ImGui::SetItemDefaultFocus();

    // Auto-focus input field if reclaim is demanded.
    if (m_reclaimFocus)
    {
        ImGui::SetKeyboardFocusHere(-1); // -1 means previous widget.
        m_reclaimFocus = false;
    }

    DetermineAutoCompleteWindowRect();

    ImGui::SameLine();
    if (ImGui::Button("Submit"))
    {
        fnHandleInput();
    }

    ImGui::End();
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: draws the options panel
//-----------------------------------------------------------------------------
void CConsole::DrawOptionsPanel(void)
{
    ImGui::Checkbox("Auto-scroll", &m_colorTextLogger.m_bAutoScroll);

    ImGui::SameLine();
    ImGui::PushItemWidth(100);

    ImGui::PopItemWidth();

    if (ImGui::SmallButton("Clear"))
    {
        ClearLog();
    }

    ImGui::SameLine();

    // Copies all logged text to the clip board
    if (ImGui::SmallButton("Copy"))
    {
        AUTO_LOCK(m_colorTextLoggerMutex);
        m_colorTextLogger.Copy(true);
    }

    ImGui::Text("Console hotkey:");
    ImGui::SameLine();

    if (ImGui::Hotkey("##ToggleConsole", &g_ImGuiConfig.m_ConsoleConfig.m_nBind0, ImVec2(80, 80)))
    {
        g_ImGuiConfig.Save();
    }

    ImGui::Text("Browser hotkey:");
    ImGui::SameLine();

    if (ImGui::Hotkey("##ToggleBrowser", &g_ImGuiConfig.m_BrowserConfig.m_nBind0, ImVec2(80, 80)))
    {
        g_ImGuiConfig.Save();
    }

    ImGui::EndPopup();
}

//-----------------------------------------------------------------------------
// Purpose: draws the autocomplete panel with results based on user input
//-----------------------------------------------------------------------------
void CConsole::DrawAutoCompletePanel(void)
{
    const static int autoCompleteWindowFlags = 
        ImGuiWindowFlags_NoTitleBar                |
        ImGuiWindowFlags_NoMove                    |
        ImGuiWindowFlags_NoSavedSettings           |
        ImGuiWindowFlags_NoFocusOnAppearing        |
        ImGuiWindowFlags_AlwaysVerticalScrollbar   |
        ImGuiWindowFlags_AlwaysHorizontalScrollbar;

    ImGui::Begin("##suggest", nullptr, autoCompleteWindowFlags);
    ImGui::PushAllowKeyboardFocus(false);

    ImGuiWindow* const autocompleteWindow = ImGui::GetCurrentWindow();

    // NOTE: this makes sure we always draw this window behind the main console
    // window, this is necessary as otherwise if you were to drag another
    // window above the console, and then focus on the console again, that
    // window will now be in between the console window and the autocomplete
    // suggest window.
    ImGui::BringWindowToDisplayBehind(autocompleteWindow, m_mainWindow);

    for (size_t i = 0, ns = m_vecSuggest.size(); i < ns; i++)
    {
        const ConAutoCompleteSuggest_s& suggest = m_vecSuggest[i];
        const bool isIndexActive = m_suggestPos == ssize_t(i);

        ImGui::PushID(static_cast<int>(i));

        if (m_autoCompleteTexturesLoaded && con_autocomplete_window_textures.GetBool())
        {
            // Show the flag texture before the cvar name.
            const int mainTexIdx = GetFlagTextureIndex(suggest.flags);
            const MODULERESOURCE& mainRes = m_vecFlagIcons[mainTexIdx];

            ImGui::Image(mainRes.m_idIcon, ImVec2(float(mainRes.m_nWidth), float(mainRes.m_nHeight)));

            // Show a more detailed description of the flag when user hovers over the texture.
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly) &&
                suggest.flags != COMMAND_COMPLETION_MARKER)
            {
                const std::function<void(const ConVarFlags::FlagDesc_t&)> fnAddHint = [&](const ConVarFlags::FlagDesc_t& cvarInfo)
                {
                    const int hintTexIdx = GetFlagTextureIndex(cvarInfo.bit);
                    const MODULERESOURCE& hintRes = m_vecFlagIcons[hintTexIdx];

                    ImGui::Image(hintRes.m_idIcon, ImVec2(float(hintRes.m_nWidth), float(hintRes.m_nHeight)));
                    ImGui::SameLine();
                    ImGui::Text("%s", cvarInfo.shortdesc);
                };

                ImGui::BeginTooltip();
                bool isFlagSet = false;

                // Reverse loop to display the most significant flag first.
                for (int j = IM_ARRAYSIZE(g_ConVarFlags.m_FlagsToDesc); (j--) > 0;)
                {
                    const ConVarFlags::FlagDesc_t& info = g_ConVarFlags.m_FlagsToDesc[j];
                    if (suggest.flags & info.bit)
                    {
                        isFlagSet = true;
                        fnAddHint(info);
                    }
                }
                if (!isFlagSet) // Display the FCVAR_NONE flag if no flags are set.
                {
                    fnAddHint(g_ConVarFlags.m_FlagsToDesc[0]);
                }

                ImGui::EndTooltip();
            }

            ImGui::SameLine();
        }

        if (ImGui::Selectable(suggest.text.c_str(), isIndexActive))
        {
            ImGui::Separator();
            string newInputText;

            DetermineInputTextFromSelectedSuggestion(suggest, newInputText);
            memmove(m_inputTextBuf, newInputText.data(), newInputText.size() + 1);

            m_canAutoComplete = true;
            m_reclaimFocus = true;

            BuildSummaryText(newInputText.c_str());
        }

        ImGui::PopID();

        // Update the suggest position
        if (m_autoCompletePosMoved)
        {
            if (isIndexActive) // Bring the 'active' element into view
            {
                ImRect imRect = ImGui::GetCurrentContext()->LastItemData.Rect;

                // Reset to keep flag icon in display.
                imRect.Min.x = autocompleteWindow->InnerRect.Min.x;
                imRect.Max.x = autocompleteWindow->InnerRect.Max.x;

                // Eliminate jiggle when going up/down in the menu.
                imRect.Min.y += 1;
                imRect.Max.y -= 1;

                ImGui::ScrollToRect(autocompleteWindow, imRect);
                m_autoCompletePosMoved = false;
            }
            else if (m_suggestPos == ConAutoCompletePos_e::kPark)
            {
                // Reset position; kPark = no active element.
                ImGui::SetScrollX(0.0f);
                ImGui::SetScrollY(0.0f);

                m_autoCompletePosMoved = false;
            }
        }
    }

    ImGui::PopAllowKeyboardFocus();
    ImGui::End();
}

//-----------------------------------------------------------------------------
// Purpose: runs the auto complete for the console
// Output : true if auto complete is performed, false otherwise
//-----------------------------------------------------------------------------
bool CConsole::RunAutoComplete(void)
{
    // Don't suggest if user tries to assign value to ConVar or execute ConCommand.
    if (!m_inputTextBuf[0] || strstr(m_inputTextBuf, ";"))
    {
        if (m_autoCompleteActive)
        {
            ResetAutoCompleteData();
        }

        return false;
    }

    if (!strstr(m_inputTextBuf, " "))
    {
        if (m_canAutoComplete)
        {
            CreateSuggestionsFromPartial();
        }
    }
    else if (m_canAutoComplete) // Command completion callback.
    {
        ResetAutoCompleteData();

        char szCommand[sizeof(m_inputTextBuf)];
        size_t i = 0;

        // Truncate everything past (and including) the space to get the
        // command string.
        for (; i < sizeof(m_inputTextBuf); i++)
        {
            const char c = m_inputTextBuf[i];

            if (c == '\0' || isspace(c))
            {
                break;
            }

            szCommand[i] = c;
        }

        szCommand[i] = '\0';
        ConCommand* const pCommand = g_pCVar->FindCommand(szCommand);

        if (pCommand && pCommand->CanAutoComplete())
        {
            CUtlVector< CUtlString > commands;
            const int iret = pCommand->AutoCompleteSuggest(m_inputTextBuf, commands);

            if (!iret)
            {
                return false;
            }

            for (int j = 0; j < iret; ++j)
            {
                m_vecSuggest.push_back(ConAutoCompleteSuggest_s(commands[j].String(), COMMAND_COMPLETION_MARKER));
            }
        }
        else
        {
            return false;
        }
    }

    if (m_vecSuggest.empty())
    {
        return false;
    }

    m_autoCompleteActive = true;
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: resets the auto complete window
//-----------------------------------------------------------------------------
void CConsole::ResetAutoCompleteData(void)
{
    m_suggestPos = ConAutoCompletePos_e::kPark;
    m_canAutoComplete = false;
    m_autoCompleteActive = false;
    m_autoCompletePosMoved = true;
    m_vecSuggest.clear();
}

//-----------------------------------------------------------------------------
// Purpose: find ConVars/ConCommands from user input and add to vector
// - Ignores ConVars marked FCVAR_HIDDEN
//-----------------------------------------------------------------------------
void CConsole::CreateSuggestionsFromPartial(void)
{
    ResetAutoCompleteData();

    ICvar::Iterator iter(g_pCVar);
    for (iter.SetFirst(); iter.IsValid(); iter.Next())
    {
        if (m_vecSuggest.size() >= con_suggest_limit.GetInt())
        {
            break;
        }

        const ConCommandBase* const commandBase = iter.Get();

        if (commandBase->IsFlagSet(FCVAR_HIDDEN))
        {
            continue;
        }

        const char* const commandName = commandBase->GetName();

        if (!V_stristr(commandName, m_inputTextBuf))
        {
            continue;
        }

        if (std::find(m_vecSuggest.begin(), m_vecSuggest.end(),
            commandName) == m_vecSuggest.end())
        {
            string docString;

            if (!commandBase->IsCommand())
            {
                const ConVar* conVar = reinterpret_cast<const ConVar*>(commandBase);

                docString = " = ["; // Assign current value to string if its a ConVar.
                docString.append(conVar->GetString());
                docString.append("]");
            }
            if (con_suggest_helptext.GetBool())
            {
                std::function<void(string& , const char*)> fnAppendDocString = [&](string& targetString, const char* toAppend)
                {
                    if (VALID_CHARSTAR(toAppend))
                    {
                        targetString.append(" - \"");
                        targetString.append(toAppend);
                        targetString.append("\"");
                    }
                };

                fnAppendDocString(docString, commandBase->GetHelpText());
                fnAppendDocString(docString, commandBase->GetUsageText());
            }
            m_vecSuggest.push_back(ConAutoCompleteSuggest_s(commandName + docString, commandBase->GetFlags()));
        }
        else
        {
            // Trying to push a duplicate ConCommandBase in the vector; code bug.
            Assert(0);
        }
    }

    std::sort(m_vecSuggest.begin(), m_vecSuggest.end());
}

//-----------------------------------------------------------------------------
// Purpose: processes submitted commands for the main thread
// Input  : inputText - 
//-----------------------------------------------------------------------------
void CConsole::ProcessCommand(const char* const inputText)
{
    string commandFormatted(inputText);
    StringRTrim(commandFormatted, " "); // Remove trailing white space characters to prevent history duplication.

    const ImU32 commandColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.00f, 0.80f, 0.60f, 1.00f));
    AddLog(commandColor, "%s] %s\n", Plat_GetProcessUpTime(), commandFormatted.c_str());

    Cbuf_AddText(Cbuf_GetCurrentPlayer(), commandFormatted.c_str(), cmd_source_t::kCommandSrcCode);
    m_historyPos = ConAutoCompletePos_e::kPark;

    AddHistory(commandFormatted.c_str());

    m_colorTextLogger.ShouldScrollToStart(true);
    m_colorTextLogger.ShouldScrollToBottom(true);
}

//-----------------------------------------------------------------------------
// Purpose: builds the console summary, this function will attempt to search
//          for a ConVar first, from which it formats the current and default
//          value. If the string is empty, or no ConVar is found, the function
//          formats the number of history items instead
// Input  : inputText - 
//-----------------------------------------------------------------------------
void CConsole::BuildSummaryText(const char* const inputText)
{
    if (*inputText)
    {
        string conVarFormatted(inputText);

        // Remove trailing space and/or semicolon before we call 'g_pCVar->FindVar(..)'.
        StringRTrim(conVarFormatted, " ;", true);
        const ConVar* const conVar = g_pCVar->FindVar(conVarFormatted.c_str());

        if (conVar && !conVar->IsFlagSet(FCVAR_HIDDEN))
        {
            // Display the current and default value of ConVar if found.
            snprintf(m_summaryTextBuf, sizeof(m_summaryTextBuf), "(\"%s\", default \"%s\")", 
                conVar->GetString(), conVar->GetDefault());

            return;
        }
    }

    snprintf(m_summaryTextBuf, sizeof(m_summaryTextBuf), "%zu history items", m_vecHistory.size());
}

//-----------------------------------------------------------------------------
// Purpose: creates the selected suggestion for input field
// Input  : &suggest - 
//-----------------------------------------------------------------------------
void CConsole::DetermineInputTextFromSelectedSuggestion(const ConAutoCompleteSuggest_s& suggest, string& svInput)
{
    if (suggest.flags == COMMAND_COMPLETION_MARKER)
    {
        svInput = suggest.text + ' ';
    }
    else // Remove the default value from ConVar before assigning it to the input buffer.
    {
        svInput = suggest.text.substr(0, suggest.text.find(' ')) + ' ';
    }
}

//-----------------------------------------------------------------------------
// Purpose: determines the autocomplete window rect
//-----------------------------------------------------------------------------
void CConsole::DetermineAutoCompleteWindowRect(void)
{
    float flSinglePadding = 0.f;
    const float flItemHeight = ImGui::GetTextLineHeightWithSpacing() + 1.0f;

    if (m_vecSuggest.size() > 1)
    {
        // Pad with 18 to keep all items in view.
        flSinglePadding = flItemHeight;
    }

    // NOTE: last item rect = the input text box, the idea here is to set the
    // pos to that of the input text bar, whilst also clamping the width to it.
    const ImVec2 lastItemRectMin = ImGui::GetItemRectMin();
    const ImVec2 lastItemRectSize = ImGui::GetItemRectSize();

    m_autoCompleteWindowPos = lastItemRectMin;
    m_autoCompleteWindowPos.y += lastItemRectSize.y;

    const float maxWindowWidth = con_autocomplete_window_width.GetFloat();

    const float flWindowWidth = maxWindowWidth > 0
        ? ImMin(maxWindowWidth, lastItemRectSize.x)
        : lastItemRectSize.x;

    // NOTE: minimum vertical size of the window, going below this will
    // truncate the first element in the window making it looked bugged.
    const static float minWindowHeight = 37.0f;

    const float flWindowHeight = flSinglePadding + ImClamp(
        static_cast<float>(m_vecSuggest.size() * flItemHeight), 
        minWindowHeight,
        con_autocomplete_window_height.GetFloat());

    m_autoCompleteWindowRect = ImVec2(flWindowWidth, flWindowHeight);
}

//-----------------------------------------------------------------------------
// Purpose: loads flag images from resource section (must be aligned with resource.h!)
// Output : true on success, false on failure 
//-----------------------------------------------------------------------------
bool CConsole::LoadFlagIcons(void)
{
    bool ret = false;

    // Get all flag image resources for displaying flags.
    for (int i = IDB_PNG3, k = NULL; i <= IDB_PNG32; i++, k++)
    {
        m_vecFlagIcons.push_back(MODULERESOURCE(GetModuleResource(i)));
        MODULERESOURCE& rFlagIcon = m_vecFlagIcons[k];

        ret = LoadTextureBuffer(reinterpret_cast<unsigned char*>(rFlagIcon.m_pData), // !TODO: Fall-back texture.
            static_cast<int>(rFlagIcon.m_nSize), &rFlagIcon.m_idIcon, &rFlagIcon.m_nWidth, &rFlagIcon.m_nHeight);

        if (!ret)
        {
            Assert(0, "Texture flags load failed for %i", i);
            break;
        }
    }

    m_autoCompleteTexturesLoaded = ret;
    return ret;
}

//-----------------------------------------------------------------------------
// Purpose: returns flag texture index for CommandBase (must be aligned with resource.h!)
//          in the future we should build the texture procedurally with use of popcnt.
// Input  : nFlags - 
//-----------------------------------------------------------------------------
int CConsole::GetFlagTextureIndex(const int flags) const
{
    switch (flags) // All indices for single/dual flag textures.
    {
    case FCVAR_DEVELOPMENTONLY:
        return 9;
    case FCVAR_GAMEDLL:
        return 10;
    case FCVAR_CLIENTDLL:
        return 11;
    case FCVAR_REPLICATED:
        return 12;
    case FCVAR_CHEAT:
        return 13;
    case FCVAR_RELEASE:
        return 14;
    case FCVAR_MATERIAL_SYSTEM_THREAD:
        return 15;
    case FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL:
        return 16;
    case FCVAR_DEVELOPMENTONLY | FCVAR_CLIENTDLL:
        return 17;
    case FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED:
        return 18;
    case FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT:
        return 19;
    case FCVAR_DEVELOPMENTONLY | FCVAR_MATERIAL_SYSTEM_THREAD:
        return 20;
    case FCVAR_REPLICATED | FCVAR_CHEAT:
        return 21;
    case FCVAR_REPLICATED | FCVAR_RELEASE:
        return 22;
    case FCVAR_GAMEDLL | FCVAR_CHEAT:
        return 23;
    case FCVAR_GAMEDLL | FCVAR_RELEASE:
        return 24;
    case FCVAR_CLIENTDLL | FCVAR_CHEAT:
        return 25;
    case FCVAR_CLIENTDLL | FCVAR_RELEASE:
        return 26;
    case FCVAR_MATERIAL_SYSTEM_THREAD | FCVAR_CHEAT:
        return 27;
    case FCVAR_MATERIAL_SYSTEM_THREAD | FCVAR_RELEASE:
        return 28;
    case COMMAND_COMPLETION_MARKER:
        return 29;

    default: // Hit when flag is zero/non-indexed or 3+ bits are set.

        const unsigned int v = __popcnt(flags);
        switch (v)
        {
        case 0:
            return 0; // Pink checker texture (FCVAR_NONE)
        case 1:
            return 1; // Yellow checker texture (non-indexed).
        default:

            // If 3 or more bits are set, we test the flags
            // and display the appropriate checker texture.
            bool mul = v > 2;

            if (flags & FCVAR_DEVELOPMENTONLY)
            {
                return mul ? 4 : 3;
            }
            else if (flags & FCVAR_CHEAT)
            {
                return mul ? 6 : 5;
            }
            else if (flags & FCVAR_RELEASE && // RELEASE command but no context restriction.
                !(flags & FCVAR_SERVER_CAN_EXECUTE) &&
                !(flags & FCVAR_CLIENTCMD_CAN_EXECUTE))
            {
                return mul ? 8 : 7;
            }

            // Rainbow checker texture (user needs to manually check flags).
            // These commands are not restricted if ran from the same context.
            return 2;
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: console input box callback
// Input  : *iData - 
// Output : 
//-----------------------------------------------------------------------------
int CConsole::TextEditCallback(ImGuiInputTextCallbackData* iData)
{
    switch (iData->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        // Locate beginning of current word.
        const char* pszWordEnd = iData->Buf + iData->CursorPos;
        const char* pszWordStart = pszWordEnd;
        while (pszWordStart > iData->Buf)
        {
            const char c = pszWordStart[-1];
            if (c == ' ' || c == '\t' || c == ',' || c == ';')
            {
                break;
            }
            pszWordStart--;
        }
        break;
    }
    case ImGuiInputTextFlags_CallbackHistory:
    {
        if (m_autoCompleteActive)
        {
            if (iData->EventKey == ImGuiKey_UpArrow && m_suggestPos > - 1)
            {
                m_suggestPos--;
                m_autoCompletePosMoved = true;
            }
            else if (iData->EventKey == ImGuiKey_DownArrow)
            {
                if (m_suggestPos < static_cast<int>(m_vecSuggest.size()) - 1)
                {
                    m_suggestPos++;
                    m_autoCompletePosMoved = true;
                }
            }
        }
        else // Allow user to navigate through the history if suggest panel isn't drawn.
        {
            const int prevHistoryPos = m_historyPos;

            if (iData->EventKey == ImGuiKey_UpArrow)
            {
                if (m_historyPos == ConAutoCompletePos_e::kPark)
                {
                    m_historyPos = static_cast<int>(m_vecHistory.size()) - 1;
                }
                else if (m_historyPos > 0)
                {
                    m_historyPos--;
                }
            }
            else if (iData->EventKey == ImGuiKey_DownArrow)
            {
                if (m_historyPos != ConAutoCompletePos_e::kPark)
                {
                    if (++m_historyPos >= static_cast<int>(m_vecHistory.size()))
                    {
                        m_historyPos = ConAutoCompletePos_e::kPark;
                    }
                }
            }

            if (prevHistoryPos != m_historyPos)
            {
                string historyText = (m_historyPos >= 0) ? m_vecHistory[m_historyPos] : "";

                if (!historyText.empty())
                {
                    if (historyText.find(' ') == string::npos)
                    {
                        // Append whitespace to previous entered command if 
                        // absent or no parameters where passed. This is to
                        // the user could directly start adding their own
                        // params without needing to hit the space bar first.
                        historyText.append(" ");
                    }
                }

                iData->DeleteChars(0, iData->BufTextLen);
                iData->InsertChars(0, historyText.c_str());
            }
        }

        BuildSummaryText(iData->Buf);
        break;
    }
    case ImGuiInputTextFlags_CallbackAlways:
    {
        m_selectedSuggestionTextLen = iData->BufTextLen;

        if (m_inputTextBufModified) // User entered a value in the input field.
        {
            iData->DeleteChars(0, m_selectedSuggestionTextLen);

            if (!m_selectedSuggestionText.empty()) // User selected a ConVar from the suggestion window, copy it to the buffer.
            {
                iData->InsertChars(0, m_selectedSuggestionText.c_str());
                m_selectedSuggestionText.clear();

                m_canAutoComplete = true;
                m_reclaimFocus = true;
            }

            m_inputTextBufModified = false;
        }

        break;
    }
    case ImGuiInputTextFlags_CallbackCharFilter:
    {
        const ImWchar c = iData->EventChar;

        if (!m_selectedSuggestionTextLen)
        {
            if (c == '~') // Discard tilde character as first input.
            {
                iData->EventChar = 0;
                return 1;
            }
        }

        if (c == '`') // Discard back quote character (default console invoke key).
        {
            iData->EventChar = 0;
            return 1;
        }

        return 0;
    }
    case ImGuiInputTextFlags_CallbackEdit:
    {
        // If user selected all text in the input field and replaces it with
        // a tilde or space character, it will be set as the first character
        // in the input field as m_nInputTextLen is set before the actual edit.
        while (iData->Buf[0] == '~' || iData->Buf[0] == ' ')
        {
            iData->DeleteChars(0, 1);
        }

        if (iData->BufTextLen)
        {
            m_canAutoComplete = true;
        }
        else // Reset state and enable history scrolling when buffer is empty.
        {
            ResetAutoCompleteData();
        }

        BuildSummaryText(iData->Buf);
        break;
    }
    }

    return 0;
}

//-----------------------------------------------------------------------------
// Purpose: console input box callback stub
// Input  : *iData - 
// Output : 
//-----------------------------------------------------------------------------
int CConsole::TextEditCallbackStub(ImGuiInputTextCallbackData* iData)
{
    CConsole* const pConsole = reinterpret_cast<CConsole*>(iData->UserData);
    return pConsole->TextEditCallback(iData);
}

//-----------------------------------------------------------------------------
// Purpose: adds logs to the console; this is the only place text is added to
// the vector, do not call 'm_Logger.InsertText' elsewhere as we also manage
// the size of the vector here !!!
// Input  : &conLog - 
//-----------------------------------------------------------------------------
void CConsole::AddLog(const char* const text, const ImU32 color)
{
    if (!ImguiSystem()->IsEnabled())
        return;

    AUTO_LOCK(m_colorTextLoggerMutex);

    m_colorTextLogger.InsertText(text, color);
    ClampLogSize();
}

//-----------------------------------------------------------------------------
// Purpose: adds logs to the console (internal)
// Input  : &color - 
//          *fmt - 
//          ... - 
//-----------------------------------------------------------------------------
void CConsole::AddLog(const ImU32 color, const char* fmt, ...) /*IM_FMTARGS(2)*/
{
    va_list args;
    va_start(args, fmt);

    string result = FormatV(fmt, args);
    va_end(args);

    AddLog(result.c_str(), color);
}

//-----------------------------------------------------------------------------
// Purpose: removes lines from console with sanitized start and end indices
// Input  : nStart - 
//          nEnd - 
//-----------------------------------------------------------------------------
void CConsole::RemoveLog(int nStart, int nEnd)
{
    AUTO_LOCK(m_colorTextLoggerMutex);

    const int numLines = m_colorTextLogger.GetTotalLines();

    if (nEnd >= numLines)
    {
        // Sanitize for last array elem.
        nEnd = (numLines - 1);
    }

    if (nStart >= nEnd)
    {
        if (nEnd > 0)
        {
            nStart = (nEnd - 1);
        }
        else
        {
            // First elem cannot be removed!
            return;
        }
    }
    else if (nStart < 0)
    {
        nStart = 0;
    }

    // User wants to remove everything.
    if (numLines <= (nStart - nEnd))
    {
        ClearLog();
        return;
    }

    m_colorTextLogger.RemoveLine(nStart, nEnd);
}

//-----------------------------------------------------------------------------
// Purpose: clears the entire log vector
//-----------------------------------------------------------------------------
void CConsole::ClearLog(void)
{
    AUTO_LOCK(m_colorTextLoggerMutex);
    m_colorTextLogger.RemoveLine(0, (m_colorTextLogger.GetTotalLines() - 1));
}

//-----------------------------------------------------------------------------
// Purpose: clamps the size of the log vector
//-----------------------------------------------------------------------------
void CConsole::ClampLogSize(void)
{
    // +1 since the first row is a dummy
    const int maxLines = con_max_lines.GetInt() + 1;

    if (m_colorTextLogger.GetTotalLines() > maxLines)
    {
        while (m_colorTextLogger.GetTotalLines() > maxLines)
        {
            m_colorTextLogger.RemoveLine(0);

            m_scrollBackAmount++;
            m_selectBackAmount++;
        }

        m_colorTextLogger.MoveSelection(m_selectBackAmount, false);
        m_colorTextLogger.MoveCursor(m_selectBackAmount, false);

        m_selectBackAmount = 0;
    }
}

//-----------------------------------------------------------------------------
// Purpose: adds a command to the history vector; this is the only place text 
// is added to the vector, do not call 'm_History.push_back' elsewhere as we
// also manage the size of the vector here !!!
//-----------------------------------------------------------------------------
void CConsole::AddHistory(const char* const command)
{
    // If this command was already in the history, remove it so when we push it
    // in, it would appear all the way at the top of the list
    for (size_t i = m_vecHistory.size(); i-- > 0;)
    {
        if (m_vecHistory[i].compare(command) == 0)
        {
            m_vecHistory.erase(m_vecHistory.begin() + i);
            break;
        }
    }

    m_vecHistory.push_back(command);
    ClampHistorySize();
}

//-----------------------------------------------------------------------------
// Purpose: gets all console submissions
// Output : vector of strings
//-----------------------------------------------------------------------------
const vector<string>& CConsole::GetHistory(void) const
{
    return m_vecHistory;
}

//-----------------------------------------------------------------------------
// Purpose: clears the entire submission history vector
//-----------------------------------------------------------------------------
void CConsole::ClearHistory(void)
{
    m_vecHistory.clear();
    BuildSummaryText("");
}

//-----------------------------------------------------------------------------
// Purpose: clamps the size of the history vector
//-----------------------------------------------------------------------------
void CConsole::ClampHistorySize(void)
{
    while (m_vecHistory.size() > con_max_history.GetInt())
    {
        m_vecHistory.erase(m_vecHistory.begin());
    }
}

//-----------------------------------------------------------------------------
// Purpose: toggles the console
//-----------------------------------------------------------------------------
void CConsole::ToggleConsole_f()
{
    g_Console.m_activated ^= true;
    ResetInput(); // Disable input to game when console is drawn.
}

//-----------------------------------------------------------------------------
// Purpose: shows the game console submission history.
//-----------------------------------------------------------------------------
void CConsole::LogHistory_f()
{
    const vector<string>& vHistory = g_Console.GetHistory();
    for (size_t i = 0, nh = vHistory.size(); i < nh; i++)
    {
        Msg(eDLL_T::COMMON, "%3d: %s\n", i, vHistory[i].c_str());
    }
}

//-----------------------------------------------------------------------------
// Purpose: removes a range of lines from the console.
//-----------------------------------------------------------------------------
void CConsole::RemoveLine_f(const CCommand& args)
{
    if (args.ArgC() < 3)
    {
        Msg(eDLL_T::CLIENT, "Usage 'con_removeline': start(int) end(int)\n");
        return;
    }

    const int start = atoi(args[1]);
    const int end = atoi(args[2]);

    g_Console.RemoveLog(start, end);
}

//-----------------------------------------------------------------------------
// Purpose: clears all lines from the developer console.
//-----------------------------------------------------------------------------
void CConsole::ClearLines_f()
{
    g_Console.ClearLog();
}

//-----------------------------------------------------------------------------
// Purpose: clears all submissions from the developer console history.
//-----------------------------------------------------------------------------
void CConsole::ClearHistory_f()
{
    g_Console.ClearHistory();
}

CConsole g_Console;
