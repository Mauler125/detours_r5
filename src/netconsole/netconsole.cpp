//=====================================================================================//
// 
// Purpose: Lightweight netconsole client.
// 
//=====================================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "core/logger.h"
#include "tier0/cpu.h"
#include "tier0/utility.h"
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "windows/console.h"
#include "protoc/netcon.pb.h"
#include "engine/net.h"
#include "engine/shared/shared_rcon.h"
#include "netconsole/netconsole.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetCon::CNetCon(void)
	: m_bInitialized(false)
	, m_bQuitting(false)
	, m_bPromptConnect(true)
	, m_bEncryptFrames(false)
	, m_flTickInterval(0.05f)
{
	// Empty character set used for ip addresses if we still need to initiate a
	// connection, as we don't want to break on ':' characters found in an IPv6
	// address.
	CharacterSetBuild(&m_CharacterSet, "");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetCon::~CNetCon(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: WSA and NETCON systems init
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Init(const bool bAnsiColor, const char* pAdr, const char* pKey)
{
	AUTO_LOCK(m_Mutex);

	g_CoreMsgVCallback = &EngineLoggerSink;
	TermSetup(bAnsiColor);

	Msg(eDLL_T::NONE, "R5 TCP net console [Version %s]\n", NETCON_VERSION);

	WSAData wsaData;
	const int nError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nError != 0)
	{
		Error(eDLL_T::CLIENT, NO_ERROR, "%s - Failed to start Winsock: (%s)\n", __FUNCTION__, NET_ErrorString(WSAGetLastError()));
		return false;
	}

	// Install the encryption key and enable encryption if this has been passed
	// in by the user.
	if (pKey)
	{
		SetKey(pKey, true);
		m_bEncryptFrames = true;
	}

	// Try and connect to the giver remote address if this has been passed in
	// by the user. If we fail, return out as this allows the user to quickly
	// try again.
	if (pAdr)
	{
		if (!Connect(pAdr))
		{
			return false;
		}
	}

	m_bInitialized = true;

	static std::thread frame([this]()
		{
			while (RunFrame())
			{}
		});
	frame.detach();


	static std::thread input([this]()
		{
			while (!NetConsole()->GetQuitting())
			{
				std::string lineInput;

				if (std::getline(std::cin, lineInput))
				{
					NetConsole()->RunInput(lineInput);
				}
			}
		});
	input.detach();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: WSA and NETCON systems shutdown
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Shutdown(void)
{
	if (!m_bInitialized)
	{
		// Called twice!
		Assert(0);
		return false;
	}

	m_bInitialized = false;

	AUTO_LOCK(m_Mutex);
	bool bResult = false;

	GetSocketCreator()->CloseAllAcceptedSockets();

	const int nError = ::WSACleanup();
	if (nError == 0)
	{
		bResult = true;
	}
	else // WSACleanup() failed.
	{
		Error(eDLL_T::CLIENT, NO_ERROR, "%s - Failed to stop Winsock: (%s)\n",
			__FUNCTION__, NET_ErrorString(WSAGetLastError()));
	}

	SpdLog_Shutdown();
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: handle close events outside application routines
//-----------------------------------------------------------------------------
BOOL WINAPI CNetCon::CloseHandler(DWORD eventCode)
{
	switch (eventCode)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:

		NetConsole()->SetQuitting(true);

		// Give it time to shutdown properly, value is set to the max possible
		// of SPI_GETWAITTOKILLSERVICETIMEOUT, which is 20000ms by default.
		Sleep(20000);
		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Purpose: terminal setup
//-----------------------------------------------------------------------------
void CNetCon::TermSetup(const bool bAnsiColor)
{
	SpdLog_Init(bAnsiColor);
	Console_Init(bAnsiColor);

	// Handle ctrl+x or X close events, give the application time to shutdown
	// properly and flush all logging buffers.
	SetConsoleCtrlHandler(CloseHandler, true);

	// Write console output to a file, this includes everything from local logs
	// to messages over the wire. Note that logs emitted locally are prefixed
	// with timestamps in () while messages over the wire are in [].
	SpdLog_InstallSupplementalLogger("supplemental_logger_mt", "netconsole.log");
}

//-----------------------------------------------------------------------------
// Purpose: tries to set the passed in netkey, falls back to default on failure
//-----------------------------------------------------------------------------
void CNetCon::TrySetKey(const char* const pKey)
{
	if (!*pKey)
	{
		Warning(eDLL_T::CLIENT, "No key provided; using default %s'%s%s%s'\n",
			g_svReset.c_str(), g_svGreyB.c_str(), DEFAULT_NET_ENCRYPTION_KEY, g_svReset.c_str());

		SetKey(DEFAULT_NET_ENCRYPTION_KEY, true);
	}
	else
	{
		SetKey(pKey, true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets input IP and port for initialization
//-----------------------------------------------------------------------------
void CNetCon::RunInput(const string& lineInput)
{
	if (lineInput.empty())
	{
		// Empty string given, don't process it.
		return;
	}

	if (lineInput.compare("nquit") == 0)
	{
		SetQuitting(true);
		return;
	}

	AUTO_LOCK(m_Mutex);

	if (IsConnected())
	{
		CCommand cmd;
		cmd.Tokenize(lineInput.c_str());

		if (V_strcmp(cmd.Arg(0), "disconnect") == 0)
		{
			Disconnect("user closed connection");
			return;
		}

		vector<char> vecMsg;

		const SocketHandle_t hSocket = GetSocket();
		bool bSend = false;

		if (cmd.ArgC() > 1)
		{
			if (V_strcmp(cmd.Arg(0), "PASS") == 0) // Auth with RCON server.
			{
				bSend = Serialize(vecMsg, cmd.Arg(1), "",
					netcon::request_e::SERVERDATA_REQUEST_AUTH);
			}
			else // Execute command query.
			{
				bSend = Serialize(vecMsg, cmd.Arg(0), cmd.GetCommandString(),
					netcon::request_e::SERVERDATA_REQUEST_EXECCOMMAND);
			}
		}
		else // Single arg command query.
		{
			bSend = Serialize(vecMsg, lineInput.c_str(), "", netcon::request_e::SERVERDATA_REQUEST_EXECCOMMAND);
		}

		if (bSend) // Only send if serialization process was successful.
		{
			if (!Send(hSocket, vecMsg.data(), int(vecMsg.size())))
			{
				Error(eDLL_T::CLIENT, NO_ERROR, "Failed to send RCON message: (%s)\n", "SOCKET_ERROR");
			}
		}
	}
	else // Setup connection from input.
	{
		CCommand cmd;
		cmd.Tokenize(lineInput.c_str(), cmd_source_t::kCommandSrcCode, &m_CharacterSet);

		if (cmd.ArgC() > 1)
		{
			const char* inAdr = cmd.Arg(0);
			const char* inKey = cmd.Arg(1);

			if (!*inAdr)
			{
				Warning(eDLL_T::CLIENT, "No address provided\n");
				SetPrompting(true);
				return;
			}

			TrySetKey(inKey);
			m_bEncryptFrames = true;

			if (!Connect(inAdr))
			{
				SetPrompting(true);
				return;
			}
		}
		else // No encryption
		{
			const char* inAdr = cmd.GetCommandString();
			m_bEncryptFrames = false;

			if (!Connect(inAdr))
			{
				SetPrompting(true);
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: client's main processing loop
//-----------------------------------------------------------------------------
bool CNetCon::RunFrame(void)
{
	if (IsInitialized())
	{
		AUTO_LOCK(m_Mutex);

		if (IsConnected())
		{
			CConnectedNetConsoleData& pData = GetSocketCreator()->GetAcceptedSocketData(0);
			Recv(pData);
		}
		else if (GetPrompting())
		{
			Msg(eDLL_T::NONE, "Enter [<address>]:<port> and <key>: ");
			SetPrompting(false);
		}
	}

	std::this_thread::sleep_for(IntervalToDuration(m_flTickInterval));
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: checks if application should be terminated
// Output : true for termination, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::GetQuitting(void) const
{
	return m_bQuitting;
}

//-----------------------------------------------------------------------------
// Purpose: set whether we should quit
// Input  : bQuit
//-----------------------------------------------------------------------------
void CNetCon::SetQuitting(const bool bQuit)
{
	m_bQuitting = bQuit;
}

//-----------------------------------------------------------------------------
// Purpose: checks if we should prompt the connect message
// Output : true for prompting, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::GetPrompting(void) const
{
	return m_bPromptConnect;
}

//-----------------------------------------------------------------------------
// Purpose: set whether we should prompt the connect message
// Input  : bPrompt
//-----------------------------------------------------------------------------
void CNetCon::SetPrompting(const bool bPrompt)
{
	m_bPromptConnect = bPrompt;
}

//-----------------------------------------------------------------------------
// Purpose: connect to remote
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Connect(const char* pHostName, const int nPort)
{
	Assert(nPort == SOCKET_ERROR, "Port should be part of the address on the CNetCon implementation!");
	NOTE_UNUSED(nPort);

	if (m_bEncryptFrames)
	{
		Msg(eDLL_T::CLIENT, "Attempting connection to '%s' with key %s'%s%s%s'\n",
			pHostName, g_svReset.c_str(), g_svGreyB.c_str(), GetKey(), g_svReset.c_str());
	}
	else
	{
		Msg(eDLL_T::CLIENT, "Attempting connection to '%s'\n", pHostName);
	}

	return NetconClient_Connect(this, pHostName, SOCKET_ERROR);
}

//-----------------------------------------------------------------------------
// Purpose: disconnect from current session
// Input  : *szReason - 
//-----------------------------------------------------------------------------
void CNetCon::Disconnect(const char* szReason)
{
	if (IsConnected())
	{
		if (!szReason)
		{
			szReason = "unknown reason";
		}

		Msg(eDLL_T::CLIENT, "Disconnect: (%s)\n", szReason);
		GetSocketCreator()->CloseAcceptedSocket(0);
	}

	SetPrompting(true);
}

//-----------------------------------------------------------------------------
// Purpose: processes received message
// Input  : *pMsgBuf - 
//			nMsgLen - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::ProcessMessage(const char* pMsgBuf, const int nMsgLen)
{
	netcon::response response;

	if (!NetconShared_UnpackEnvelope(this, pMsgBuf, nMsgLen, &response, true))
	{
		Disconnect("received invalid message");
		return false;
	}

	switch (response.responsetype())
	{
	case netcon::response_e::SERVERDATA_RESPONSE_AUTH:
	{
		if (!response.responseval().empty())
		{
			const long i = strtol(response.responseval().c_str(), NULL, NULL);
			if (!i) // Means we are marked 'input only' on the rcon server.
			{
				vector<char> vecMsg;
				bool ret = Serialize(vecMsg, "", "1", netcon::request_e::SERVERDATA_REQUEST_SEND_CONSOLE_LOG);

				if (ret && !Send(GetSocket(), vecMsg.data(), int(vecMsg.size())))
				{
					Error(eDLL_T::CLIENT, NO_ERROR, "Failed to send RCON message: (%s)\n", "SOCKET_ERROR");
				}
			}
		}

		Msg(eDLL_T::NETCON, "%s", response.responsemsg().c_str());
		break;
	}
	case netcon::response_e::SERVERDATA_RESPONSE_CONSOLE_LOG:
	{
		NetMsg(static_cast<LogType_t>(response.messagetype()),
			static_cast<eDLL_T>(response.messageid()),
			response.responseval().c_str(), "%s", response.responsemsg().c_str());
		break;
	}
	default:
	{
		break;
	}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: serializes message to vector
// Input  : &vecBuf - 
//			*szReqBuf - 
//			*svReqVal - 
//			requestType - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CNetCon::Serialize(vector<char>& vecBuf, const char* szReqBuf,
	const char* szReqVal, const netcon::request_e requestType) const
{
	return NetconClient_Serialize(this, vecBuf, szReqBuf, szReqVal, requestType, m_bEncryptFrames, true);
}

//-----------------------------------------------------------------------------
// Purpose: retrieves the remote socket
// Output : SOCKET_ERROR (-1) on failure
//-----------------------------------------------------------------------------
SocketHandle_t CNetCon::GetSocket(void)
{
	return NetconShared_GetSocketHandle(this, 0);
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the rcon client is initialized
//-----------------------------------------------------------------------------
bool CNetCon::IsInitialized(void) const
{
	return m_bInitialized;
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the rcon client is connected
//-----------------------------------------------------------------------------
bool CNetCon::IsConnected(void)
{
	return (GetSocket() != SOCKET_ERROR);
}

//-----------------------------------------------------------------------------
// Purpose: entrypoint
// Input  : argc - 
//			*argv - 
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	CheckSystemCPUForSSE2();

	bool bEnableColor = false;

	for (int i = 0; i < argc; i++)
	{
		if (V_strcmp(argv[i], "-ansicolor") == NULL)
		{
			bEnableColor = true;
			break;
		}
	}

	// The address and key from command line if passed in.
	const char* pAdr = nullptr;
	const char* pKey = nullptr;

	if (argc >= 2)
	{
		pAdr = argv[1];
	}

	if (argc >= 3)
	{
		pKey = argv[2];
	}

	if (!NetConsole()->Init(bEnableColor, pAdr, pKey))
	{
		return EXIT_FAILURE;
	}

	while (!NetConsole()->GetQuitting())
	{
		// Run with a reasonable tick rate so we don't eat all the CPU.
		std::this_thread::sleep_for(
			IntervalToDuration(NetConsole()->GetTickInterval()));
	}

	if (!NetConsole()->Shutdown())
	{
		return EXIT_FAILURE;
	}

	return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// singleton
//-----------------------------------------------------------------------------
CNetCon g_NetCon;
inline CNetCon* NetConsole()
{
	return &g_NetCon;
}