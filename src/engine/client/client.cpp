//===============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//
//===============================================================================//
// client.cpp: implementation of the CClient class.
//
///////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "mathlib/bitvec.h"
#include "tier1/cvar.h"
#include "tier1/strtools.h"
#include "engine/server/server.h"
#include "engine/client/client.h"
#ifndef CLIENT_DLL
#include "networksystem/hostmanager.h"
#include "jwt/include/decode.h"
#include "mbedtls/include/mbedtls/sha256.h"
#endif
#include "game/server/gameinterface.h"

// Absolute max string cmd length, any character past this will be NULLED.
#define STRINGCMD_MAX_LEN 512

//---------------------------------------------------------------------------------
// Purpose: throw away any residual garbage in the channel
//---------------------------------------------------------------------------------
void CClient::Clear(void)
{
#ifndef CLIENT_DLL
	GetClientExtended()->Reset(); // Reset extended data.
#endif // !CLIENT_DLL
	CClient__Clear(this);
}

//---------------------------------------------------------------------------------
// Purpose: throw away any residual garbage in the channel
// Input  : *pClient - 
//---------------------------------------------------------------------------------
void CClient::VClear(CClient* pClient)
{
	pClient->Clear();
}

#ifndef CLIENT_DLL
//---------------------------------------------------------------------------------
// Purpose: gets the extended client data
// Output  : CClientExtended* - 
//---------------------------------------------------------------------------------
CClientExtended* CClient::GetClientExtended(void) const 
{
	return m_pServer->GetClientExtended(m_nUserID);
}
#endif // !CLIENT_DLL


static const char JWT_PUBLIC_KEY[] = 
"-----BEGIN PUBLIC KEY-----\n"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2/335exIZ6LE8pYi6e50\n"
"7tH19tXaeeEJVF5XXpTCXpndXIIWVimvg6xQ381eajySDw93wvG1DzW3U/6LHzyt\n"
"Q++N8w7N+FwnXyoDUD5Y8hheTZv6jjLoYT8ZtsMl20k9UosrbFBTMUhgmIT2dVth\n"
"LH+rT9ohpUNwQXHJvTOs9eY74GyfFw93+32LANBPZ8b+S8S3oZnKFVeCxRkYKsV0\n"
"b34POHVBbXNw6Kt163gR5zaiCfJJtRto9AA7MV2t9pfy8CChs3uJ+Xn7QVHD5cqt\n"
"Msg9MBac2Pvs2j+8wJ/igAVL5L81z3FXVt04id59TfPMUbYhRfY8pk7FB0MCigOH\n"
"dwIDAQAB\n"
"-----END PUBLIC KEY-----\n";

static ConVar sv_onlineAuthEnable("sv_onlineAuthEnable", "1", FCVAR_RELEASE, "Enables the server-side online authentication system");

static ConVar sv_onlineAuthValidateExpiry("sv_onlineAuthValidateExpiry", "1", FCVAR_RELEASE, "Validate the online authentication token 'expiry' claim");
static ConVar sv_onlineAuthValidateIssuedAt("sv_onlineAuthValidateIssuedAt", "1", FCVAR_RELEASE, "Validate the online authentication token 'issued at' claim");

static ConVar sv_onlineAuthExpiryTolerance("sv_onlineAuthExpiryTolerance", "1", FCVAR_DEVELOPMENTONLY, "The online authentication token 'expiry' claim tolerance in seconds", true, 0.f, true, float(UINT8_MAX), "Must range between [0,255]");
static ConVar sv_onlineAuthIssuedAtTolerance("sv_onlineAuthIssuedAtTolerance", "30", FCVAR_DEVELOPMENTONLY, "The online authentication token 'issued at' claim tolerance in seconds", true, 0.f, true, float(UINT8_MAX), "Must range between [0,255]");

static ConVar sv_quota_stringCmdsPerSecond("sv_quota_stringCmdsPerSecond", "32", FCVAR_RELEASE, "How many string commands per second clients are allowed to submit, 0 to disallow all string commands", true, 0.f, false, 0.f);

//---------------------------------------------------------------------------------
// Purpose: check whether this client is authorized to join this server
// Input  : *playerName  - 
//			*reasonBuf   - 
//			reasonBufLen - 
// Output : true if authorized, false otherwise
//---------------------------------------------------------------------------------
bool CClient::Authenticate(const char* const playerName, char* const reasonBuf, const size_t reasonBufLen)
{
#ifndef CLIENT_DLL
	// don't bother checking origin auth on bots or local clients
	if (IsFakeClient() || GetNetChan()->GetRemoteAddress().IsLoopback())
		return true;

	l8w8jwt_claim* claims = nullptr;
	size_t numClaims = 0;

	// formats the error reason, and frees the claims and returns
#define ERROR_AND_RETURN(fmt, ...) \
		do {\
			V_snprintf(reasonBuf, reasonBufLen, fmt, ##__VA_ARGS__); \
			if (claims) {\
				l8w8jwt_free_claims(claims, numClaims); \
			}\
			return false; \
		} while(0)\

	KeyValues* const cl_onlineAuthTokenKv = this->m_ConVars->FindKey("cl_onlineAuthToken");
	KeyValues* const cl_onlineAuthTokenSignature1Kv = this->m_ConVars->FindKey("cl_onlineAuthTokenSignature1");
	KeyValues* const cl_onlineAuthTokenSignature2Kv = this->m_ConVars->FindKey("cl_onlineAuthTokenSignature2");

	if (!cl_onlineAuthTokenKv || !cl_onlineAuthTokenSignature1Kv)
		ERROR_AND_RETURN("Missing token");

	const char* const onlineAuthToken = cl_onlineAuthTokenKv->GetString();
	const char* const onlineAuthTokenSignature1 = cl_onlineAuthTokenSignature1Kv->GetString();
	const char* const onlineAuthTokenSignature2 = cl_onlineAuthTokenSignature2Kv->GetString();

	char fullToken[1024]; // enough buffer for 3x255, which is cvar count * userinfo str limit.
	const int tokenLen = snprintf(fullToken, sizeof(fullToken), "%s.%s%s", 
		onlineAuthToken, onlineAuthTokenSignature1, onlineAuthTokenSignature2);

	if (tokenLen < 0)
		ERROR_AND_RETURN("Token stitching failed");

	struct l8w8jwt_decoding_params params;
	l8w8jwt_decoding_params_init(&params);

	params.alg = L8W8JWT_ALG_RS256;

	params.jwt = (char*)fullToken;
	params.jwt_length = tokenLen;

	params.verification_key = (unsigned char*)JWT_PUBLIC_KEY;
	params.verification_key_length = sizeof(JWT_PUBLIC_KEY);

	params.validate_exp = sv_onlineAuthValidateExpiry.GetBool();
	params.exp_tolerance_seconds = (uint8_t)sv_onlineAuthExpiryTolerance.GetInt();

	params.validate_iat = sv_onlineAuthValidateIssuedAt.GetBool();
	params.iat_tolerance_seconds = (uint8_t)sv_onlineAuthIssuedAtTolerance.GetInt();

	enum l8w8jwt_validation_result validation_result;
	const int r = l8w8jwt_decode(&params, &validation_result, &claims, &numClaims);

	if (r != L8W8JWT_SUCCESS)
		ERROR_AND_RETURN("Code %i", r);

	if (validation_result != L8W8JWT_VALID)
	{
		char reasonBuffer[64];
		l8w8jwt_get_validation_result_desc(validation_result, reasonBuffer, sizeof(reasonBuffer));

		ERROR_AND_RETURN("%s", reasonBuffer);
	}

	bool foundSessionId = false;
	for (size_t i = 0; i < numClaims; ++i)
	{
		// session id
		if (!strcmp(claims[i].key, "sessionId"))
		{
			const char* const sessionId = claims[i].value;

			char newId[256];
			const int idLen = snprintf(newId, sizeof(newId), "%llu-%s-%s",
				(NucleusID_t)this->m_DataBlock.userData,
				playerName,
				g_ServerHostManager.GetHostIP().c_str());

			if (idLen < 0)
				ERROR_AND_RETURN("Session ID stitching failed");

			uint8_t sessionHash[32]; // hash decoded from JWT token
			V_hextobinary(sessionId, strlen(sessionId), sessionHash, sizeof(sessionHash));

			uint8_t oobHash[32]; // hash of data collected from out of band packet
			const int shRet = mbedtls_sha256((const uint8_t*)newId, idLen, oobHash, NULL);

			if (shRet != NULL)
				ERROR_AND_RETURN("Session ID hashing failed");

			if (memcmp(oobHash, sessionHash, sizeof(sessionHash)) != 0)
				ERROR_AND_RETURN("Token is not authorized for the connecting client");

			foundSessionId = true;
		}
	}

	if (!foundSessionId)
		ERROR_AND_RETURN("No session ID");

	l8w8jwt_free_claims(claims, numClaims);

#undef ERROR_AND_RETURN
#endif // !CLIENT_DLL

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: connect new client
// Input  : *szName - 
//			*pNetChan - 
//			bFakePlayer - 
//			*conVars - 
//			*szMessage -
//			nMessageSize - 
// Output : true if connection was successful, false otherwise
//---------------------------------------------------------------------------------
bool CClient::Connect(const char* szName, CNetChan* pNetChan, bool bFakePlayer,
	CUtlVector<NET_SetConVar::cvar_t>* conVars, char* szMessage, int nMessageSize)
{
#ifndef CLIENT_DLL
	GetClientExtended()->Reset(); // Reset extended data.
#endif

	if (!CClient__Connect(this, szName, pNetChan, bFakePlayer, conVars, szMessage, nMessageSize))
		return false;

#ifndef CLIENT_DLL

#define REJECT_CONNECTION(fmt, ...) V_snprintf(szMessage, nMessageSize, fmt, ##__VA_ARGS__);

	if (sv_onlineAuthEnable.GetBool())
	{
		char authFailReason[512];
		if (!Authenticate(szName, authFailReason, sizeof(authFailReason)))
		{
			REJECT_CONNECTION("Failed to verify authentication token [%s]", authFailReason);

			const bool bEnableLogging = sv_showconnecting.GetBool();
			if (bEnableLogging)
			{
				const char* const netAdr = pNetChan ? pNetChan->GetAddress() : "<unknown>";

				Warning(eDLL_T::SERVER, "Client '%s' ('%llu') failed online authentication! [%s]\n",
					netAdr, (NucleusID_t)m_DataBlock.userData, authFailReason);
			}

			return false;
		}
	}

#undef REJECT_CONNECTION
#endif // !CLIENT_DLL

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: connect new client
// Input  : *pClient - 
//			*szName - 
//			*pNetChan - 
//			bFakePlayer - 
//			*a5 - 
//			*szMessage -
//			nMessageSize - 
// Output : true if connection was successful, false otherwise
//---------------------------------------------------------------------------------
bool CClient::VConnect(CClient* pClient, const char* szName, CNetChan* pNetChan, bool bFakePlayer,
	CUtlVector<NET_SetConVar::cvar_t>* conVars, char* szMessage, int nMessageSize)
{
	return pClient->Connect(szName, pNetChan, bFakePlayer, conVars, szMessage, nMessageSize);
}

//---------------------------------------------------------------------------------
// Purpose: registers net messages
// Input  : *pClient - 
//			*pChan - 
// Output : true if setup was successful, false otherwise
//---------------------------------------------------------------------------------
bool CClient::VConnectionStart(CClient* pClient, CNetChan* pChan)
{
	pClient->RegisterNetMsgs(pChan);
	return CClient__ConnectionStart(pClient, pChan);
}

//---------------------------------------------------------------------------------
// Purpose: disconnect client
// Input  : nRepLvl - 
//			*szReason - 
//			... - 
//---------------------------------------------------------------------------------
void CClient::Disconnect(const Reputation_t nRepLvl, const char* szReason, ...)
{
	if (m_nSignonState != SIGNONSTATE::SIGNONSTATE_NONE)
	{
		char szBuf[1024];
		{/////////////////////////////
			va_list vArgs;
			va_start(vArgs, szReason);

			vsnprintf(szBuf, sizeof(szBuf), szReason, vArgs);

			szBuf[sizeof(szBuf) - 1] = '\0';
			va_end(vArgs);
		}/////////////////////////////
		CClient__Disconnect(this, nRepLvl, szBuf);
	}
}

//---------------------------------------------------------------------------------
// Purpose: activate player
// Input  : *pClient - 
//---------------------------------------------------------------------------------
void CClient::VActivatePlayer(CClient* pClient)
{
	// Set the client instance to 'ready' before calling ActivatePlayer.
	pClient->SetPersistenceState(PERSISTENCE::PERSISTENCE_READY);
	CClient__ActivatePlayer(pClient);

#ifndef CLIENT_DLL
	const CNetChan* pNetChan = pClient->GetNetChan();

	if (pNetChan && sv_showconnecting.GetBool())
	{
		Msg(eDLL_T::SERVER, "Activated player #%d; channel %s(%s) ('%llu')\n",
			pClient->GetUserID(), pNetChan->GetName(), pNetChan->GetAddress(), pClient->GetNucleusID());
	}
#endif // !CLIENT_DLL
}

//---------------------------------------------------------------------------------
// Purpose: registers net messages
// Input  : *chan
//---------------------------------------------------------------------------------
void CClient::RegisterNetMsgs(CNetChan* chan)
{
}

//---------------------------------------------------------------------------------
// Purpose: send a net message with replay.
//			set 'CNetMessage::m_nGroup' to 'NoReplay' to disable replay.
// Input  : *pMsg - 
//			bLocal - 
//			bForceReliable - 
//			bVoice - 
//---------------------------------------------------------------------------------
bool CClient::SendNetMsgEx(CNetMessage* pMsg, bool bLocal, bool bForceReliable, bool bVoice)
{
	if (!CanReplayMessage(pMsg))
	{
		// Don't copy the message into the replay buffer.
		pMsg->m_nGroup = NetMessageGroup::NoReplay;
	}

	return CClient__SendNetMsgEx(this, pMsg, bLocal, bForceReliable, bVoice);
}

//---------------------------------------------------------------------------------
// Purpose: send a snapshot
// Input  : *pClient - 
//			*pFrame - 
//			nTick - 
//			nTickAck - 
//---------------------------------------------------------------------------------
void* CClient::VSendSnapshot(CClient* pClient, CClientFrame* pFrame, int nTick, int nTickAck)
{
	return CClient__SendSnapshot(pClient, pFrame, nTick, nTickAck);
}

//---------------------------------------------------------------------------------
// Purpose: internal hook to 'CClient::SendNetMsgEx'
// Input  : *pClient - 
//			*pMsg - 
//			bLocal - 
//			bForceReliable - 
//			bVoice - 
//---------------------------------------------------------------------------------
bool CClient::VSendNetMsgEx(CClient* pClient, CNetMessage* pMsg, bool bLocal, bool bForceReliable, bool bVoice)
{
	return pClient->SendNetMsgEx(pMsg, bLocal, bForceReliable, bVoice);
}

//---------------------------------------------------------------------------------
// Purpose: write data into data blocks to send to the client
// Input  : &buf
//---------------------------------------------------------------------------------
void CClient::WriteDataBlock(CClient* pClient, bf_write& buf)
{
#ifndef CLIENT_DLL
	if (net_data_block_enabled->GetBool())
	{
		buf.WriteUBitLong(net_NOP, NETMSG_TYPE_BITS);

		const int remainingBits = buf.GetNumBitsWritten() % 8;

		if (remainingBits && (8 - remainingBits) > 0)
		{
			// fill the last bits in the last byte with NOP
			buf.WriteUBitLong(net_NOP, 8 - remainingBits);
		}

		const bool isMultiplayer = gpGlobals->gameMode < GameMode_t::PVE_MODE;
		pClient->m_DataBlock.sender.WriteDataBlock(buf.GetData(), buf.GetNumBytesWritten(), isMultiplayer, buf.GetDebugName());
	}
	else
	{
		pClient->m_NetChannel->SendData(buf, true);
	}
#endif // !CLIENT_DLL
}

//---------------------------------------------------------------------------------
// Purpose: some versions of the binary have an optimization that shifts the 'this'
// pointer of the CClient structure by 8 bytes to avoid having to cache the vftable
// pointer if it never get used. Here we shift it back so it aligns again.
//---------------------------------------------------------------------------------
CClient* AdjustShiftedThisPointer(CClient* shiftedPointer)
{
	/* Original function called method "CClient::ExecuteStringCommand" with an optimization
	 * that shifted the 'this' pointer with 8 bytes.
	 * Since this has been inlined with "CClient::ProcessStringCmd" as of S2, the shifting
	 * happens directly to anything calling this function. */
	char* pShifted = reinterpret_cast<char*>(shiftedPointer) - 8;
	return reinterpret_cast<CClient*>(pShifted);
}

//---------------------------------------------------------------------------------
// Purpose: process string commands (kicking anyone attempting to DOS)
// Input  : *pClient - (ADJ)
//			*pMsg - 
// Output : false if cmd should be passed to CServerGameClients
//---------------------------------------------------------------------------------
bool CClient::VProcessStringCmd(CClient* pClient, NET_StringCmd* pMsg)
{
#ifndef CLIENT_DLL
	CClient* const pClient_Adj = AdjustShiftedThisPointer(pClient);

	// Jettison the cmd if the client isn't active.
	if (!pClient_Adj->IsActive())
		return true;

	CClientExtended* const pSlot = pClient_Adj->GetClientExtended();

	const double flStartTime = Plat_FloatTime();
	const int nCmdQuotaLimit = sv_quota_stringCmdsPerSecond.GetInt();

	if (!nCmdQuotaLimit)
		return true;

	const char* pCmd = pMsg->cmd;
	// Just skip if the cmd pointer is null, we still check if the
	// client sent too many commands and take appropriate actions.
	// The internal function discards the command if it's null.
	if (pCmd)
	{
		// There is an issue in CUtlBuffer::ParseToken() that causes it to read
		// past its buffer; mostly seems to happen on 32bit, but a carefully
		// crafted string should work on 64bit too). The fix is to just null 
		// everything past the maximum allowed length. The second 'theoretical'
		// fix would be to properly fix CUtlBuffer::ParseToken() by computing
		// the UTF8 character size each iteration and check if it still doesn't
		// exceed bounds.
		memset(&pMsg->buffer[STRINGCMD_MAX_LEN],
			'\0', sizeof(pMsg->buffer) - (STRINGCMD_MAX_LEN));

		if (!V_IsValidUTF8(pCmd))
		{
			Warning(eDLL_T::SERVER, "Removing client '%s' from slot #%i ('%llu' sent invalid string command!)\n",
				pClient_Adj->GetNetChan()->GetAddress(), pClient_Adj->GetUserID(), pClient_Adj->GetNucleusID());

			pClient_Adj->Disconnect(Reputation_t::REP_MARK_BAD, "#DISCONNECT_INVALID_STRINGCMD");
			return true;
		}
	}

	if (flStartTime - pSlot->m_flStringCommandQuotaTimeStart >= 1.0)
	{
		pSlot->m_flStringCommandQuotaTimeStart = flStartTime;
		pSlot->m_nStringCommandQuotaCount = 0;
	}
	++pSlot->m_nStringCommandQuotaCount;

	if (pSlot->m_nStringCommandQuotaCount > nCmdQuotaLimit)
	{
		Warning(eDLL_T::SERVER, "Removing client '%s' from slot #%i ('%llu' exceeded string command quota!)\n",
			pClient_Adj->GetNetChan()->GetAddress(), pClient_Adj->GetUserID(), pClient_Adj->GetNucleusID());

		pClient_Adj->Disconnect(Reputation_t::REP_MARK_BAD, "#DISCONNECT_STRINGCMD_OVERFLOW");
		return true;
	}
#endif // !CLIENT_DLL

	return CClient__ProcessStringCmd(pClient, pMsg);
}

//---------------------------------------------------------------------------------
// Purpose: process set convar
// Input  : *pClient - (ADJ)
//			*pMsg - 
// Output : 
//---------------------------------------------------------------------------------
bool CClient::VProcessSetConVar(CClient* pClient, NET_SetConVar* pMsg)
{
#ifndef CLIENT_DLL
	CClient* const pAdj = AdjustShiftedThisPointer(pClient);
	CClientExtended* const pSlot = pAdj->GetClientExtended();

	// This loop never exceeds 255 iterations, NET_SetConVar::ReadFromBuffer(...)
	// reads and inserts up to 255 entries in the vector (reads a byte for size).
	FOR_EACH_VEC(pMsg->m_ConVars, i)
	{
		const NET_SetConVar::cvar_t& entry = pMsg->m_ConVars[i];
		const char* const name = entry.name;
		const char* const value = entry.value;

		// Discard any ConVar change request if it contains funky characters.
		bool bFunky = false;
		for (const char* s = name; *s != '\0'; ++s)
		{
			if (!V_isalnum(*s) && *s != '_')
			{
				bFunky = true;
				break;
			}
		}
		if (bFunky)
		{
			DevWarning(eDLL_T::SERVER, "Ignoring ConVar change request for variable '%s' from client '%s'; invalid characters in the variable name\n",
				name, pAdj->GetClientName());
			continue;
		}

		// The initial set of ConVars must contain all client ConVars that are
		// flagged UserInfo. This is a simple fix to exploits that send bogus
		// data later, and catches bugs, such as new UserInfo ConVars appearing
		// later, which shouldn't happen.
		if (pSlot->m_bInitialConVarsSet && !pAdj->m_ConVars->FindKey(name))
		{
			DevWarning(eDLL_T::SERVER, "UserInfo update from \"%s\" ignored: %s = %s\n",
				pAdj->GetClientName(), name, value);
			continue;
		}

		// Add ConVar to list and set string.
		pAdj->m_ConVars->SetString(name, value);
		DevMsg(eDLL_T::SERVER, "UserInfo update from \"%s\": %s = %s\n", pAdj->GetClientName(), name, value);
	}

	pSlot->m_bInitialConVarsSet = true;
	pAdj->m_bConVarsChanged = true;
#endif // !CLIENT_DLL

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: process voice data
// Input  : *pClient - (ADJ)
//			*pMsg - 
// Output : 
//---------------------------------------------------------------------------------
bool CClient::VProcessVoiceData(CClient* pClient, CLC_VoiceData* pMsg)
{
#ifndef CLIENT_DLL
	char voiceDataBuffer[4096];
	const int bitsRead = pMsg->m_DataIn.ReadBitsClamped(voiceDataBuffer, pMsg->m_nLength);

	if (pMsg->m_DataIn.IsOverflowed())
		return false;

	CClient* const pAdj = AdjustShiftedThisPointer(pClient);
	SV_BroadcastVoiceData(pAdj, Bits2Bytes(bitsRead), voiceDataBuffer);
#endif // !CLIENT_DLL

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: process durango voice data
// Input  : *pClient - (ADJ)
//			*pMsg - 
// Output : 
//---------------------------------------------------------------------------------
bool CClient::VProcessDurangoVoiceData(CClient* pClient, CLC_DurangoVoiceData* pMsg)
{
#ifndef CLIENT_DLL
	char voiceDataBuffer[4096];
	const int bitsRead = pMsg->m_DataIn.ReadBitsClamped(voiceDataBuffer, pMsg->m_nLength);

	if (pMsg->m_DataIn.IsOverflowed())
		return false;

	CClient* const pAdj = AdjustShiftedThisPointer(pClient);
	SV_BroadcastDurangoVoiceData(pAdj, Bits2Bytes(bitsRead), voiceDataBuffer,
		pMsg->m_xid, pMsg->m_unknown, pMsg->m_useVoiceStream, pMsg->m_skipXidCheck);
#endif // !CLIENT_DLL

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: set UserCmd time buffer
// Input  : numUserCmdProcessTicksMax - 
//			tickInterval - 
//---------------------------------------------------------------------------------
void CClientExtended::InitializeMovementTimeForUserCmdProcessing(const int numUserCmdProcessTicksMax, const float tickInterval)
{
	// Grant the client some time buffer to execute user commands
	m_flMovementTimeForUserCmdProcessingRemaining += tickInterval;

	// but never accumulate more than N ticks
	if (m_flMovementTimeForUserCmdProcessingRemaining > numUserCmdProcessTicksMax * tickInterval)
		m_flMovementTimeForUserCmdProcessingRemaining = numUserCmdProcessTicksMax * tickInterval;
}

//---------------------------------------------------------------------------------
// Purpose: consume UserCmd time buffer
// Input  : flTimeNeeded -
// Output : max time allowed for processing
//---------------------------------------------------------------------------------
float CClientExtended::ConsumeMovementTimeForUserCmdProcessing(const float flTimeNeeded)
{
	if (m_flMovementTimeForUserCmdProcessingRemaining <= 0.0f)
		return 0.0f;
	else if (flTimeNeeded > m_flMovementTimeForUserCmdProcessingRemaining + FLT_EPSILON)
	{
		const float flResult = m_flMovementTimeForUserCmdProcessingRemaining;
		m_flMovementTimeForUserCmdProcessingRemaining = 0.0f;

		return flResult;
	}
	else
	{
		m_flMovementTimeForUserCmdProcessingRemaining -= flTimeNeeded;

		if (m_flMovementTimeForUserCmdProcessingRemaining < 0.0f)
			m_flMovementTimeForUserCmdProcessingRemaining = 0.0f;

		return flTimeNeeded;
	}
}

void VClient::Detour(const bool bAttach) const
{
#ifndef CLIENT_DLL
	DetourSetup(&CClient__Clear, &CClient::VClear, bAttach);
	DetourSetup(&CClient__Connect, &CClient::VConnect, bAttach);
	DetourSetup(&CClient__ConnectionStart, &CClient::VConnectionStart, bAttach);
	DetourSetup(&CClient__ActivatePlayer, &CClient::VActivatePlayer, bAttach);
	DetourSetup(&CClient__SendNetMsgEx, &CClient::VSendNetMsgEx, bAttach);
	//DetourSetup(&CClient__SendSnapshot, &CClient::VSendSnapshot, bAttach);
	DetourSetup(&CClient__WriteDataBlock, &CClient::WriteDataBlock, bAttach);

	DetourSetup(&CClient__ProcessStringCmd, &CClient::VProcessStringCmd, bAttach);
	DetourSetup(&CClient__ProcessSetConVar, &CClient::VProcessSetConVar, bAttach);
	DetourSetup(&CClient__ProcessVoiceData, &CClient::VProcessVoiceData, bAttach);
	DetourSetup(&CClient__ProcessDurangoVoiceData, &CClient::VProcessDurangoVoiceData, bAttach);
#endif // !CLIENT_DLL
}
