#include "tier0/utility.h"
#ifndef _TOOLS
#include "tier0/commandline.h"
#endif // !_TOOLS
#include "init.h"
#include "logdef.h"
#include "logger.h"
#ifndef DEDICATED
#include "vgui/vgui_debugpanel.h"
#include "gameui/IConsole.h"
#endif // !DEDICATED
#ifndef CLIENT_DLL
#include "engine/server/sv_rcon.h"
#endif // !CLIENT_DLL
#ifndef _TOOLS
#include "vscript/languages/squirrel_re/include/sqstdaux.h"
#endif // !_TOOLS
static const std::regex s_AnsiRowRegex("\\\033\\[.*?m");
static std::mutex s_LogMutex;

#if !defined (DEDICATED) && !defined (_TOOLS)
ImVec4 CheckForWarnings(LogType_t type, eDLL_T context, const ImVec4& defaultCol)
{
	ImVec4 color = defaultCol;
	if (type == LogType_t::LOG_WARNING || context == eDLL_T::SYSTEM_WARNING)
	{
		color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f);
	}
	else if (type == LogType_t::LOG_ERROR || context == eDLL_T::SYSTEM_ERROR)
	{
		color = ImVec4(1.00f, 0.00f, 0.00f, 0.80f);
	}

	return color;
}

ImVec4 GetColorForContext(LogType_t type, eDLL_T context)
{
	switch (context)
	{
	case eDLL_T::SCRIPT_SERVER:
		return CheckForWarnings(type, context, ImVec4(0.59f, 0.58f, 0.73f, 1.00f));
	case eDLL_T::SCRIPT_CLIENT:
		return CheckForWarnings(type, context, ImVec4(0.59f, 0.58f, 0.63f, 1.00f));
	case eDLL_T::SCRIPT_UI:
		return CheckForWarnings(type, context, ImVec4(0.59f, 0.48f, 0.53f, 1.00f));
	case eDLL_T::SERVER:
		return CheckForWarnings(type, context, ImVec4(0.23f, 0.47f, 0.85f, 1.00f));
	case eDLL_T::CLIENT:
		return CheckForWarnings(type, context, ImVec4(0.46f, 0.46f, 0.46f, 1.00f));
	case eDLL_T::UI:
		return CheckForWarnings(type, context, ImVec4(0.59f, 0.35f, 0.46f, 1.00f));
	case eDLL_T::ENGINE:
		return CheckForWarnings(type, context, ImVec4(0.70f, 0.70f, 0.70f, 1.00f));
	case eDLL_T::FS:
		return CheckForWarnings(type, context, ImVec4(0.32f, 0.64f, 0.72f, 1.00f));
	case eDLL_T::RTECH:
		return CheckForWarnings(type, context, ImVec4(0.36f, 0.70f, 0.35f, 1.00f));
	case eDLL_T::MS:
		return CheckForWarnings(type, context, ImVec4(0.75f, 0.30f, 0.68f, 1.00f));
	case eDLL_T::AUDIO:
		return CheckForWarnings(type, context, ImVec4(0.93f, 0.42f, 0.12f, 1.00f));
	case eDLL_T::VIDEO:
		return CheckForWarnings(type, context, ImVec4(0.73f, 0.00f, 0.92f, 1.00f));
	case eDLL_T::NETCON:
		return CheckForWarnings(type, context, ImVec4(0.81f, 0.81f, 0.81f, 1.00f));
	case eDLL_T::COMMON:
		return CheckForWarnings(type, context, ImVec4(1.00f, 0.80f, 0.60f, 1.00f));
	default:
		return CheckForWarnings(type, context, ImVec4(0.81f, 0.81f, 0.81f, 1.00f));
	}
}
#endif // !DEDICATED && !_TOOLS

static const char* GetContextNameByIndex(eDLL_T context, size_t& numTotalChars, size_t& numAnsiChars, const bool ansiColor)
{
	const int index = static_cast<int>(context);
	const char* contextName;

	switch (context)
	{
	case eDLL_T::SCRIPT_SERVER:
		contextName = s_ScriptAnsiColor[0];
		numTotalChars = s_FullAnsiContextPrefixTextSize;
		break;
	case eDLL_T::SCRIPT_CLIENT:
		contextName = s_ScriptAnsiColor[1];
		numTotalChars = s_FullAnsiContextPrefixTextSize;
		break;
	case eDLL_T::SCRIPT_UI:
		contextName = s_ScriptAnsiColor[2];
		numTotalChars = s_FullAnsiContextPrefixTextSize;
		break;
	case eDLL_T::SERVER:
	case eDLL_T::CLIENT:
	case eDLL_T::UI:
	case eDLL_T::ENGINE:
	case eDLL_T::FS:
	case eDLL_T::RTECH:
	case eDLL_T::MS:
	case eDLL_T::AUDIO:
	case eDLL_T::VIDEO:
	case eDLL_T::NETCON:
	case eDLL_T::COMMON:
	case eDLL_T::SYSTEM_WARNING:
	case eDLL_T::SYSTEM_ERROR:
		contextName = s_DllAnsiColor[index];
		numTotalChars = context >= eDLL_T::COMMON ? s_AnsiColorTextSize : s_FullAnsiContextPrefixTextSize;
		break;
	default:
		contextName = s_DefaultAnsiColor;
		numTotalChars = s_AnsiColorTextSize;
		break;
	}

	if (!ansiColor)
	{
		// Shift # chars to skip ANSI row.
		contextName += s_AnsiColorTextSize;
		numTotalChars -= s_AnsiColorTextSize;
	}
	else
		numAnsiChars = s_AnsiColorTextSize;

	return contextName;
}

bool LoggedFromClient(eDLL_T context)
{
#ifndef DEDICATED
	return (context == eDLL_T::CLIENT || context == eDLL_T::SCRIPT_CLIENT
		|| context == eDLL_T::UI || context == eDLL_T::SCRIPT_UI
		|| context == eDLL_T::NETCON);
#else
	NOTE_UNUSED(context);
	return false;
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: Show logs to all console interfaces (va_list version)
// Input  : logType - 
//			logLevel - 
//			context - 
//			*pszLogger - 
//			*pszFormat -
//			args - 
//			exitCode - 
//			*pszUptimeOverride - 
//-----------------------------------------------------------------------------
void EngineLoggerSink(LogType_t logType, LogLevel_t logLevel, eDLL_T context,
	const char* pszLogger, const char* pszFormat, va_list args,
	const UINT exitCode /*= NO_ERROR*/, const char* pszUptimeOverride /*= nullptr*/)
{
	const char* pszUpTime = pszUptimeOverride ? pszUptimeOverride : Plat_GetProcessUpTime();
	string message(pszUpTime);

	const size_t contextTextStartIndex = message.length();

	const bool bToConsole = (logLevel >= LogLevel_t::LEVEL_CONSOLE);
	const bool bUseColor = (bToConsole && g_bSpdLog_UseAnsiClr);

	size_t numTotalContextTextChars = 0;
	size_t numAnsiContextChars = 0;

	const char* pszContext = GetContextNameByIndex(context, numTotalContextTextChars, numAnsiContextChars, bUseColor);
	message.append(pszContext, numTotalContextTextChars);

#if !defined (DEDICATED) && !defined (_TOOLS)
	ImVec4 overlayColor = GetColorForContext(logType, context);
	eDLL_T overlayContext = context;
#endif // !DEDICATED && !_TOOLS

#if !defined (_TOOLS)
	bool bSquirrel = false;
	bool bWarning = false;
	bool bError = false;
#else
	NOTE_UNUSED(pszLogger);
#endif // !_TOOLS

	const size_t messageTextStartIndex = message.length();
	size_t numMessageAnsiChars = 0;

	//-------------------------------------------------------------------------
	// Setup logger and context
	//-------------------------------------------------------------------------
	switch (logType)
	{
	case LogType_t::LOG_WARNING:
#if !defined (DEDICATED) && !defined (_TOOLS)
		overlayContext = eDLL_T::SYSTEM_WARNING;
#endif // !DEDICATED && !_TOOLS
		if (bUseColor)
		{
			message.append(g_svYellowF);
			numMessageAnsiChars = g_svYellowF.length();
		}
		break;
	case LogType_t::LOG_ERROR:
#if !defined (DEDICATED) && !defined (_TOOLS)
		overlayContext = eDLL_T::SYSTEM_ERROR;
#endif // !DEDICATED && !_TOOLS
		if (bUseColor)
		{
			message.append(g_svRedF);
			numMessageAnsiChars = g_svRedF.length();
		}
		break;
#ifndef _TOOLS
	case LogType_t::SQ_INFO:
		bSquirrel = true;
		break;
	case LogType_t::SQ_WARNING:
#ifndef DEDICATED
		overlayContext = eDLL_T::SYSTEM_WARNING;
		overlayColor = ImVec4(1.00f, 1.00f, 0.00f, 0.80f);
#endif // !DEDICATED
		bSquirrel = true;
		bWarning = true;
		break;
#endif // !_TOOLS
	default:
		break;
	}

	//-------------------------------------------------------------------------
	// Format actual input
	//-------------------------------------------------------------------------
	va_list argsCopy;
	va_copy(argsCopy, args);
	const string formatted = FormatV(pszFormat, argsCopy);
	va_end(argsCopy);

#ifndef _TOOLS
	//-------------------------------------------------------------------------
	// Colorize script warnings and errors
	//-------------------------------------------------------------------------
	if (bToConsole && bSquirrel)
	{
		if (bWarning && g_bSQAuxError)
		{
			if (formatted.find("SCRIPT ERROR:") != string::npos ||
				formatted.find(" -> ") != string::npos)
			{
				bError = true;
			}
		}
		else if (g_bSQAuxBadLogic)
		{
			if (formatted.find("There was a problem processing game logic.") != string::npos)
			{
				bError = true;
				g_bSQAuxBadLogic = false;
			}
		}

		// Append warning/error color before appending the formatted text,
		// so that this gets marked as such while preserving context colors.
		if (bError)
		{
#ifndef DEDICATED
			overlayContext = eDLL_T::SYSTEM_ERROR;
			overlayColor = ImVec4(1.00f, 0.00f, 0.00f, 0.80f);
#endif // !DEDICATED

			if (bUseColor)
			{
				if (logType != LogType_t::LOG_ERROR)
				{
					if (numMessageAnsiChars > 0)
						message.replace(messageTextStartIndex, numMessageAnsiChars, g_svRedF);
					else
						message.append(g_svRedF);

					numMessageAnsiChars = g_svRedF.length();
				}
			}
		}
		else if (bUseColor && bWarning)
		{
			if (logType != LogType_t::LOG_ERROR)
			{
				if (numMessageAnsiChars > 0)
					message.replace(messageTextStartIndex, numMessageAnsiChars, g_svYellowF);
				else
					message.append(g_svYellowF);

				numMessageAnsiChars = g_svYellowF.length();
			}
		}
	}
#endif // !_TOOLS
	message.append(formatted);

	//-------------------------------------------------------------------------
	// Emit to all interfaces
	//-------------------------------------------------------------------------
	std::lock_guard<std::mutex> lock(s_LogMutex);
	if (bToConsole)
	{
		g_TermLogger->debug(message);

		// Remove ANSI rows if we have them, before emitting to file or over wire.
		if (bUseColor)
		{
			// Start with the message first because else the indices will shift.
			// The message colors comes after the context colors.
			if (numMessageAnsiChars > 0)
			{
				message.erase(messageTextStartIndex, numMessageAnsiChars);
				numMessageAnsiChars = 0;
			}

			if (numAnsiContextChars > 0)
			{
				message.erase(contextTextStartIndex, numAnsiContextChars);
				numAnsiContextChars = 0;
			}

			// Remove anything else that was passed in as a format argument.
			message = std::regex_replace(message, s_AnsiRowRegex, "");
		}
	}

	// If a debugger is attached, emit the text there too
	if (Plat_IsInDebugSession())
		Plat_DebugString(message.c_str());

#ifndef _TOOLS
	// Output is always logged to the file.
	std::shared_ptr<spdlog::logger> ntlogger = spdlog::get(pszLogger); // <-- Obtain by 'pszLogger'.
	assert(ntlogger.get() != nullptr);

	if (ntlogger)
		ntlogger->debug(message);

	if (bToConsole)
	{
#ifndef CLIENT_DLL
		if (!LoggedFromClient(context) && RCONServer()->ShouldSend(netcon::response_e::SERVERDATA_RESPONSE_CONSOLE_LOG))
		{
			RCONServer()->SendEncoded(formatted.c_str(), pszUpTime, netcon::response_e::SERVERDATA_RESPONSE_CONSOLE_LOG,
				int(context), int(logType));
		}
#endif // !CLIENT_DLL
#ifndef DEDICATED
		g_Console.AddLog(message.c_str(), ImGui::ColorConvertFloat4ToU32(overlayColor));

		// We can only log to the in-game overlay console when the SDK has
		// been fully initialized, due to the use of ConVar's.
		if (g_bSdkInitialized && logLevel >= LogLevel_t::LEVEL_NOTIFY)
		{
			// Draw to mini console.
			g_TextOverlay.AddLog(overlayContext, message.c_str());
		}
#endif // !DEDICATED
	}

#else
	if (g_SuppementalToolsLogger)
	{
		g_SuppementalToolsLogger->debug(message);
	}
#endif

	if (exitCode) // Terminate the process if an exit code was passed.
	{
#ifndef _TOOLS
		if (!CommandLine()->CheckParm("-nomessagebox"))
#endif // !_TOOLS
		{
			MessageBoxA(NULL, Format("%s- %s", pszUpTime, formatted.c_str()).c_str(), "SDK Error", MB_ICONERROR | MB_OK);
		}
		TerminateProcess(GetCurrentProcess(), exitCode);
	}
}
