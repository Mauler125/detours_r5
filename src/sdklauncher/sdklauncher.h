#pragma once
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "base_surface.h"
#include "advanced_surface.h"

class CLauncher
{
public:
    CLauncher()
    {
        m_pSurface = nullptr;
        m_ProcessorAffinity = NULL;
		m_svCurrentDir = fs::current_path().u8string();
    }
    ~CLauncher()
    {
    }

    void RunSurface();

    void Init();
    void Shutdown(uint32_t exitCode = EXIT_SUCCESS);

    void AddLog(const LogType_t level, const char* const szText);

    int HandleCommandLine(int argc, char* argv[]);
    int HandleInput();

    bool CreateLaunchContext(eLaunchMode lMode, uint64_t nProcessorAffinity = NULL, const char* const szCommandLine = nullptr, const char* szConfig = nullptr);
    void SetupLaunchContext(const char* const szConfig, const char* const szGameDll, const char* const szCommandLine);
    bool LaunchProcess() const;

    eLaunchMode BuildParameter(string& parameterList) { return m_pSurface->BuildParameter(parameterList); }

private:
    CAdvancedSurface* m_pSurface;
    CBaseSurface* m_pBaseSurface;

    uint64_t m_ProcessorAffinity;

    string m_svGameDll;
    string m_svCmdLine;
    string m_svCurrentDir;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

extern CLauncher* SDKLauncher();
