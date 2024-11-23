#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "imgui/misc/imgui_logger.h"
#include "imgui/misc/imgui_utility.h"

#include "imgui_surface.h"

class CConsole : public CImguiSurface
{
public:
    ///////////////////////////////////////////////////////////////////////////
    CConsole(void);
    virtual ~CConsole(void);

    virtual bool Init(void);
    virtual void Shutdown(void);

    virtual void RunFrame(void);
    virtual bool DrawSurface(void);

private:
    void DrawOptionsPanel(void);
    void DrawAutoCompletePanel(void);

    bool RunAutoComplete(void);
    void ResetAutoCompleteData(void);

    void CreateSuggestionsFromPartial(void);
    void ProcessCommand(const char* const inputText);

    void BuildSummaryText(const char* const inputText);

    struct ConAutoCompleteSuggest_s;
    void DetermineInputTextFromSelectedSuggestion(const ConAutoCompleteSuggest_s& suggest, string& svInput);
    void DetermineAutoCompleteWindowRect(void);

    bool LoadFlagIcons(void);
    int GetFlagTextureIndex(const int flags) const;

    int TextEditCallback(ImGuiInputTextCallbackData* pData);
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* pData);

    ///////////////////////////////////////////////////////////////////////////
public:
    void AddLog(const char* const text, const ImU32 color);
    void RemoveLog(int nStart, int nEnd);
    void ClearLog(void);

    void AddHistory(const char* const command);
    const vector<string>& GetHistory(void) const;
    void ClearHistory(void);

public:
    // Console command callbacks
    static void ToggleConsole_f();
    static void LogHistory_f();
    static void RemoveLine_f(const CCommand& args);
    static void ClearLines_f();
    static void ClearHistory_f();

private: // Internals.
    void HandleCommand();
    void HandleSuggest();

    void AddLog(const ImU32 color, const char* fmt, ...) /*IM_FMTARGS(2)*/;

    void ClampLogSize(void);
    void ClampHistorySize(void);

private:
    enum ConAutoCompletePos_e
    {
        // Park means the auto complete position is out of screen and not
        // indexing an item.
        kPark = -1,
    };

    struct ConAutoCompleteSuggest_s
    {
        ConAutoCompleteSuggest_s(const string& inText, const int inFlags)
        {
            text = inText;
            flags = inFlags;
        }
        bool operator==(const string& a) const
        {
            return text.compare(a) == 0;
        }
        bool operator<(const ConAutoCompleteSuggest_s& a) const
        {
            return text < a.text;
        }

        string text;
        int flags;
    };

private:
    ///////////////////////////////////////////////////////////////////////////
    ImGuiWindow*                   m_mainWindow;
    const char*                    m_loggerLabel;
    char                           m_inputTextBuf[512];
    char                           m_summaryTextBuf[256];

    // The selected ConVar from the suggestions in the autocomplete window gets
    // copied into this buffer.
    string                         m_selectedSuggestionText;
    int                            m_selectedSuggestionTextLen;

    // The positions in the history buffer; when there is nothing in the input
    // text field, the arrow keys would instead iterate over the previously
    // submitted commands and show the currently selected one right in the
    // input text field.
    int                            m_historyPos;

    // The position in the autocomplete window; this dictates the current
    // (highlighted) suggestion, and copies that one into m_selectedSuggestion
    // once selected.
    int                            m_suggestPos;

    // Scroll and select back amount; if text lines are getting removed to
    // clamp the size to the maximum allowed, we need to scroll back (and
    // if text has been selected, select back this amount) to prevent the
    // view or selection from drifting.
    int                            m_scrollBackAmount;
    int                            m_selectBackAmount;

    // Scroll position of the last drawn frame, after any scroll back has been
    // applied, this is used as a base for scrolling back when entries are
    // getting removed.
    ImVec2                         m_lastFrameScrollPos;

    // Set when the input text has been modified, used to rebuild suggestions
    // shown in the autocomplete window.
    bool                           m_inputTextBufModified;

    // Determines whether we can autocomplete and build a list of suggestions,
    // e.g. when you type "map " (with a trailing white space ' '), and "map"
    // happens to be a ConCommand with an autocomplete function, this gets set
    // and the autocomplete function of that ConCommand will be called to
    // create a list of suggestions to be shown in the autocomplete window.
    // This member is always false when the input text is empty.
    bool                           m_canAutoComplete;

    // Whether the autocomplete window is active. If this is set, the arrow up
    // and down keys will be used for the auto complete window instead of the
    // history (previously submitted commands) scroller.
    bool                           m_autoCompleteActive;

    // If the position in the autocomplete window had moved, this var will be
    // set. This is used to check if we need to adjust the scroll position in
    // the autocomplete window to keep the current selection visible.
    bool                           m_autoCompletePosMoved;

    // If the textures failed to load, this will remain false and no textures
    // will be drawn in the autocomplete window. This is because if one fails
    // to load, the indices will be incorrect.
    bool                           m_autoCompleteTexturesLoaded;

    // The position and rect of the autocomplete window, the pos is set to that
    // of the input text field + an offset to move it under the item.
    ImVec2                         m_autoCompleteWindowPos;
    ImVec2                         m_autoCompleteWindowRect;

    vector<ConAutoCompleteSuggest_s>           m_vecSuggest;
    vector<MODULERESOURCE>         m_vecFlagIcons;
    vector<string>                 m_vecHistory;

    // The color logger in which text gets added, note that the mutex lock
    // should always be acquired when using this as this is accessed from 
    // multiple threads!
    CTextLogger                    m_colorTextLogger;
    mutable CThreadFastMutex       m_colorTextLoggerMutex;
};

///////////////////////////////////////////////////////////////////////////////
extern CConsole g_Console;
#endif // !DEDICATED
