#include "core/stdafx.h"
#include "core/logdef.h"

std::shared_ptr<spdlog::logger> g_TermLogger;
std::shared_ptr<spdlog::logger> g_ImGuiLogger;

std::shared_ptr<spdlog::logger> g_SuppementalToolsLogger;

std::ostringstream g_LogStream;
std::shared_ptr<spdlog::sinks::ostream_sink_st> g_LogSink;

#ifndef _TOOLS
static void SpdLog_CreateRotatingLoggers()
{
	/************************
	 * ROTATE LOGGER SETUP  *
	 ************************/
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("squirrel_re(warning)"
		, fmt::format("{:s}/{:s}", g_LogSessionDirectory, "script_warning.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("squirrel_re"
		, fmt::format("{:s}/{:s}", g_LogSessionDirectory, "script.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk"
		, fmt::format("{:s}/{:s}", g_LogSessionDirectory, "message.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk(warning)"
		, fmt::format("{:s}/{:s}", g_LogSessionDirectory, "warning.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("sdk(error)"
		, fmt::format("{:s}/{:s}", g_LogSessionDirectory, "error.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("net_trace"
		, fmt::format("{:s}/{:s}", g_LogSessionDirectory, "net_trace.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
#ifndef DEDICATED
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("netconsole"
		, fmt::format("{:s}/{:s}", g_LogSessionDirectory, "netconsole.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
#endif // !DEDICATED
	spdlog::rotating_logger_mt<spdlog::synchronous_factory>("filesystem"
		, fmt::format("{:s}/{:s}", g_LogSessionDirectory, "filesystem.log"), SPDLOG_MAX_SIZE, SPDLOG_NUM_FILE)->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
}
#endif // !_TOOLS

#ifdef _TOOLS
// NOTE: used for tools as additional file logger on top of the existing terminal logger.
void SpdLog_InstallSupplementalLogger(const char* pszLoggerName, const char* pszLogFileName, const char* pszPattern, const bool bTruncate)
{
	g_SuppementalToolsLogger = spdlog::basic_logger_mt(pszLoggerName, pszLogFileName, bTruncate);
	g_SuppementalToolsLogger->set_pattern(pszPattern);
}
#endif // _TOOLS

//#############################################################################
// SPDLOG INIT
//#############################################################################
void SpdLog_Init(const bool bAnsiColor)
{
	static bool bInitialized = false;

	if (bInitialized)
	{
		Assert(bInitialized, "'SpdLog_Init()' has already been called.");
		return;
	}

#ifndef _TOOLS
	g_LogSessionUUID = CreateUUID();

	if (g_LogSessionUUID.empty())
	{
		// Fall-back directory in case of a failure.
		g_LogSessionUUID = "00000000-0000-0000-0000-000000000000";
	}

	g_LogSessionDirectory = fmt::format("platform/logs/{:s}", g_LogSessionUUID);
	/************************
	 * IMGUI LOGGER SETUP   *
	 ************************/
	{
		g_LogSink = std::make_shared<spdlog::sinks::ostream_sink_st>(g_LogStream);
		g_ImGuiLogger = std::make_shared<spdlog::logger>("game_console", g_LogSink);
		spdlog::register_logger(g_ImGuiLogger); // in-game console logger.
		g_ImGuiLogger->set_pattern("%v");
		g_ImGuiLogger->set_level(spdlog::level::trace);
	}
#endif // !_TOOLS
	/************************
	 * WINDOWS LOGGER SETUP *
	 ************************/
	{
#ifdef _TOOLS
		g_TermLogger = spdlog::default_logger();
#else
		g_TermLogger = spdlog::stdout_logger_mt("win_console");
#endif // _TOOLS

		// Determine if user wants ansi-color logging in the terminal.
		if (bAnsiColor)
		{
			g_TermLogger->set_pattern("%v\u001b[0m");
			g_bSpdLog_UseAnsiClr = true;
		}
		else
		{
			g_TermLogger->set_pattern("%v");
		}
		//g_TermLogger->set_level(spdlog::level::trace);
	}

#ifndef _TOOLS
	spdlog::set_default_logger(g_TermLogger); // Set as default.
	SpdLog_CreateRotatingLoggers();
#endif // !_TOOLS

	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_every(std::chrono::seconds(5));

	bInitialized = true;
}

//#############################################################################
// SPDLOG SHUTDOWN
//#############################################################################
void SpdLog_Shutdown()
{
	spdlog::shutdown();
#ifdef _TOOLS
	// Destroy the tools logger to flush it.
	g_SuppementalToolsLogger.reset();
#endif // !_TOOLS
}
