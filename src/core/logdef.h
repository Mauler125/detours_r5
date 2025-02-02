#pragma once

#include <sstream>
#include "thirdparty/spdlog/spdlog.h"
#include "thirdparty/spdlog/async.h"
#include "thirdparty/spdlog/sinks/ostream_sink.h"
#include "thirdparty/spdlog/sinks/basic_file_sink.h"
#include "thirdparty/spdlog/sinks/stdout_sinks.h"
#include "thirdparty/spdlog/sinks/stdout_color_sinks.h"
#include "thirdparty/spdlog/sinks/ansicolor_sink.h"
#include "thirdparty/spdlog/sinks/rotating_file_sink.h"

constexpr int SPDLOG_MAX_SIZE = 10 * 1024 * 1024; // Sets number of bytes before rotating logger.
constexpr int SPDLOG_NUM_FILE = 512; // Sets number of files to rotate to.

inline bool g_bSpdLog_UseAnsiClr = false;

extern std::shared_ptr<spdlog::logger> g_TermLogger;
extern std::shared_ptr<spdlog::logger> g_ImGuiLogger;

#ifdef _TOOLS
extern std::shared_ptr<spdlog::logger> g_SuppementalToolsLogger;
#endif // _TOOLS

void SpdLog_Init(const bool bAnsiColor);
void SpdLog_Shutdown(void);

#ifdef _TOOLS
void SpdLog_InstallSupplementalLogger(const char* pszLoggerName, const char* pszLogFileName,
	const char* pszPattern = "[%Y-%m-%d %H:%M:%S.%e] %v", const bool bTruncate = true);
#endif // _TOOLS
