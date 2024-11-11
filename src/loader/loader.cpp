//===========================================================================//
// 
// Purpose: SDK loader stub
// 
// --------------------------------------------------------------------------
// The game module cannot be imported directly by the executable, as the
// internal memalloc system is not initialized before the entry point is
// called. So we need to load the SDK dll after the entry point is called,
// but before LauncherMain is called.
// 
// The executable exports table has been restructured; the exported function
// 'GetDenuvoTimeTicketRequest' has been swapped with 'CreateGlobalMemAlloc',
// the exported 'IEngineAPI' interface accessor has been replaced with
// 'g_pMemAllocSingleton', so we can obtain the addresses without hardcoding.
// 
// These changes allow us to load the SDK in the following order:
// - Create game process
// - Import this loader stub by its dummy export.
// - Immediately hook 'LauncherMain' by getting the address from the exports.
// - Determine if, and which SDK module to load.
// 
// Since LauncherMain is called before anything of the game is, we can still
// hook and modify anything of the game before it starts. With this order of
// initialization, we can now replace the standard memalloc system with that
// of the game, by:
// 
// - Redefining the standard C functions to use the internal memalloc system.
// - Checking if the memalloc system has been initialized, and create if not.
//===========================================================================//
#include "loader.h"
#include "tier0/module.h"

//-----------------------------------------------------------------------------
// Image statics
//-----------------------------------------------------------------------------
static const PEB64* s_ProcessEnvironmentBlock = nullptr;
static const IMAGE_DOS_HEADER* s_DosHeader = nullptr;
static const IMAGE_NT_HEADERS64* s_NtHeaders = nullptr;
static HMODULE s_SdkModule = NULL;

typedef void (*InitFunc)(void);
static InitFunc s_SdkInitFunc = NULL;
static InitFunc s_SdkShutdownFunc = NULL;

//-----------------------------------------------------------------------------
// LauncherMain function pointer
//-----------------------------------------------------------------------------
static int (*v_LauncherMain)(HINSTANCE, HINSTANCE, LPSTR, int) = nullptr;

//-----------------------------------------------------------------------------
// Purpose: Terminates the process with an error when called
//-----------------------------------------------------------------------------
static void FatalError(const char* fmt, ...)
{
	va_list vArgs;
	va_start(vArgs, fmt);

	char errorBuf[1024];
	vsnprintf(errorBuf, sizeof(errorBuf), fmt, vArgs);

	errorBuf[sizeof(errorBuf) - 1] = '\0';
	va_end(vArgs);

	MessageBoxA(NULL, errorBuf, "Loader Error", MB_ICONERROR | MB_OK);
	TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
}

//-----------------------------------------------------------------------------
// Purpose: Loads the SDK module
//-----------------------------------------------------------------------------
static void InitGameSDK(const LPSTR lpCmdLine)
{
	if (V_strstr(lpCmdLine, "-noworkerdll"))
		return;

	char moduleName[MAX_PATH];

	if (!GetModuleFileNameA((HMODULE)s_DosHeader,
		moduleName, sizeof(moduleName)))
		return;

	// Prune the path.
	const char* pModuleName = strrchr(moduleName, '\\') + 1;
	const bool bDedicated = V_stricmp(pModuleName, SERVER_GAME_DLL) == NULL;

	// The dedicated server has its own SDK module,
	// so we need to check whether we are running
	// the base game or the dedicated server.
	if (!bDedicated)
	{
		// Load the client dll if '-noserverdll' is passed,
		// as this command lime parameter prevents the
		// server dll from initializing in the engine.
		if (V_strstr(lpCmdLine, "-noserverdll"))
			s_SdkModule = LoadLibraryA(CLIENT_WORKER_DLL);
		else
			s_SdkModule = LoadLibraryA(MAIN_WORKER_DLL);
	}
	else
		s_SdkModule = LoadLibraryA(SERVER_WORKER_DLL);

	if (!s_SdkModule)
	{
		Assert(0);
		FatalError("Failed to load SDK: error code = %08x\n", GetLastError());

		return;
	}

	s_SdkInitFunc = (InitFunc)GetProcAddress(s_SdkModule, "SDK_Init");
	if (s_SdkInitFunc)
		s_SdkShutdownFunc = (InitFunc)GetProcAddress(s_SdkModule, "SDK_Shutdown");

	if (!s_SdkInitFunc || !s_SdkShutdownFunc)
	{
		Assert(0);
		FatalError("Loaded SDK is invalid: error code = %08x\n", GetLastError());

		return;
	}

	s_SdkInitFunc();
}

//-----------------------------------------------------------------------------
// Purpose: Unloads the SDK module
//-----------------------------------------------------------------------------
static void ShutdownGameSDK()
{
	if (s_SdkModule)
	{
		s_SdkShutdownFunc();

		FreeLibrary(s_SdkModule);
		s_SdkModule = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: LauncherMain hook; loads the SDK before the game inits
//-----------------------------------------------------------------------------
int WINAPI hLauncherMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	InitGameSDK(lpCmdLine); // Init GameSDK, internal function calls LauncherMain.
	const int ret = v_LauncherMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
	ShutdownGameSDK();

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: hooks the entry point
//-----------------------------------------------------------------------------
static void AttachEP()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&v_LauncherMain, &hLauncherMain);

	HRESULT hr = DetourTransactionCommit();
	if (hr != NO_ERROR) // Failed to hook into the process, terminate...
	{
		Assert(0);
		FatalError("Failed to detour process: error code = %08x\n", hr);
	}
}

//-----------------------------------------------------------------------------
// Purpose: unhooks the entry point
//-----------------------------------------------------------------------------
static void DetachEP()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach(&v_LauncherMain, &hLauncherMain);
	HRESULT hr = DetourTransactionCommit();

	Assert(hr != NO_ERROR);
	NOTE_UNUSED(hr);
}

//-----------------------------------------------------------------------------
// Purpose: APIENTRY
//-----------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		s_ProcessEnvironmentBlock = CModule::GetProcessEnvironmentBlock();
		s_DosHeader = (IMAGE_DOS_HEADER*)s_ProcessEnvironmentBlock->ImageBaseAddress;
		s_NtHeaders = (IMAGE_NT_HEADERS64*)((uintptr_t)s_DosHeader
			+ (uintptr_t)s_DosHeader->e_lfanew);

		v_LauncherMain = CModule::GetExportedSymbol((QWORD)s_DosHeader, "LauncherMain")
			.RCast<int (*)(HINSTANCE, HINSTANCE, LPSTR, int)>();

		AttachEP();
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		DetachEP();
		break;
	}
	}

	return TRUE;
}
