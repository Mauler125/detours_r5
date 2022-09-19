#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier1/cvar.h"
#include "filesystem/basefilesystem.h"
#include "filesystem/filesystem.h"
#ifndef DEDICATED
#include "gameui/IConsole.h"
#endif // !DEDICATED

//---------------------------------------------------------------------------------
// Purpose: prints the output of the filesystem based on the warning level
// Input  : *this - 
//			level - 
//			*pFmt - 
//---------------------------------------------------------------------------------
void CBaseFileSystem::Warning(CBaseFileSystem* pFileSystem, FileWarningLevel_t level, const char* pFmt, ...)
{
	if (fs_warning_level_sdk->GetInt() < static_cast<int>(level))
	{
		return;
	}

	static char szBuf[1024] = {};

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> fslogger = spdlog::get("fs_warn");

	{/////////////////////////////
		va_list args{};
		va_start(args, pFmt);

		vsnprintf(szBuf, sizeof(szBuf), pFmt, args);

		szBuf[sizeof(szBuf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	fslogger->debug(szBuf);

	if (fs_show_warning_output->GetBool())
	{
		wconsole->debug(szBuf);
#ifndef DEDICATED
		iconsole->debug(szBuf);
		g_pConsole->AddLog(ConLog_t(g_spd_sys_w_oss.str(), ImVec4(1.00f, 1.00f, 0.00f, 1.00f)));

		g_spd_sys_w_oss.str("");
		g_spd_sys_w_oss.clear();
#endif // !DEDICATED
	}
}

//---------------------------------------------------------------------------------
// Purpose: attempts to load files from disk if exist before loading from VPK
// Input  : *pVpk - 
//			*pResults - 
//			*pszFilePath - 
// Output : Handle to file on success, NULL on failure
//---------------------------------------------------------------------------------
FileHandle_t CBaseFileSystem::VReadFromVPK(CBaseFileSystem* pFileSystem, FileHandle_t pResults, char* pszFilePath)
{
	std::string svFilePath = ConvertToWinPath(pszFilePath);

	if (svFilePath.find("\\\*\\") != string::npos)
	{
		// Erase '//*/'.
		svFilePath.erase(0, 4);
	}

	// TODO: obtain 'mod' SearchPath's instead.
	svFilePath.insert(0, "platform\\");

	if (::FileExists(svFilePath) /*|| ::FileExists(pszFilePath)*/)
	{
		*reinterpret_cast<int64_t*>(pResults) = -1;
		return pResults;
	}
	return CBaseFileSystem_VLoadFromVPK(pFileSystem, pResults, pszFilePath);
}

//---------------------------------------------------------------------------------
// Purpose: attempts to load files from disk if exist before loading from cache
// Input  : *pFileSystem - 
//			*pszFilePath - 
//			*pResults - 
// Output : true if file exists, false otherwise
//---------------------------------------------------------------------------------
bool CBaseFileSystem::VReadFromCache(CBaseFileSystem* pFileSystem, char* pszFilePath, void* pResults)
{
	std::string svFilePath = ConvertToWinPath(pszFilePath);

	if (svFilePath.find("\\\*\\") != string::npos)
	{
		// Erase '//*/'.
		svFilePath.erase(0, 4);
	}

	// TODO: obtain 'mod' SearchPath's instead.
	svFilePath.insert(0, "platform\\");

	if (::FileExists(svFilePath) /*|| ::FileExists(pszFilePath)*/)
	{
		return false;
	}
	return CBaseFileSystem_VLoadFromCache(pFileSystem, pszFilePath, pResults);
}

void CBaseFileSystem_Attach()
{
	DetourAttach((LPVOID*)&CBaseFileSystem_Warning, &CBaseFileSystem::Warning);
	DetourAttach((LPVOID*)&CBaseFileSystem_VLoadFromVPK, &CBaseFileSystem::VReadFromVPK);
	DetourAttach((LPVOID*)&CBaseFileSystem_VLoadFromCache, &CBaseFileSystem::VReadFromCache);
}

void CBaseFileSystem_Detach()
{
	DetourDetach((LPVOID*)&CBaseFileSystem_Warning, &CBaseFileSystem::Warning);
	DetourDetach((LPVOID*)&CBaseFileSystem_VLoadFromVPK, &CBaseFileSystem::VReadFromVPK);
	DetourDetach((LPVOID*)&CBaseFileSystem_VLoadFromCache, &CBaseFileSystem::VReadFromCache);
}
CBaseFileSystem* g_pFileSystem = nullptr;