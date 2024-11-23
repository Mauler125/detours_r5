//=============================================================================//
//
// Purpose: SDK launcher implementation.
//
//=============================================================================//
#include "core/logger.h"
#include "core/logdef.h"
#include "tier0/cpu.h"
#include "tier0/binstream.h"
#include "tier1/fmtstr.h"
#include "advanced_surface.h"
#include "sdklauncher.h"
#include "sdklauncher_utils.h"
#include "base_surface.h"

#include "windows/console.h"
#include "vstdlib/keyvaluessystem.h"
#include "filesystem/filesystem_std.h"

static CKeyValuesSystem s_KeyValuesSystem;
static CFileSystem_Stdio s_FullFileSystem;
static CLauncher s_Launcher;

///////////////////////////////////////////////////////////////////////////////
// Purpose: keyvalues singleton accessor
///////////////////////////////////////////////////////////////////////////////
IKeyValuesSystem* KeyValuesSystem()
{
    return &s_KeyValuesSystem;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: filesystem singleton accessor
///////////////////////////////////////////////////////////////////////////////
CFileSystem_Stdio* FileSystem()
{
    return &s_FullFileSystem;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: launcher singleton accessor.
///////////////////////////////////////////////////////////////////////////////
CLauncher* SDKLauncher()
{
    return &s_Launcher;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: launcher logger sink
///////////////////////////////////////////////////////////////////////////////
void LauncherLoggerSink(LogType_t logType, LogLevel_t logLevel, eDLL_T context,
    const char* pszLogger, const char* pszFormat, va_list args,
    const UINT exitCode /*= NO_ERROR*/, const char* pszUptimeOverride /*= nullptr*/)
{
    const string buffer = FormatV(pszFormat, args);

    SDKLauncher()->AddLog(logType, buffer.c_str());
    EngineLoggerSink(logType, logLevel, context, pszLogger, pszFormat, args, exitCode, pszUptimeOverride);
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: initializes and runs the user interface
///////////////////////////////////////////////////////////////////////////////
void CLauncher::RunSurface()
{
    Forms::Application::EnableVisualStyles();
    UIX::UIXTheme::InitializeRenderer(new Themes::KoreTheme());

    //m_pSurface = new CAdvancedSurface();
    //m_pSurface->Init();

    m_pBaseSurface = new CBaseSurface();
    

    Forms::Application::Run(m_pBaseSurface, true);
    UIX::UIXTheme::ShutdownRenderer();
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: initializes the launcher
///////////////////////////////////////////////////////////////////////////////
void CLauncher::Init()
{
    g_CoreMsgVCallback = &LauncherLoggerSink; // Setup logger callback sink.

    // Init time.
    Plat_FloatTime();
    SpdLog_Init(true);

    CUtlString logFile;
    string logUUID = CreateUUID();
    if (logUUID.empty())
    {
        logUUID = "00000000-0000-0000-0000-000000000000";
    }

    logFile.Format(SDK_LAUNCHER_LOG_DIR"\\%s\\sdk_launcher.log", logUUID.c_str());

    SpdLog_InstallSupplementalLogger("sdk_launcher_logger_mt", logFile.Get());
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: de-initializes the launcher
///////////////////////////////////////////////////////////////////////////////
void CLauncher::Shutdown(uint32_t exitCode)
{
    SpdLog_Shutdown();
    ExitProcess(exitCode);
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: adds a log to the surface console
///////////////////////////////////////////////////////////////////////////////
void CLauncher::AddLog(const LogType_t level, const char* const szText)
{
    if (m_pSurface)
    {
        m_pSurface->AddLog(level, szText);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: handles user input pre-init
// Input  : argc - 
//          *argv[] - 
// Output : exit_code (-1 if EntryPoint should continue to HandleInput)
///////////////////////////////////////////////////////////////////////////////
int CLauncher::HandleCommandLine(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];
        eLaunchMode mode = eLaunchMode::LM_NONE;

        if ((arg == "-developer") || (arg == "-dev"))
        {
            mode = eLaunchMode::LM_GAME_DEV;
        }
        else if ((arg == "-retail") || (arg == "-prod"))
        {
            mode = eLaunchMode::LM_GAME;
        }
        else if ((arg == "-server_dev") || (arg == "-svd"))
        {
            mode = eLaunchMode::LM_SERVER_DEV;
        }
        else if ((arg == "-server") || (arg == "-sv"))
        {
            mode = eLaunchMode::LM_SERVER;
        }
        else if ((arg == "-client_dev") || (arg == "-cld"))
        {
            mode = eLaunchMode::LM_CLIENT_DEV;
        }
        else if ((arg == "-client") || (arg == "-cl"))
        {
            mode = eLaunchMode::LM_CLIENT;
        }
       
        if (mode != eLaunchMode::LM_NONE)
        {
            if (CreateLaunchContext(mode) && LaunchProcess())
            {
                return EXIT_SUCCESS;
            }
        }
    }

    return -1;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: handles user input post-init
// Output : exit_code
///////////////////////////////////////////////////////////////////////////////
int CLauncher::HandleInput()
{
    Msg(eDLL_T::NONE, "--------------------------------------------------------------------------------------------------------------\n");
    Warning(eDLL_T::COMMON, "The '%s' options are for development purposes; use the '%s' options for default usage.\n", "DEV", "PROD");
    Msg(eDLL_T::NONE, "--------------------------------------------------------------------------------------------------------------\n");
    Msg(eDLL_T::COMMON, "%-6s ('0' = %s | '1' = %s).\n", "GAME", "DEV", "PROD");
    Msg(eDLL_T::COMMON, "%-6s ('2' = %s | '3' = %s).\n", "SERVER", "DEV", "PROD");
    Msg(eDLL_T::COMMON, "%-6s ('4' = %s | '5' = %s).\n", "CLIENT", "DEV", "PROD");
    Msg(eDLL_T::NONE, "--------------------------------------------------------------------------------------------------------------\n");
    std::cout << "User input: ";

    std::string input;
    if (std::cin >> input)
    {
        try
        {
            eLaunchMode mode = static_cast<eLaunchMode>(std::stoi(input));

            if (CreateLaunchContext(mode) && LaunchProcess())
            {
                return EXIT_SUCCESS;
            }
            else
            {
                Error(eDLL_T::COMMON, 0, "Invalid mode (range 0-5).\n");
                Sleep(2500);

                return EXIT_FAILURE;
            }
        }
        catch (const std::exception& e)
        {
            Error(eDLL_T::COMMON, 0, "SDK Launcher only takes numerical input (error = %s).\n", e.what());
            Sleep(2500);

            return EXIT_FAILURE;
        }
    }

    Error(eDLL_T::COMMON, 0, "SDK Launcher requires numerical input.\n");
    Sleep(2500);

    return EXIT_FAILURE;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: create launch context.
// Input  : lMode - 
//          lState - 
//          *szCommandLine - 
// Output : true on success, false otherwise.
///////////////////////////////////////////////////////////////////////////////
bool CLauncher::CreateLaunchContext(eLaunchMode lMode, uint64_t nProcessorAffinity /*= NULL*/, const char* const szCommandLine /*= nullptr*/, const char* szConfig /*= nullptr*/)
{
    ///////////////////////////////////////////////////////////////////////////
    const char* szGameDLL = nullptr;
    const char* szContext = nullptr;
    const char* szLevel = nullptr;

    std::function<void(const char*, const char*,
        const char*, const char*)> fnSetup = [&](
        const char* gameDLL, const char* context,
            const char* level, const char* config)
    {
        szGameDLL = gameDLL;
        szContext = context;
        szLevel = level;

        // Only set fall-back config
        // if not already set.
        if (!szConfig)
        {
            szConfig = config;
        }
    };

    m_ProcessorAffinity = nProcessorAffinity;

    switch (lMode)
    {
    case eLaunchMode::LM_GAME_DEV:
    {
        fnSetup(MAIN_GAME_DLL, "GAME",
            "DEV", "startup_dev.cfg");
        break;
    }
    case eLaunchMode::LM_GAME:
    {
        fnSetup(MAIN_GAME_DLL, "GAME",
            "PROD", "startup_retail.cfg");
        break;
    }
    case eLaunchMode::LM_SERVER_DEV:
    {
        fnSetup(SERVER_GAME_DLL, "SERVER",
            "DEV", "startup_dedi_dev.cfg");
        break;
    }
    case eLaunchMode::LM_SERVER:
    {
        fnSetup(SERVER_GAME_DLL, "SERVER",
            "PROD", "startup_dedi_retail.cfg");
        break;
    }
    case eLaunchMode::LM_CLIENT_DEV:
    {
        fnSetup(MAIN_GAME_DLL, "CLIENT",
            "DEV", "startup_client_dev.cfg");
        break;
    }
    case eLaunchMode::LM_CLIENT:
    {
        fnSetup(MAIN_GAME_DLL, "CLIENT",
            "PROD", "startup_client_retail.cfg");
        break;
    }
    default:
    {
        Error(eDLL_T::COMMON, 0, "No launch mode specified.\n");
        return false;
    }
    }

    SetupLaunchContext(szConfig, szGameDLL, szCommandLine);
    Msg(eDLL_T::COMMON, "*** LAUNCHING %s [%s] ***\n", szContext, szLevel);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: setup launch context.
// Input  : *szConfig      - 
//          *szWorkerDll   - 
//          *szGameDll     - 
//          *szCommandLine - 
///////////////////////////////////////////////////////////////////////////////
void CLauncher::SetupLaunchContext(const char* const szConfig, const char* const szGameDll, const char* const szCommandLine)
{    
    CFmtStrN<1024> cfgFileName;
    CUtlBuffer commandLineBuff(ssize_t(0),0, CUtlBuffer::TEXT_BUFFER);

    if (szConfig && szConfig[0])
    {
        cfgFileName.Format(GAME_CFG_PATH"%s", szConfig);

        if (!FileSystem()->ReadFile(cfgFileName.String(), nullptr, commandLineBuff))
        {
            Error(eDLL_T::COMMON, 0, "Failed to open file '%s'!\n", szConfig);
        }
    }

    if (szCommandLine && szCommandLine[0])
    {
        commandLineBuff.PutString(szCommandLine);
    }

    m_svGameDll = Format("%s\\%s", m_svCurrentDir.c_str(), szGameDll);
    m_svCmdLine = Format("%s\\%s %s", m_svCurrentDir.c_str(), szGameDll, commandLineBuff.Base());

    ///////////////////////////////////////////////////////////////////////////
    // Print the file paths and arguments.
    Msg(eDLL_T::NONE, "--------------------------------------------------------------------------------------------------------------\n");
    Msg(eDLL_T::COMMON, "- CWD: %s\n", m_svCurrentDir.c_str());
    Msg(eDLL_T::COMMON, "- EXE: %s\n", m_svGameDll.c_str());
    Msg(eDLL_T::COMMON, "- CLI: %s\n", commandLineBuff.Base());
    Msg(eDLL_T::NONE, "--------------------------------------------------------------------------------------------------------------\n");
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: launches the game with results from the setup
// Output : true on success, false otherwise
///////////////////////////////////////////////////////////////////////////////
bool CLauncher::LaunchProcess() const
{
    ///////////////////////////////////////////////////////////////////////////
    STARTUPINFOA StartupInfo = { 0 };
    PROCESS_INFORMATION ProcInfo = { 0 };

    // Initialize startup info struct.
    StartupInfo.cb = sizeof(STARTUPINFOA);


    ///////////////////////////////////////////////////////////////////////////
    // Create the game process in a suspended state with our dll.
    BOOL createResult = CreateProcessA(
        m_svGameDll.c_str(),                           // lpApplicationName
        (LPSTR)m_svCmdLine.c_str(),                    // lpCommandLine
        NULL,                                          // lpProcessAttributes
        NULL,                                          // lpThreadAttributes
        FALSE,                                         // bInheritHandles
        CREATE_SUSPENDED,                              // dwCreationFlags
        NULL,                                          // lpEnvironment
        m_svCurrentDir.c_str(),                        // lpCurrentDirectory
        &StartupInfo,                                  // lpStartupInfo
        &ProcInfo                                      // lpProcessInformation
    );

    ///////////////////////////////////////////////////////////////////////////
    // Failed to create the process.
    if (createResult)
    {
        if (m_ProcessorAffinity)
        {
            BOOL affinityResult = SetProcessAffinityMask(ProcInfo.hProcess, m_ProcessorAffinity);
            if (!affinityResult)
            {
                // Just print the result, don't return, as
                // the process was created successfully.
                PrintLastError();
            }
        }
    }
    else
    {
        PrintLastError();
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Resume the process.
    ResumeThread(ProcInfo.hThread);

    ///////////////////////////////////////////////////////////////////////////
    // Close the process and thread handles.
    CloseHandle(ProcInfo.hProcess);
    CloseHandle(ProcInfo.hThread);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: Window enumerator callback.
// Input  : hwnd   -
//          lParam - 
// Output : TRUE on success, FALSE otherwise.
///////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    vector<HWND>* pHandles = reinterpret_cast<vector<HWND>*>(lParam);
    if (!pHandles)
    {
        return FALSE;
    }

    char szClassName[256];
    if (!GetClassNameA(hwnd, szClassName, 256))
    {
        return FALSE;
    }

    if (strcmp(szClassName, DEFAULT_WINDOW_CLASS_NAME) == 0)
    {
        pHandles->push_back(hwnd);
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// EntryPoint.
///////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
    CheckSystemCPUForSSE2();

    // Only 1 instance at a time.
    if (SDKLauncher_ForceExistingInstanceOnTop())
    {
        return EXIT_SUCCESS;
    }

    int exitCode = EXIT_SUCCESS;

    if (__argc < 2)
    {
#ifdef _DEBUG
        if(!Console_Init(true))
            return EXIT_FAILURE;
#endif // _DEBUG
        SDKLauncher()->Init();
        SDKLauncher()->RunSurface();
#ifdef _DEBUG
        exitCode = !Console_Shutdown();
#endif // _DEBUG
        SDKLauncher()->Shutdown(exitCode);
    }
    else
    {
#ifdef _DEBUG
        if (!Console_Init(true))
            return EXIT_FAILURE;
#endif // _DEBUG

        SDKLauncher()->Init();

        exitCode = SDKLauncher()->HandleCommandLine(__argc, __argv);

        if (exitCode == -1)
            exitCode = SDKLauncher()->HandleInput();

#ifdef _DEBUG
        //Make sure that the consoles exit code doesnt overwrite a failure from handle input        
        exitCode = max(exitCode, static_cast<int>(!Console_Shutdown()));
#endif // _DEBUG

        SDKLauncher()->Shutdown(exitCode);
    }
#if _DEBUG
    return exitCode;
#endif // _DEBUG
}
