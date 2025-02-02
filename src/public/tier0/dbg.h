//========== Copyright (c) Valve Corporation, All rights reserved. ==========//
//
// Purpose:  
//
// $NoKeywords: $
//
//===========================================================================//
#ifndef DBG_H
#define DBG_H
#define AssertDbg assert
#define Verify( _exp ) ( _exp )
#include "tier0/dbgflag.h"
#include "tier0/platform.h"

// Used for the 'Error' function, this tells the function to only log, not quit.
//#define NO_ERROR 0

//-----------------------------------------------------------------------------
enum class eDLL_T : int
{
	//-------------------------------------------------------------------------
	// Script enumerants
	//-------------------------------------------------------------------------
	SCRIPT_SERVER = -3,
	SCRIPT_CLIENT = -2,
	SCRIPT_UI     = -1,

	//-------------------------------------------------------------------------
	// Native enumerants
	//-------------------------------------------------------------------------
	SERVER = 0, // server.dll                (GameDLL)
	CLIENT = 1, // client.dll                (GameDLL)
	UI     = 2, // ui.dll                    (GameDLL)
	ENGINE = 3, // engine.dll                (Wrapper)
	FS     = 4, // filesystem_stdio.dll      (FileSystem API)
	RTECH  = 5, // rtech_game.dll            (RTech API)
	MS     = 6, // materialsystem_dx11.dll   (MaterialSystem API)
	AUDIO  = 7, // binkawin64/mileswin64.dll (AudioSystem API)
	VIDEO  = 8, // bink2w64                  (VideoSystem API)
	NETCON = 9, // netconsole impl           (RCON wire)

	//-------------------------------------------------------------------------
	// Common enumerants
	//-------------------------------------------------------------------------
	COMMON         = 10, // general         (No specific subsystem)
	SYSTEM_WARNING = 11, // general warning (No specific subsystem)
	SYSTEM_ERROR   = 12, // general error   (No specific subsystem)
	NONE           = 13  // no context
};
//-----------------------------------------------------------------------------
enum class LogType_t
{
	LOG_INFO = 0,
	LOG_NET,
	LOG_WARNING,
	LOG_ERROR,
	SQ_INFO,
	SQ_WARNING
};
//-----------------------------------------------------------------------------
enum class LogLevel_t
{
	LEVEL_DISK_ONLY = 0,
	LEVEL_CONSOLE, // Emit to console panels
	LEVEL_NOTIFY   // Emit to in-game mini console
};
//-----------------------------------------------------------------------------
constexpr const char s_CommonAnsiColor[]  = "\033[38;2;255;204;153m";
constexpr const char s_WarningAnsiColor[] = "\033[38;2;255;255;000m";
constexpr const char s_ErrorAnsiColor[]   = "\033[38;2;255;000;000m";
constexpr const char s_DefaultAnsiColor[] = "\033[38;2;204;204;204m";
constexpr const char* s_DllAnsiColor[14]  =
{
	"\033[38;2;059;120;218mNative(S):",
	"\033[38;2;118;118;118mNative(C):",
	"\033[38;2;151;090;118mNative(U):",
	"\033[38;2;204;204;204mNative(E):",
	"\033[38;2;097;214;214mNative(F):",
	"\033[38;2;092;181;089mNative(R):",
	"\033[38;2;192;077;173mNative(M):",
	"\033[38;2;238;108;030mNative(A):",
	"\033[38;2;185;000;235mNative(V):",
	"\033[38;2;204;204;204mNetcon(X):",
	s_CommonAnsiColor,
	s_WarningAnsiColor,
	s_ErrorAnsiColor,
	s_DefaultAnsiColor
};
//-----------------------------------------------------------------------------
constexpr const char* s_ScriptAnsiColor[4] =
{
	"\033[38;2;151;149;187mScript(S):",
	"\033[38;2;151;149;163mScript(C):",
	"\033[38;2;151;123;136mScript(U):",
	"\033[38;2;151;149;163mScript(X):"
};

// "Native", "Script" and "Netcon" have the same length, so we can cheat here.
constexpr size_t s_ContextPrefixTextSize = sizeof("Native(X):") - 1;
constexpr size_t s_AnsiColorTextSize = sizeof(s_CommonAnsiColor) - 1;
constexpr size_t s_FullAnsiContextPrefixTextSize = s_AnsiColorTextSize + s_ContextPrefixTextSize;

//////////////////////////////////////////////////////////////////////////
// Legacy Logging System
//////////////////////////////////////////////////////////////////////////

void CoreMsgV(LogType_t logType, LogLevel_t logLevel, eDLL_T context, const char* pszLogger,
	const char* pszFormat, va_list args, const UINT exitCode = NO_ERROR, const char* pszUptimeOverride = nullptr);
void CoreMsg(LogType_t logType, LogLevel_t logLevel, eDLL_T context,
	const UINT exitCode, const char* pszLogger, const char* pszFormat, ...);

// These functions do not return.
DLL_EXPORT void Msg(eDLL_T context, const char* fmt, ...) FMTFUNCTION(2, 3);
DLL_EXPORT void NetMsg(LogType_t logType, eDLL_T context, const char* uptime, const char* fmt, ...) FMTFUNCTION(4, 5);
DLL_EXPORT void Warning(eDLL_T context, const char* fmt, ...) FMTFUNCTION(2, 3);
DLL_EXPORT void Error(eDLL_T context, const UINT code, const char* fmt, ...) FMTFUNCTION(3, 4);

// TODO[ AMOS ]: export to DLL?
void Plat_FatalError(eDLL_T context, const char* fmt, ...);

#if defined DBGFLAG_STRINGS_STRIP
#define DevMsg( ... ) ((void)0)
#define DevWarning( ... ) ((void)0)
#else // DBGFLAG_STRINGS_STRIP
DLL_EXPORT void DevMsg(eDLL_T context, const char* fmt, ...) FMTFUNCTION(2, 3);
DLL_EXPORT void DevWarning(eDLL_T context, const char* fmt, ...) FMTFUNCTION(2, 3);
#endif

// You can use this macro like a runtime assert macro.
// If the condition fails, then Error is called with the message. This macro is called
// like AssertMsg, where msg must be enclosed in parenthesis:
//
// ErrorIfNot( bCondition, ("a b c %d %d %d", 1, 2, 3) );
#define ErrorIfNot( condition, msg ) \
	if ( (condition) )		\
		;					\
	else 					\
	{						\
		Error msg;			\
	}

//-----------------------------------------------------------------------------
// Templates to assist in validating pointers:

// Have to use these stubs so we don't have to include windows.h here.
/*PLATFORM_INTERFACE*/ void _AssertValidReadPtr(void* ptr, int count = 1);
/*PLATFORM_INTERFACE*/ void _AssertValidWritePtr(void* ptr, int count = 1);
/*PLATFORM_INTERFACE*/ void _AssertValidReadWritePtr(void* ptr, int count = 1);
/*PLATFORM_INTERFACE*/ void _AssertValidStringPtr(const TCHAR* ptr, int maxchar);

#ifdef DBGFLAG_ASSERT
inline void AssertValidStringPtr(const TCHAR* ptr, int maxchar = 0xFFFFFF) { _AssertValidStringPtr(ptr, maxchar); }
template<class T> inline void AssertValidReadPtr(T* ptr, int count = 1) { _AssertValidReadPtr((void*)ptr, count); }
template<class T> inline void AssertValidWritePtr(T* ptr, int count = 1) { _AssertValidWritePtr((void*)ptr, count); }
template<class T> inline void AssertValidReadWritePtr(T* ptr, int count = 1) { _AssertValidReadWritePtr((void*)ptr, count); }
#define AssertValidThis() AssertValidReadWritePtr(this,sizeof(*this))

#else

inline void AssertValidStringPtr(const TCHAR* /*ptr*/, int maxchar = 0xFFFFFF) { NOTE_UNUSED(maxchar); }
template<class T> inline void AssertValidReadPtr(T* /*ptr*/, int count = 1) { NOTE_UNUSED(count); }
template<class T> inline void AssertValidWritePtr(T* /*ptr*/, int count = 1) { NOTE_UNUSED(count); }
template<class T> inline void AssertValidReadWritePtr(T* /*ptr*/, int count = 1) { NOTE_UNUSED(count); }
#define AssertValidThis() 
#endif

//-----------------------------------------------------------------------------
// Macro to protect functions that are not reentrant

#ifdef _DEBUG
class CReentryGuard
{
public:
	CReentryGuard(int* pSemaphore)
		: m_pSemaphore(pSemaphore)
	{
		++(*m_pSemaphore);
	}

	~CReentryGuard()
	{
		--(*m_pSemaphore);
	}

private:
	int* m_pSemaphore;
};

#define ASSERT_NO_REENTRY() \
	static int fSemaphore##__LINE__; \
	Assert( !fSemaphore##__LINE__ ); \
	CReentryGuard ReentryGuard##__LINE__( &fSemaphore##__LINE__ )
#else
#define ASSERT_NO_REENTRY()
#endif

#define AssertMsg(condition, ...) assert(condition)

typedef void (*CoreMsgVCallbackSink_t)(LogType_t logType, LogLevel_t logLevel, eDLL_T context,
	const char* pszLogger, const char* pszFormat, va_list args, const UINT exitCode, const char* pszUptimeOverride);

extern CoreMsgVCallbackSink_t g_CoreMsgVCallback;

#endif /* DBG_H */
