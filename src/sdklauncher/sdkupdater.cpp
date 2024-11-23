//=============================================================================//
//
// Purpose: sdk file updater
// ----------------------------------------------------------------------------
// This is actually used so that stuff like the SDK launcher (responsible for
// downloading and installing the updates) could be updated as well. It works
// by having the launcher downloading the files, install what's possible, and
// then kill itself and task this executable by replacing the last files which
// requires the launcher or anything else to be killed first.
//=============================================================================//
#include "sdkupdater.h"
#include "sdklauncher/sdklauncher_const.h"

//-----------------------------------------------------------------------------
// Purpose: prints error and returns 'EXIT_FAILURE'
// Input  : *pSymbol - 
//-----------------------------------------------------------------------------
DWORD ErrorAndExit(const char* pSymbol)
{
	printf("%s: Failed to update SDK: error code = %08x\n", pSymbol, GetLastError());
	Sleep(UPDATER_SLEEP_TIME_BEFORE_EXIT);

	return EXIT_FAILURE;
}

//----------------------------------------------------------------------------
// Purpose: clears all intermediate and deprecated files
//----------------------------------------------------------------------------
bool ClearIntermediateFiles()
{
	const bool bret = std::system("bin\\clean_sdk.bat") != NULL;
	return bret;
}

//-----------------------------------------------------------------------------
// Purpose: update process's privileges to aid with terminating other processes
//-----------------------------------------------------------------------------
void UpdatePrivilege(void)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: moves files from source to destination
// Input  : *pSourceDir - 
//        : *pDestDir   - 
//-----------------------------------------------------------------------------
void MoveFiles(const char* pSourceDir, const char* pDestDir)
{
	printf("*** Moving files from \"%s\" to \"%s\"\n", pSourceDir, pDestDir);

	char sourceDir[MAX_PATH];
	char destDir[MAX_PATH];

	V_snprintf(sourceDir, sizeof(sourceDir), "%s", pSourceDir);
	V_snprintf(destDir, sizeof(destDir), "%s", pDestDir);

	V_AppendSlash(sourceDir, sizeof(sourceDir));
	V_AppendSlash(destDir, sizeof(destDir));

	V_strcat(sourceDir, "*");

	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(sourceDir, &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		// Directory not found or unable to access.
		Assert(0);
		return;
	}

	size_t nLen = V_strlen(sourceDir);
	sourceDir[nLen-1] = '\0'; // Remove the star.

	char sourceFilePath[MAX_PATH];
	char destFilePath[MAX_PATH];

	do
	{
		const char* pFileName = findFileData.cFileName;

		if (V_strcmp(pFileName, ".") == NULL || V_strcmp(pFileName, "..") == NULL)
			continue;

		V_snprintf(sourceFilePath, sizeof(sourceFilePath), "%s%s", sourceDir, pFileName);
		V_snprintf(destFilePath, sizeof(destFilePath), "%s%s", destDir, pFileName);

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// It's a directory, so create it in the destination
			// and recursively move the contents of the directory.
			CreateDirectoryA(destFilePath, nullptr);
			MoveFiles(sourceFilePath, destFilePath);

			printf("*** Moving directory \"%s\" to \"%s\"\n", sourceFilePath, destFilePath);
		}
		else
		{
			printf("*** Moving file \"%s\" to \"%s\"\n", sourceFilePath, destFilePath);

			// It's a file, so move it to the destination folder.
			if (!MoveFileEx(sourceFilePath, destFilePath, MOVEFILE_REPLACE_EXISTING))
				printf("Failed to move file \"%s\": Error code = %08x\n", sourceFilePath, GetLastError());
		}

	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
}

//-----------------------------------------------------------------------------
// Purpose: print version numbers, warnings, etc
//-----------------------------------------------------------------------------
void PrintHeader()
{
	printf("********************************************************************************\n");
	printf("R5 updater [Version %s]\n", UPDATER_VERSION);
	printf("!!! DO NOT INTERRUPT THE APPLICATION WHILE THE UPDATE IS IN PROGRESS !!!\n");
	printf("********************************************************************************\n");
}

//-----------------------------------------------------------------------------
// Purpose: entry point
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
	PrintHeader();

	// Make sure the caller passed in a process id.
	if (argc < 1)
	{
		printf("%s: Updater must be invoked with a Process ID of the parent process!\n", "Error");
		Sleep(UPDATER_SLEEP_TIME_BEFORE_EXIT);

		return EXIT_FAILURE;
	}

	UpdatePrivilege();

	char* end; // Convert the process id back to an integral type.
	DWORD processId = strtoul(argv[0], &end, 10);
	HANDLE launcher = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processId);

	// If the launcher is still running, terminate it.
	if (launcher)
	{
		printf("*** Terminating launcher process...\n");

		// Make sure the launcher is getting terminated if
		// 'ExitProcess()' somehow failed.
		TerminateProcess(launcher, 1);

		DWORD waitResult = WaitForSingleObject(launcher, UPDATER_SLEEP_TIME_BEFORE_EXIT);

		if (waitResult == WAIT_TIMEOUT)
		{
			return ErrorAndExit("WaitForSingleObject");
		}

		CloseHandle(launcher);
	}

	// Get the current working directory, required for launching
	// the launcher again as launcher called this process, and
	// thus will use the launcher's working directory, we want
	// to launch the launcher again with this working directory.
	char currentPath[MAX_PATH];
	BOOL getResult = GetCurrentDirectoryA(sizeof(currentPath), currentPath);

	if (!getResult)
	{
		return ErrorAndExit("GetCurrentDirectory");
	}

	// Caller passed a path containing the files that should be
	// patched; install these over the game.
	if (argc > 1)
	{
		const char* destPath = (argc > 2) ? argv[2] : currentPath;
		MoveFiles(argv[1], destPath);

		if (!ClearIntermediateFiles())
		{
			printf("Failed to remove intermediate files!\n");
		}
	}

	///////////////////////////////////////////////////////////////////////////
	printf("*** Starting launcher process...\n");

	STARTUPINFOA startupInfo = { 0 };
	PROCESS_INFORMATION processInfo = { 0 };

	// Initialize startup info struct.
	startupInfo.cb = sizeof(STARTUPINFOA);

	char commandLine[256];
	V_snprintf(commandLine, sizeof(commandLine), "launcher.exe %s", "-retail");

	BOOL createResult = CreateProcessA(
		"launcher.exe",                                // lpApplicationName
		commandLine,                                   // lpCommandLine
		NULL,                                          // lpProcessAttributes
		NULL,                                          // lpThreadAttributes
		FALSE,                                         // bInheritHandles
		CREATE_SUSPENDED,                              // dwCreationFlags
		NULL,                                          // lpEnvironment
		currentPath,                                   // lpCurrentDirectory
		&startupInfo,                                  // lpStartupInfo
		&processInfo                                   // lpProcessInformation
	);

	if (!createResult)
	{
		return ErrorAndExit("CreateProcess");
	}

	///////////////////////////////////////////////////////////////////////////
	// Resume the process.
	ResumeThread(processInfo.hThread);

	///////////////////////////////////////////////////////////////////////////
	// Close the process and thread handles.
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	printf("*** Updater finished successfully.\n");
	return EXIT_SUCCESS;
}
