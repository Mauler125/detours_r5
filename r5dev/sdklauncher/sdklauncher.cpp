#include "core/stdafx.h"
#include "basepanel.h"
#include "sdklauncher_const.h"
#include "sdklauncher.h"

///////////////////////////////////////////////////////////////////////////////
// Purpose: initializes the user interface
///////////////////////////////////////////////////////////////////////////////
void CLauncher::InitSurface()
{
    Forms::Application::EnableVisualStyles();
    UIX::UIXTheme::InitializeRenderer(new Themes::KoreTheme());

    g_pLauncher->m_pSurface = new CUIBaseSurface();
    Forms::Application::Run(g_pLauncher->m_pSurface);
    UIX::UIXTheme::ShutdownRenderer();
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: initializes the console (release builds only)
///////////////////////////////////////////////////////////////////////////////
void CLauncher::InitConsole()
{
    AllocConsole();
    freopen("conin$", "r", stdin);
    freopen("conout$", "w", stdout);
    freopen("conout$", "w", stderr);
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: initializes the logger
///////////////////////////////////////////////////////////////////////////////
void CLauncher::InitLogger()
{
    wconsole->set_pattern("[%^%l%$] %v");
    wconsole->set_level(spdlog::level::trace);
    spdlog::set_default_logger(wconsole); // Set as default.
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: handles user input pre-init
// Input  : argc - 
//          *argv - 
// Output : exit_code (-1 if EP should continue to HandleInput)
///////////////////////////////////////////////////////////////////////////////
int CLauncher::HandleCmdLine(int argc, char* argv[])
{
    for (int i = 1; i < __argc; ++i)
    {
        std::string arg = __argv[i];
        if ((arg == "-debug") || (arg == "-dbg"))
        {
            if (g_pLauncher->Setup(eLaunchMode::LM_HOST_DEBUG, eLaunchState::LS_CHEATS))
            {
                if (g_pLauncher->Launch())
                {
                    Sleep(2000);
                    return EXIT_SUCCESS;
                }
            }

            Sleep(2000);
            return EXIT_FAILURE;
        }
        if ((arg == "-release") || (arg == "-rel"))
        {
            if (g_pLauncher->Setup(eLaunchMode::LM_HOST, eLaunchState::LS_CHEATS))
            {
                if (g_pLauncher->Launch())
                {
                    Sleep(2000);
                    return EXIT_SUCCESS;
                }
            }

            Sleep(2000);
            return EXIT_FAILURE;
        }
        if ((arg == "-dedicated_dev") || (arg == "-dedid"))
        {
            if (g_pLauncher->Setup(eLaunchMode::LM_SERVER_DEBUG, eLaunchState::LS_CHEATS))
            {
                if (g_pLauncher->Launch())
                {
                    Sleep(2000);
                    return EXIT_SUCCESS;
                }
            }

            Sleep(2000);
            return EXIT_FAILURE;
        }
        if ((arg == "-dedicated") || (arg == "-dedi"))
        {
            if (g_pLauncher->Setup(eLaunchMode::LM_SERVER, eLaunchState::LS_CHEATS))
            {
                if (g_pLauncher->Launch())
                {
                    Sleep(2000);
                    return EXIT_SUCCESS;
                }
            }

            Sleep(2000);
            return EXIT_FAILURE;
        }
        if ((arg == "-client_dev") || (arg == "-cld"))
        {
            if (g_pLauncher->Setup(eLaunchMode::LM_CLIENT_DEBUG, eLaunchState::LS_CHEATS))
            {
                if (g_pLauncher->Launch())
                {
                    Sleep(2000);
                    return EXIT_SUCCESS;
                }
            }

            Sleep(2000);
            return EXIT_FAILURE;
        }
        if ((arg == "-client") || (arg == "-cl"))
        {
            if (g_pLauncher->Setup(eLaunchMode::LM_CLIENT, eLaunchState::LS_CHEATS))
            {
                if (g_pLauncher->Launch())
                {
                    Sleep(2000);
                    return EXIT_SUCCESS;
                }
            }

            Sleep(2000);
            return EXIT_FAILURE;
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
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    g_pLauncher->AddLog(spdlog::level::level_enum::warn, "If a DEBUG option has been chosen as launch parameter, do not broadcast servers to the Server Browser!\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::warn, "All FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY ConVar's/ConCommand's will be enabled.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::warn, "Connected clients will be able to set and execute anything flagged FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY.\n");
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    g_pLauncher->AddLog(spdlog::level::level_enum::warn, "Use DEBUG HOST        [0] for research and development purposes.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::warn, "Use RELEASE HOST      [1] for playing the game and creating servers.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::warn, "Use DEBUG SERVER      [2] for research and development purposes.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::warn, "Use RELEASE SERVER    [3] for running and hosting dedicated servers.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::warn, "Use DEBUG CLIENT      [4] for research and development purposes.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::warn, "Use RELEASE CLIENT    [5] for running client only builds against remote servers.\n");
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    g_pLauncher->AddLog(spdlog::level::level_enum::info, "Enter '0' for 'DEBUG HOST'.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::info, "Enter '1' for 'RELEASE HOST'.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::info, "Enter '2' for 'DEBUG SERVER'.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::info, "Enter '3' for 'RELEASE SERVER'.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::info, "Enter '4' for 'DEBUG CLIENT'.\n");
    g_pLauncher->AddLog(spdlog::level::level_enum::info, "Enter '5' for 'RELEASE CLIENT'.\n");
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "User input: ";

    std::string input = std::string();
    if (std::cin >> input)
    {
        try
        {
            eLaunchMode mode = static_cast<eLaunchMode>(std::stoi(input));
            switch (mode)
            {
            case eLaunchMode::LM_HOST_DEBUG:
            {
                if (g_pLauncher->Setup(mode, eLaunchState::LS_CHEATS))
                {
                    if (g_pLauncher->Launch())
                    {
                        Sleep(2000);
                        return EXIT_SUCCESS;
                    }
                }

                Sleep(2000);
                return EXIT_FAILURE;
            }
            case eLaunchMode::LM_HOST:
            {
                if (g_pLauncher->Setup(mode, eLaunchState::LS_CHEATS))
                {
                    if (g_pLauncher->Launch())
                    {
                        Sleep(2000);
                        return EXIT_SUCCESS;
                    }
                }

                Sleep(2000);
                return EXIT_FAILURE;
            }
            case eLaunchMode::LM_SERVER_DEBUG:
            {
                if (g_pLauncher->Setup(mode, eLaunchState::LS_CHEATS))
                {
                    if (g_pLauncher->Launch())
                    {
                        Sleep(2000);
                        return EXIT_SUCCESS;
                    }
                }

                Sleep(2000);
                return EXIT_FAILURE;
            }
            case eLaunchMode::LM_SERVER:
            {
                if (g_pLauncher->Setup(mode, eLaunchState::LS_CHEATS))
                {
                    if (g_pLauncher->Launch())
                    {
                        Sleep(2000);
                        return EXIT_SUCCESS;
                    }
                }

                Sleep(2000);
                return EXIT_FAILURE;
            }
            case eLaunchMode::LM_CLIENT_DEBUG:
            {
                if (g_pLauncher->Setup(mode, eLaunchState::LS_CHEATS))
                {
                    if (g_pLauncher->Launch())
                    {
                        Sleep(2000);
                        return EXIT_SUCCESS;
                    }
                }

                Sleep(2000);
                return EXIT_FAILURE;
            }
            case eLaunchMode::LM_CLIENT:
            {
                if (g_pLauncher->Setup(mode, eLaunchState::LS_CHEATS))
                {
                    if (g_pLauncher->Launch())
                    {
                        Sleep(2000);
                        return EXIT_SUCCESS;
                    }
                }

                Sleep(2000);
                return EXIT_FAILURE;
            }
            default:
            {
                g_pLauncher->AddLog(spdlog::level::level_enum::err, "Invalid mode (range 0-5).\n");
                Sleep(2000);
                return EXIT_FAILURE;
            }
            }
        }
        catch (std::exception& e)
        {
            g_pLauncher->AddLog(spdlog::level::level_enum::err, "SDK Launcher only takes numerical input. Error: {:s}.\n", e.what());
            Sleep(2000);
            return EXIT_FAILURE;
        }
    }
    g_pLauncher->AddLog(spdlog::level::level_enum::err, "SDK Launcher requires numerical input.\n");

    Sleep(2000);
    return EXIT_FAILURE;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: setup for game dll's and configurations
// Input  : lMode - 
//          lState - 
// Output : true on success, false otherwise
///////////////////////////////////////////////////////////////////////////////
bool CLauncher::Setup(eLaunchMode lMode, eLaunchState lState)
{
    ///////////////////////////////////////////////////////////////////////////
    std::string svCmdLineArgs              = std::string();

    ///////////////////////////////////////////////////////////////////////////
    switch (lMode)
    {
    case eLaunchMode::LM_HOST_DEBUG:
    {
        fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_debug.cfg";
        std::ifstream cfgFile(cfgPath);
        if (cfgFile.good() && cfgFile)
        {
            std::stringstream ss;
            ss << cfgFile.rdbuf();
            svCmdLineArgs = ss.str() + "-launcher";
        }
        else
        {
            AddLog(spdlog::level::level_enum::err, "File 'platform\\cfg\\startup_debug.cfg' does not exist!\n");
            return false;
        }

        m_svWorkerDll = m_svCurrentDir + "\\gamesdk.dll";
        m_svGameExe   = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine   = m_svCurrentDir + "\\r5apex.exe " + svCmdLineArgs;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING GAME [DEBUG] ***\n");
        break;
    }
    case eLaunchMode::LM_HOST:
    {
        fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_retail.cfg";
        std::ifstream cfgFile(cfgPath);
        if (cfgFile.good() && cfgFile)
        {
            std::stringstream ss;
            ss << cfgFile.rdbuf();
            svCmdLineArgs = ss.str() + "-launcher";
        }
        else
        {
            AddLog(spdlog::level::level_enum::err, "File 'platform\\cfg\\startup_retail.cfg' does not exist!\n");
            return false;
        }

        m_svWorkerDll = m_svCurrentDir + "\\gamesdk.dll";
        m_svGameExe   = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine   = m_svCurrentDir + "\\r5apex.exe " + svCmdLineArgs;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING GAME [RELEASE] ***\n");
        break;
    }
    case eLaunchMode::LM_SERVER_DEBUG:
    {
        fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_dedi_debug.cfg";
        std::ifstream cfgFile(cfgPath);
        if (cfgFile.good() && cfgFile)
        {
            std::stringstream ss;
            ss << cfgFile.rdbuf();
            svCmdLineArgs = ss.str() + "-launcher";
        }
        else
        {
            AddLog(spdlog::level::level_enum::err, "File 'platform\\cfg\\startup_dedi_debug.cfg' does not exist!\n");
            return false;
        }

        m_svWorkerDll = m_svCurrentDir + "\\dedicated.dll";
        m_svGameExe   = m_svCurrentDir + "\\r5apex_ds.exe";
        m_svCmdLine   = m_svCurrentDir + "\\r5apex_ds.exe " + svCmdLineArgs;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING DEDICATED [DEBUG] ***\n");
        break;
    }
    case eLaunchMode::LM_SERVER:
    {
        fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_dedi_retail.cfg";
        std::ifstream cfgFile(cfgPath);
        if (cfgFile.good() && cfgFile)
        {
            std::stringstream ss;
            ss << cfgFile.rdbuf();
            svCmdLineArgs = ss.str(); +"-launcher";
        }
        else
        {
            AddLog(spdlog::level::level_enum::err, "File 'platform\\cfg\\startup_dedi_retail.cfg' does not exist!\n");
            return false;
        }

        m_svWorkerDll = m_svCurrentDir + "\\dedicated.dll";
        m_svGameExe   = m_svCurrentDir + "\\r5apex_ds.exe";
        m_svCmdLine   = m_svCurrentDir + "\\r5apex_ds.exe " + svCmdLineArgs;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING DEDICATED [RELEASE] ***\n");
        break;
    }
    case eLaunchMode::LM_CLIENT_DEBUG:
    {
        fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_client_debug.cfg";
        std::ifstream cfgFile(cfgPath);
        if (cfgFile.good() && cfgFile)
        {
            std::stringstream ss;
            ss << cfgFile.rdbuf();
            svCmdLineArgs = ss.str(); +"-launcher";
        }
        else
        {
            AddLog(spdlog::level::level_enum::err, "File 'platform\\cfg\\startup_client_debug.cfg' does not exist!\n");
            return false;
        }

        m_svWorkerDll = m_svCurrentDir + "\\bin\\x64_retail\\client.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex.exe " + svCmdLineArgs;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING CLIENT [DEBUG] ***\n");
        break;
    }
    case eLaunchMode::LM_CLIENT:
    {
        fs::path cfgPath = fs::current_path() /= "platform\\cfg\\startup_client_retail.cfg";
        std::ifstream cfgFile(cfgPath);
        if (cfgFile.good() && cfgFile)
        {
            std::stringstream ss;
            ss << cfgFile.rdbuf();
            svCmdLineArgs = ss.str(); +"-launcher";
        }
        else
        {
            AddLog(spdlog::level::level_enum::err, "File 'platform\\cfg\\startup_client_retail.cfg' does not exist!\n");
            return false;
        }

        m_svWorkerDll = m_svCurrentDir + "\\bin\\x64_retail\\client.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex.exe " + svCmdLineArgs;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHING CLIENT [RELEASE] ***\n");
        break;
    }
    default:
    {
        AddLog(spdlog::level::level_enum::err, "*** NO LAUNCH MODE SPECIFIED ***\n");
        return false;
    }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Print the file paths and arguments.
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    g_pLauncher->AddLog(spdlog::level::level_enum::debug, "- CWD: {:s}\n", m_svCurrentDir);
    g_pLauncher->AddLog(spdlog::level::level_enum::debug, "- EXE: {:s}\n", m_svGameExe);
    g_pLauncher->AddLog(spdlog::level::level_enum::debug, "- DLL: {:s}\n", m_svWorkerDll);
    g_pLauncher->AddLog(spdlog::level::level_enum::debug, "- CLI: {:s}\n", svCmdLineArgs);
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: setup for game dll's and configurations
// Input  : lMode - 
//          &svCommandLine - 
// Output : true on success, false otherwise
///////////////////////////////////////////////////////////////////////////////
bool CLauncher::Setup(eLaunchMode lMode, const string& svCommandLine)
{
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    switch (lMode)
    {
    case eLaunchMode::LM_HOST_DEBUG:
    {
        m_svWorkerDll = m_svCurrentDir + "\\gamesdk.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex.exe " + svCommandLine;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHER SETUP FOR HOST [DEBUG] ***\n");
        break;
    }
    case eLaunchMode::LM_HOST:
    {
        m_svWorkerDll = m_svCurrentDir + "\\gamesdk.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex.exe " + svCommandLine;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHER SETUP FOR HOST [RELEASE] ***\n");
        break;
    }
    case eLaunchMode::LM_SERVER_DEBUG:
    {
        m_svWorkerDll = m_svCurrentDir + "\\dedicated.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex_ds.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex_ds.exe " + svCommandLine;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHER SETUP FOR DEDICATED [DEBUG] ***\n");
        break;
    }
    case eLaunchMode::LM_SERVER:
    {
        m_svWorkerDll = m_svCurrentDir + "\\dedicated.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex_ds.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex_ds.exe " + svCommandLine;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHER SETUP FOR DEDICATED [RELEASE] ***\n");
        break;
    }
    case eLaunchMode::LM_CLIENT_DEBUG:
    {
        m_svWorkerDll = m_svCurrentDir + "\\bin\\x64_retail\\client.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex.exe " + svCommandLine;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHER SETUP FOR CLIENT [DEBUG] ***\n");
        break;
    }
    case eLaunchMode::LM_CLIENT:
    {
        m_svWorkerDll = m_svCurrentDir + "\\bin\\x64_retail\\client.dll";
        m_svGameExe = m_svCurrentDir + "\\r5apex.exe";
        m_svCmdLine = m_svCurrentDir + "\\r5apex.exe " + svCommandLine;

        AddLog(spdlog::level::level_enum::info, "*** LAUNCHER SETUP FOR CLIENT [RELEASE] ***\n");
        break;
    }
    default:
    {
        AddLog(spdlog::level::level_enum::err, "*** INVALID LAUNCH MODE SPECIFIED ***\n");
        return false;
    }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Print the file paths and arguments.
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;
    g_pLauncher->AddLog(spdlog::level::level_enum::debug, "- CWD: {:s}\n", m_svCurrentDir);
    g_pLauncher->AddLog(spdlog::level::level_enum::debug, "- EXE: {:s}\n", m_svGameExe);
    g_pLauncher->AddLog(spdlog::level::level_enum::debug, "- DLL: {:s}\n", m_svWorkerDll);
    g_pLauncher->AddLog(spdlog::level::level_enum::debug, "- CLI: {:s}\n", svCommandLine);
    std::cout << "----------------------------------------------------------------------------------------------------------------------" << std::endl;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Purpose: launches the game with results from the setup
// Output : true on success, false otherwise
///////////////////////////////////////////////////////////////////////////////
bool CLauncher::Launch() const
{
    ///////////////////////////////////////////////////////////////////////////
    // Build our list of dlls to inject.
    LPCSTR DllsToInject[1] =
    {
        m_svWorkerDll.c_str()
    };

    STARTUPINFOA StartupInfo = { 0 };
    PROCESS_INFORMATION ProcInfo = { 0 };

    // Initialize startup info struct.
    StartupInfo.cb = sizeof(STARTUPINFOA);

    ///////////////////////////////////////////////////////////////////////////
    // Create the game process in a suspended state with our dll.
    BOOL result = DetourCreateProcessWithDllsA
    (
        m_svGameExe.c_str(),                           // lpApplicationName
        (LPSTR)m_svCmdLine.c_str(),                    // lpCommandLine
        NULL,                                          // lpProcessAttributes
        NULL,                                          // lpThreadAttributes
        FALSE,                                         // bInheritHandles
        CREATE_SUSPENDED,                              // dwCreationFlags
        NULL,                                          // lpEnvironment
        m_svCurrentDir.c_str(),                        // lpCurrentDirectory
        &StartupInfo,                                  // lpStartupInfo
        &ProcInfo,                                     // lpProcessInformation
        sizeof(DllsToInject) / sizeof(LPCSTR),         // nDlls
        DllsToInject,                                  // rlpDlls
        NULL                                           // pfCreateProcessA
    );

    ///////////////////////////////////////////////////////////////////////////
    // Failed to create the process.
    if (!result)
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
// Entrypoint.
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[], char* envp[])
{
    g_pLauncher->InitLogger();
    if (__argc < 2)
    {
        FreeConsole();
        g_pLauncher->InitSurface();
    }
    else
    {
        int results = g_pLauncher->HandleCmdLine(__argc, __argv);
        if (results != -1)
            return results;

        return g_pLauncher->HandleInput();
    }
    return EXIT_SUCCESS;
}
