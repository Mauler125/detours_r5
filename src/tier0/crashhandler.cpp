//=============================================================================//
//
// Purpose: Crash handler (overrides the game's implementation!)
//
//=============================================================================//
#include "tier0/binstream.h"
#include "tier0/cpu.h"
#include "tier0/crashhandler.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::Start()
{
	AcquireSRWLockExclusive(&m_Lock);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::End()
{
	ReleaseSRWLockExclusive(&m_Lock);
}

//-----------------------------------------------------------------------------
// Purpose: formats the crasher (module, address and exception)
//-----------------------------------------------------------------------------
void CCrashHandler::FormatCrash()
{
	m_Buffer.Append("crash:\n{\n");

	FormatExceptionAddress();
	FormatExceptionCode();

	m_Buffer.Append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats the captured callstack
//-----------------------------------------------------------------------------
void CCrashHandler::FormatCallstack()
{
	m_Buffer.Append("callstack:\n{\n");

	if (m_nCapturedFrames)
	{
		const PEXCEPTION_RECORD pExceptionRecord = m_pExceptionPointers->ExceptionRecord;

		if (m_ppStackTrace[m_nCapturedFrames - 1] == pExceptionRecord->ExceptionAddress)
		{
			const PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;
			MEMORY_BASIC_INFORMATION mbi = { 0 };

			const SIZE_T t = VirtualQuery((LPCVOID)pContextRecord->Rsp, &mbi, sizeof(LPCVOID));

			if (t >= sizeof(mbi)
				&& !(mbi.Protect & PAGE_NOACCESS)
				&& (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE))
				&& (mbi.State & MEM_COMMIT))
			{
				m_Buffer.Append("\t// call stack ended; possible return address?\n");
			}
		}
	}
	for (WORD i = 0; i < m_nCapturedFrames; i++)
	{
		FormatExceptionAddress(reinterpret_cast<LPCSTR>(m_ppStackTrace[i]));
	}

	m_Buffer.Append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats all the registers and their contents
//-----------------------------------------------------------------------------
void CCrashHandler::FormatRegisters()
{
	m_Buffer.Append("registers:\n{\n");
	const PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;

	FormatALU("rax", pContextRecord->Rax);
	FormatALU("rbx", pContextRecord->Rbx);
	FormatALU("rcx", pContextRecord->Rcx);
	FormatALU("rdx", pContextRecord->Rdx);
	FormatALU("rsp", pContextRecord->Rsp);
	FormatALU("rbp", pContextRecord->Rbp);
	FormatALU("rsi", pContextRecord->Rsi);
	FormatALU("rdi", pContextRecord->Rdi);
	FormatALU("r8 ", pContextRecord->R8);
	FormatALU("r9 ", pContextRecord->R9);
	FormatALU("r10", pContextRecord->R10);
	FormatALU("r11", pContextRecord->R11);
	FormatALU("r12", pContextRecord->R12);
	FormatALU("r13", pContextRecord->R13);
	FormatALU("r14", pContextRecord->R14);
	FormatALU("r15", pContextRecord->R15);
	FormatALU("rip", pContextRecord->Rip);

	FormatFPU("xmm0 ", &pContextRecord->Xmm0);
	FormatFPU("xmm1 ", &pContextRecord->Xmm1);
	FormatFPU("xmm2 ", &pContextRecord->Xmm2);
	FormatFPU("xmm3 ", &pContextRecord->Xmm3);
	FormatFPU("xmm4 ", &pContextRecord->Xmm4);
	FormatFPU("xmm5 ", &pContextRecord->Xmm5);
	FormatFPU("xmm6 ", &pContextRecord->Xmm6);
	FormatFPU("xmm7 ", &pContextRecord->Xmm7);
	FormatFPU("xmm8 ", &pContextRecord->Xmm8);
	FormatFPU("xmm9 ", &pContextRecord->Xmm9);
	FormatFPU("xmm10", &pContextRecord->Xmm10);
	FormatFPU("xmm11", &pContextRecord->Xmm11);
	FormatFPU("xmm12", &pContextRecord->Xmm12);
	FormatFPU("xmm13", &pContextRecord->Xmm13);
	FormatFPU("xmm14", &pContextRecord->Xmm14);
	FormatFPU("xmm15", &pContextRecord->Xmm15);

	m_Buffer.Append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats all loaded modules (verbose)
//-----------------------------------------------------------------------------
void CCrashHandler::FormatModules()
{
	m_Buffer.Append("modules:\n{\n");

	const HANDLE hProcess = GetCurrentProcess();

	DWORD cbNeeded;
	const BOOL result = K32EnumProcessModulesEx(hProcess, m_ppModuleHandles, MAX_MODULE_HANDLES, &cbNeeded, LIST_MODULES_ALL);

	if (result && cbNeeded <= MAX_MODULE_HANDLES && cbNeeded >> 3)
	{
		CHAR szModuleName[MAX_FILEPATH];
		LPSTR pszModuleName;
		MODULEINFO modInfo;

		for (DWORD i = 0, j = cbNeeded >> 3; j; i++, j--)
		{
			const DWORD m = GetModuleFileNameA(m_ppModuleHandles[i], szModuleName, sizeof(szModuleName));

			if ((m - 1) > (sizeof(szModuleName) - 2)) // Too small for buffer.
			{
				snprintf(szModuleName, sizeof(szModuleName), "module@%p", m_ppModuleHandles[i]);
				pszModuleName = szModuleName;
			}
			else
			{
				pszModuleName = strrchr(szModuleName, '\\') + 1;
			}

			K32GetModuleInformation(hProcess, m_ppModuleHandles[i], &modInfo, sizeof(modInfo));

			m_Buffer.AppendFormat("\t%-15s: [%p, %p]\n", 
				pszModuleName, modInfo.lpBaseOfDll, (reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll) + modInfo.SizeOfImage));
		}
	}

	m_Buffer.Append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats the system information
//-----------------------------------------------------------------------------
void CCrashHandler::FormatSystemInfo()
{
	m_Buffer.Append("system:\n{\n");

	const CPUInformation& pi = GetCPUInformation();

	m_Buffer.AppendFormat("\tcpu_model = \"%s\"\n", pi.m_szProcessorBrand);
	m_Buffer.AppendFormat("\tcpu_speed = %010lld // clock cycles\n", pi.m_Speed);

	DISPLAY_DEVICE& dd = m_HardWareInfo.displayDevice;

	for (DWORD i = 0; ; i++)
	{
		const BOOL f = EnumDisplayDevices(NULL, i, &dd, EDD_GET_DEVICE_INTERFACE_NAME);

		if (!f)
		{
			break;
		}

		if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) // The primary device is the only relevant device.
		{
			m_Buffer.AppendFormat("\tgpu_model = \"%s\"\n", dd.DeviceString);
			m_Buffer.AppendFormat("\tgpu_flags = 0x%08X // primary device\n", dd.StateFlags);

			break;
		}
	}

	MEMORYSTATUSEX& statex = m_HardWareInfo.memoryStatus;

	if (GlobalMemoryStatusEx(&statex))
	{
		m_Buffer.AppendFormat("\tram_total = [%.2lf, %.2lf] // physical/virtual (MiB)\n", (f64)(statex.ullTotalPhys / (1024.0 * 1024.0)), (f64)(statex.ullTotalVirtual / (1024.0 * 1024.0)));
		m_Buffer.AppendFormat("\tram_avail = [%.2lf, %.2lf] // physical/virtual (MiB)\n", (f64)(statex.ullAvailPhys / (1024.0 * 1024.0)), (f64)(statex.ullAvailVirtual / (1024.0 * 1024.0)));
	}

	DWORD sectorsPerCluster, bytesPerSector, freeClusters, totalClusters;

	if (GetDiskFreeSpaceA(NULL, &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters))
	{
		m_HardWareInfo.totalDiskSpace = (u64)totalClusters * sectorsPerCluster * bytesPerSector;
		m_HardWareInfo.availDiskSpace = (u64)freeClusters * sectorsPerCluster * bytesPerSector;

		m_Buffer.AppendFormat("\tdsk_total = %.2lf // (MiB)\n", (f64)(m_HardWareInfo.totalDiskSpace / (1024.0 * 1024.0)));
		m_Buffer.AppendFormat("\tdsk_avail = %.2lf // (MiB)\n", (f64)(m_HardWareInfo.availDiskSpace / (1024.0 * 1024.0)));
	}

	m_Buffer.Append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats the build information
//-----------------------------------------------------------------------------
void CCrashHandler::FormatBuildInfo()
{
	m_Buffer.AppendFormat("build_id: %u\n", g_SDKDll.GetNTHeaders()->FileHeader.TimeDateStamp);
}

//-----------------------------------------------------------------------------
// Purpose: formats the module, address and exception
//-----------------------------------------------------------------------------
void CCrashHandler::FormatExceptionAddress()
{
	FormatExceptionAddress(static_cast<LPCSTR>(m_pExceptionPointers->ExceptionRecord->ExceptionAddress));
}

//-----------------------------------------------------------------------------
// Purpose: formats the module, address and exception
// Input  : pExceptionAddress - 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatExceptionAddress(const LPCSTR pExceptionAddress)
{
	HMODULE hCrashedModule;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, pExceptionAddress, &hCrashedModule))
	{
		m_Buffer.AppendFormat("\t!!!unknown-module!!!: %p\n", pExceptionAddress);
		m_nCrashMsgFlags = 0; // Display the "unknown DLL or EXE" message.
		return;
	}

	const LPCSTR pModuleBase = reinterpret_cast<LPCSTR>(pExceptionAddress - reinterpret_cast<LPCSTR>(hCrashedModule));

	CHAR szCrashedModuleFullName[MAX_PATH];
	if (GetModuleFileNameExA(GetCurrentProcess(), hCrashedModule, szCrashedModuleFullName, sizeof(szCrashedModuleFullName)) - 1 > 0x1FE)
	{
		m_Buffer.AppendFormat("\tmodule@%p: %p\n", (void*)hCrashedModule, pModuleBase);
		m_nCrashMsgFlags = 2; // Display the "Apex crashed" message without additional information regarding the module.
		return;
	}

	// NOTE: original implementation strips the extension as well, but we keep
	// this in as its useful for when additional modules are loaded that aren't
	// part of the OS or game
	const char* const szCrashedModuleName = strrchr(szCrashedModuleFullName, '\\') + 1;

	m_Buffer.AppendFormat("\t%-15s: %p\n", szCrashedModuleName, pModuleBase);
	m_nCrashMsgFlags = 1; // Display the "Apex crashed in <module>" message.

	// Only set it once to the crashing module,
	// empty strings get treated as "unknown
	// DLL or EXE" in the crashmsg executable.
	if (!m_CrashingModule.Length())
	{
		m_CrashingModule.Append(szCrashedModuleName);
	}
}

//-----------------------------------------------------------------------------
// Purpose: formats the exception code
//-----------------------------------------------------------------------------
void CCrashHandler::FormatExceptionCode()
{
	const DWORD nExceptionCode = m_pExceptionPointers->ExceptionRecord->ExceptionCode;

	if (nExceptionCode > EXCEPTION_IN_PAGE_ERROR)
	{
		m_Buffer.AppendFormat(ExceptionToString(), nExceptionCode);
	}
	else if (nExceptionCode >= EXCEPTION_ACCESS_VIOLATION)
	{
		const CHAR* pszException = "EXCEPTION_IN_PAGE_ERROR";

		if (nExceptionCode == EXCEPTION_ACCESS_VIOLATION)
		{
			pszException = "EXCEPTION_ACCESS_VIOLATION";
		}

		const ULONG_PTR uExceptionInfo0 = m_pExceptionPointers->ExceptionRecord->ExceptionInformation[0];
		const ULONG_PTR uExceptionInfo1 = m_pExceptionPointers->ExceptionRecord->ExceptionInformation[1];

		if (uExceptionInfo0)
		{
			if (uExceptionInfo0 == 1)
			{
				m_Buffer.AppendFormat("\t%s(write): %p\n", pszException, uExceptionInfo1);
			}
			else if (uExceptionInfo0 == 8)
			{
				m_Buffer.AppendFormat("\t%s(execute): %p\n", pszException, uExceptionInfo1);
			}
			else
			{
				m_Buffer.AppendFormat("\t%s(unknown): %p\n", pszException, uExceptionInfo1);
			}
		}
		else
		{
			m_Buffer.AppendFormat("\t%s(read): %p\n", pszException, uExceptionInfo1);
		}

		if (uExceptionInfo0 != 8)
		{
			if (IsPageAccessible())
			{
				FormatExceptionAddress();
			}
		}
	}
	else
	{
		m_Buffer.AppendFormat(ExceptionToString(), nExceptionCode);
	}
}

//-----------------------------------------------------------------------------
// Purpose: formats the arithmetic logic register and its content
// Input  : *pszRegister - 
//			nContent - 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatALU(const char* const pszRegister, const DWORD64 nContent)
{
	if (nContent >= 1000000)
	{
		if (nContent > UINT_MAX)
		{
			// Print the full 64bits of the register.
			m_Buffer.AppendFormat("\t%s = 0x%016llX\n", pszRegister, nContent);
		}
		else
		{
			m_Buffer.AppendFormat("\t%s = 0x%08X\n", pszRegister, nContent);
		}
	}
	else if (nContent >= 10)
	{
		// Print as decimal with a hexadecimal comment.
		m_Buffer.AppendFormat("\t%s = %-6i // 0x%08X\n", pszRegister, nContent, nContent);
	}
	else
	{
		// Print as decimal only.
		m_Buffer.AppendFormat("\t%s = %-10i\n", pszRegister, nContent);
	}
}

//-----------------------------------------------------------------------------
// Purpose: formats the floating point register and its content
// Input  : *pszRegister - 
//			*pxContent - 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatFPU(const char* const pszRegister, const M128A* const pxContent)
{
	const DWORD nVec[4] =
	{
		static_cast<DWORD>(pxContent->Low & UINT_MAX),
		static_cast<DWORD>(pxContent->Low >> 32),
		static_cast<DWORD>(pxContent->High & UINT_MAX),
		static_cast<DWORD>(pxContent->High >> 32),
	};

	m_Buffer.AppendFormat("\t%s = [ [%.8g, %.8g, %.8g, %.8g]", pszRegister,
		*reinterpret_cast<const FLOAT*>(&nVec[0]),
		*reinterpret_cast<const FLOAT*>(&nVec[1]),
		*reinterpret_cast<const FLOAT*>(&nVec[2]),
		*reinterpret_cast<const FLOAT*>(&nVec[3]));

	const DWORD nHighest = *MaxElement(std::begin(nVec), std::end(nVec));

	if (nHighest >= 1000000)
	{
		m_Buffer.AppendFormat(", [0x%08X, 0x%08X, 0x%08X, 0x%08X] ]\n",
			nVec[0], nVec[1], nVec[2], nVec[3]);
	}
	else
	{
		m_Buffer.AppendFormat(", [%i, %i, %i, %i] ]\n",
			static_cast<LONG>(nVec[0]), static_cast<LONG>(nVec[1]),
			static_cast<LONG>(nVec[2]), static_cast<LONG>(nVec[3]));
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the current exception code as string
// Output : exception code, "UNKNOWN_EXCEPTION" if exception code doesn't exist in this context
//-----------------------------------------------------------------------------
const char* CCrashHandler::ExceptionToString(const DWORD nExceptionCode) const
{
	switch (nExceptionCode)
	{
	case EXCEPTION_BREAKPOINT:               { return "\tEXCEPTION_BREAKPOINT"               ": %08X\n"; };
	case EXCEPTION_SINGLE_STEP:              { return "\tEXCEPTION_SINGLE_STEP"              ": %08X\n"; };
	case EXCEPTION_ACCESS_VIOLATION:         { return "\tEXCEPTION_ACCESS_VIOLATION"         ": %08X\n"; };
	case EXCEPTION_IN_PAGE_ERROR:            { return "\tEXCEPTION_IN_PAGE_ERROR"            ": %08X\n"; };
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    { return "\tEXCEPTION_ARRAY_BOUNDS_EXCEEDED"    ": %08X\n"; };
	case EXCEPTION_ILLEGAL_INSTRUCTION:      { return "\tEXCEPTION_ILLEGAL_INSTRUCTION"      ": %08X\n"; };
	case EXCEPTION_INVALID_DISPOSITION:      { return "\tEXCEPTION_INVALID_DISPOSITION"      ": %08X\n"; };
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: { return "\tEXCEPTION_NONCONTINUABLE_EXCEPTION" ": %08X\n"; };
	case EXCEPTION_PRIV_INSTRUCTION:         { return "\tEXCEPTION_PRIV_INSTRUCTION"         ": %08X\n"; };
	case EXCEPTION_STACK_OVERFLOW:           { return "\tEXCEPTION_STACK_OVERFLOW"           ": %08X\n"; };
	case EXCEPTION_DATATYPE_MISALIGNMENT:    { return "\tEXCEPTION_DATATYPE_MISALIGNMENT"    ": %08X\n"; };
	case EXCEPTION_FLT_DENORMAL_OPERAND:     { return "\tEXCEPTION_FLT_DENORMAL_OPERAND"     ": %08X\n"; };
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:       { return "\tEXCEPTION_FLT_DIVIDE_BY_ZERO"       ": %08X\n"; };
	case EXCEPTION_FLT_INEXACT_RESULT:       { return "\tEXCEPTION_FLT_INEXACT_RESULT"       ": %08X\n"; };
	case EXCEPTION_FLT_INVALID_OPERATION:    { return "\tEXCEPTION_FLT_INVALID_OPERATION"    ": %08X\n"; };
	case EXCEPTION_FLT_OVERFLOW:             { return "\tEXCEPTION_FLT_OVERFLOW"             ": %08X\n"; };
	case EXCEPTION_FLT_STACK_CHECK:          { return "\tEXCEPTION_FLT_STACK_CHECK"          ": %08X\n"; };
	case EXCEPTION_FLT_UNDERFLOW:            { return "\tEXCEPTION_FLT_UNDERFLOW"            ": %08X\n"; };
	case EXCEPTION_INT_DIVIDE_BY_ZERO:       { return "\tEXCEPTION_INT_DIVIDE_BY_ZERO"       ": %08X\n"; };
	case EXCEPTION_INT_OVERFLOW:             { return "\tEXCEPTION_INT_OVERFLOW"             ": %08X\n"; };
	default:                                 { return "\tUNKNOWN_EXCEPTION"                  ": %08X\n"; };
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the current exception code as string
//-----------------------------------------------------------------------------
const char* CCrashHandler::ExceptionToString() const
{
	return ExceptionToString(m_pExceptionPointers->ExceptionRecord->ExceptionCode);
}

//-----------------------------------------------------------------------------
// Purpose: tests if memory page is accessible
// Output : true if accessible, false otherwise
//-----------------------------------------------------------------------------
bool CCrashHandler::IsPageAccessible() const
{
	const PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;
	MEMORY_BASIC_INFORMATION mbi = { 0 };

	const SIZE_T t = VirtualQuery((LPCVOID)pContextRecord->Rsp, &mbi, sizeof(LPCVOID));
	if (t < sizeof(mbi) || (mbi.Protect & PAGE_NOACCESS) || !((mbi.Protect & PAGE_NOACCESS) | PAGE_READWRITE))
	{
		return false;
	}
	else
	{
		return !(mbi.State & MEM_COMMIT);
	}
}

//-----------------------------------------------------------------------------
// Purpose: captures the callstack
//-----------------------------------------------------------------------------
void CCrashHandler::CaptureCallStack()
{
	m_nCapturedFrames = RtlCaptureStackBackTrace(2, NUM_FRAMES_TO_CAPTURE, m_ppStackTrace, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: writes the stack trace and minidump to the disk
//-----------------------------------------------------------------------------
void CCrashHandler::WriteFile()
{
	CFmtStrQuietTruncationN<256> outFile;

	outFile.Format("%s/%s.txt", g_LogSessionDirectory.c_str(), "apex_crash");
	HANDLE hTxtFile = CreateFile(outFile.String(), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hTxtFile != INVALID_HANDLE_VALUE)
	{
		::WriteFile(hTxtFile, m_Buffer.String(), (DWORD)m_Buffer.Length(), NULL, NULL);
		CloseHandle(hTxtFile);
	}

	outFile.Format("%s/%s.dmp", g_LogSessionDirectory.c_str(), "minidump");
	HANDLE hDmpFile = CreateFile(outFile.String(), GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);

	if (hDmpFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION dumpExceptionInfo;
		dumpExceptionInfo.ThreadId = GetCurrentThreadId();
		dumpExceptionInfo.ExceptionPointers = m_pExceptionPointers;
		dumpExceptionInfo.ClientPointers = false;

		MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hDmpFile, MiniDumpNormal,
			&dumpExceptionInfo, NULL, NULL);

		CloseHandle(hDmpFile);
	}
}

//-----------------------------------------------------------------------------
// Purpose: creates the crashmsg process displaying the error to the user
// the process has to be separate as the current process is getting killed
//-----------------------------------------------------------------------------
void CCrashHandler::CreateMessageProcess() const
{
	CFmtStrQuietTruncationN<256> messageCmdLine;

	const PEXCEPTION_RECORD pExceptionRecord = m_pExceptionPointers->ExceptionRecord;
	const PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;

	if (pExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
		pExceptionRecord->ExceptionInformation[0] == 8 &&
		pExceptionRecord->ExceptionInformation[1] != pContextRecord->Rip)
	{
		messageCmdLine.Append(CRASHMESSAGE_MSG_EXECUTABLE" overclock");
	}
	else
	{
		messageCmdLine.Format(CRASHMESSAGE_MSG_EXECUTABLE" crash %hhu \"%s\"",
			m_nCrashMsgFlags, m_CrashingModule.String());
	}

	STARTUPINFOA startupInfo = { 0 };
	PROCESS_INFORMATION processInfo;

	startupInfo.cb = sizeof(STARTUPINFOA);

	if (CreateProcessA(NULL, (LPSTR)messageCmdLine.String(),
		NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startupInfo, &processInfo))
	{
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
}

//-----------------------------------------------------------------------------
// Purpose: calls the crash callback
//-----------------------------------------------------------------------------
void CCrashHandler::CrashCallback() const
{
	if (m_pCrashCallback)
	{
		m_pCrashCallback(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
long __stdcall BottomLevelExceptionFilter(EXCEPTION_POINTERS* const pExceptionInfo)
{
	g_CrashHandler.Start();

	// If the exception couldn't be handled, terminate the process
	if (g_CrashHandler.GetExit())
	{
		ExitProcess(EXIT_FAILURE);
	}

	g_CrashHandler.Reset();
	g_CrashHandler.SetExceptionPointers(pExceptionInfo);

	// Let the higher level exception handlers deal with this particular
	// exception.
	if (g_CrashHandler.ExceptionToString() == g_CrashHandler.ExceptionToString(0xFFFFFFFF))
	{
		g_CrashHandler.End();
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// Don't run when a debugger is present.
	if (IsDebuggerPresent())
	{
		g_CrashHandler.End();
		return EXCEPTION_CONTINUE_SEARCH;
	}

	g_CrashHandler.SetExit(true);

	g_CrashHandler.CaptureCallStack();

	g_CrashHandler.FormatCrash();
	g_CrashHandler.FormatCallstack();
	g_CrashHandler.FormatRegisters();
	g_CrashHandler.FormatModules();
	g_CrashHandler.FormatSystemInfo();
	g_CrashHandler.FormatBuildInfo();

	g_CrashHandler.WriteFile();

	// Run the crash callback
	g_CrashHandler.CrashCallback();

	// End it here, the next recursive call terminates the process.
	g_CrashHandler.End();

	return EXCEPTION_EXECUTE_HANDLER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::Init()
{
	InitializeSRWLock(&m_Lock);
	m_hExceptionHandler = AddVectoredExceptionHandler(TRUE, BottomLevelExceptionFilter);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::Shutdown()
{
	if (m_hExceptionHandler)
	{
		RemoveVectoredExceptionHandler(m_hExceptionHandler);
		m_hExceptionHandler = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::Reset()
{
	m_Buffer.Clear();
	m_CrashingModule.Clear();
	m_nCrashMsgFlags = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCrashHandler::CCrashHandler()
	: m_ppStackTrace()
	, m_nCapturedFrames(0)
	, m_ppModuleHandles()
	, m_pCrashCallback(nullptr)
	, m_hExceptionHandler(nullptr)
	, m_pExceptionPointers(nullptr)
	, m_nCrashMsgFlags(0)
	, m_bExit(false)
	, m_bMessageCreated(false)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCrashHandler::~CCrashHandler()
{
	Shutdown();
}

CCrashHandler g_CrashHandler;
