//===========================================================================//
// 
// Purpose: Shared rcon utilities.
// 
//===========================================================================//
#include "core/stdafx.h"
#include "base_rcon.h"
#include "shared_rcon.h"
#include "protoc/netcon.pb.h"

//-----------------------------------------------------------------------------
// Purpose: serialize message to vector
// Input  : *pBase - 
//			&vecBuf - 
//			*pResponseMsg - 
//			*pResponseVal - 
//			responseType - 
//			nMessageId - 
//			nMessageType - 
//			bEncrypt - 
//			bDebug - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool NetconServer_Serialize(const CNetConBase* pBase, vector<char>& vecBuf, const char* pResponseMsg, const char* pResponseVal,
	const netcon::response_e responseType, const int nMessageId, const int nMessageType, const bool bEncrypt, const bool bDebug)
{
	netcon::response response;

	response.set_messageid(nMessageId);
	response.set_messagetype(nMessageType);
	response.set_responsetype(responseType);
	response.set_responsemsg(pResponseMsg);
	response.set_responseval(pResponseVal);

	if (!NetconShared_PackEnvelope(pBase, vecBuf, response.ByteSizeLong(), &response, bEncrypt, bDebug))
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: serialize message to vector
// Input  : *pBase - 
//			&vecBuf - 
//			*szReqBuf - 
//			*szReqVal - 
//			*requestType - 
//			bEncrypt - 
//			bDebug - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool NetconClient_Serialize(const CNetConBase* pBase, vector<char>& vecBuf, const char* szReqBuf,
	const char* szReqVal, const netcon::request_e requestType, const bool bEncrypt, const bool bDebug)
{
	netcon::request request;

	request.set_messageid(-1);
	request.set_requesttype(requestType);
	request.set_requestmsg(szReqBuf);
	request.set_requestval(szReqVal);

	if (!NetconShared_PackEnvelope(pBase, vecBuf, request.ByteSizeLong(), &request, bEncrypt, bDebug))
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: attempt to connect to remote
// Input  : *pBase - 
//			*pHostAdr - 
//			nHostPort - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool NetconClient_Connect(CNetConBase* pBase, const char* pHostAdr, const int nHostPort)
{
	string svLocalHost;
	const bool bValidSocket = nHostPort != SOCKET_ERROR;

	if (bValidSocket && (strcmp(pHostAdr, "localhost") == 0))
	{
		char szHostName[512];
		if (!gethostname(szHostName, sizeof(szHostName)))
		{
			svLocalHost = Format("[%s]:%i", szHostName, nHostPort);
			pHostAdr = svLocalHost.c_str();
		}
	}

	CNetAdr* pNetAdr = pBase->GetNetAddress();
	if (!pNetAdr->SetFromString(pHostAdr, true))
	{
		Error(eDLL_T::CLIENT, NO_ERROR, "Failed to set RCON address: %s\n", pHostAdr);
		return false;
	}

	// Pass 'SOCKET_ERROR' if you want to set port from address string instead.
	if (bValidSocket)
	{
		pNetAdr->SetPort(htons(u_short(nHostPort)));
	}

	CSocketCreator* pCreator = pBase->GetSocketCreator();
	if (pCreator->ConnectSocket(*pNetAdr, true) == SOCKET_ERROR)
	{
		return false;
	}

	Msg(eDLL_T::CLIENT, "Connected to: %s\n", pNetAdr->ToString());
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: packs a message envelope
// Input  : *pBase - 
//			&outMsgBuf - 
//			nMsgLen - 
//			*inMsg - 
//			bEncrypt - 
//			bDebug - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool NetconShared_PackEnvelope(const CNetConBase* pBase, vector<char>& outMsgBuf, const size_t nMsgLen,
	google::protobuf::MessageLite* inMsg, const bool bEncrypt, const bool bDebug)
{
	char* encodeBuf = new char[nMsgLen];
	std::unique_ptr<char[]> encodedContainer(encodeBuf);

	if (!pBase->Encode(inMsg, encodeBuf, nMsgLen))
	{
		if (bDebug)
		{
			Error(eDLL_T::ENGINE, NO_ERROR, "Failed to encode RCON message data\n");
		}

		return false;
	}

	netcon::envelope envelope;
	envelope.set_encrypted(bEncrypt);

	const char* dataBuf = encodeBuf;
	std::unique_ptr<char[]> container;

	if (bEncrypt)
	{
		char* encryptBuf = new char[nMsgLen];
		container.reset(encryptBuf);

		CryptoContext_s ctx;
		if (!pBase->Encrypt(ctx, encodeBuf, encryptBuf, nMsgLen))
		{
			if (bDebug)
			{
				Error(eDLL_T::ENGINE, NO_ERROR, "Failed to encrypt RCON message data\n");
			}

			return false;
		}

		envelope.set_nonce(ctx.ivData, sizeof(ctx.ivData));
		dataBuf = encryptBuf;
	}

	envelope.set_data(dataBuf, nMsgLen);
	const size_t envelopeSize = envelope.ByteSizeLong();

	outMsgBuf.resize(envelopeSize);

	if (!pBase->Encode(&envelope, &outMsgBuf[0], envelopeSize))
	{
		if (bDebug)
		{
			Error(eDLL_T::ENGINE, NO_ERROR, "Failed to encode RCON message envelope\n");
		}

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: unpacks a message envelope
// Input  : *pBase - 
//			*pMsgBuf - 
//			nMsgLen - 
//			*outMsg - 
//			bEncrypt - 
//			bDebug - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool NetconShared_UnpackEnvelope(const CNetConBase* pBase, const char* pMsgBuf, const size_t nMsgLen,
	google::protobuf::MessageLite* outMsg, const bool bDebug)
{
	netcon::envelope envelope;

	if (!pBase->Decode(&envelope, pMsgBuf, nMsgLen))
	{
		if (bDebug)
		{
			Error(eDLL_T::ENGINE, NO_ERROR, "Failed to decode RCON message envelope\n");
		}

		return false;
	}

	const size_t msgLen = envelope.data().size();

	if (msgLen > RCON_MAX_PAYLOAD_SIZE)
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "Data in RCON message envelope is too large (%zu > %zu)\n",
			msgLen, RCON_MAX_PAYLOAD_SIZE);

		return false;
	}

	const char* netMsg = envelope.data().c_str();
	const char* dataBuf = netMsg;

	std::unique_ptr<char[]> container;

	if (envelope.encrypted())
	{
		char* decryptBuf = new char[msgLen];
		container.reset(decryptBuf);

		const size_t ivLen = envelope.nonce().size();

		if (ivLen != sizeof(CryptoIV_t))
		{
			if (bDebug)
			{
				Error(eDLL_T::ENGINE, NO_ERROR, "Nonce in RCON message envelope is invalid (%zu != %zu)\n",
					ivLen, sizeof(CryptoIV_t));
			}

			return false;
		}

		CryptoContext_s ctx;
		memcpy(ctx.ivData, envelope.nonce().data(), ivLen);

		if (!pBase->Decrypt(ctx, netMsg, decryptBuf, msgLen))
		{
			if (bDebug)
			{
				Error(eDLL_T::ENGINE, NO_ERROR, "Failed to decrypt RCON message data\n");
			}

			return false;
		}

		dataBuf = decryptBuf;
	}

	Assert(dataBuf);

	if (!pBase->Decode(outMsg, dataBuf, msgLen))
	{
		if (bDebug)
		{
			Error(eDLL_T::ENGINE, NO_ERROR, "Failed to decode RCON message data\n");
		}

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netconsole data
// Input  : *pBase - 
//			iSocket - 
// Output : nullptr on failure
//-----------------------------------------------------------------------------
CConnectedNetConsoleData* NetconShared_GetConnData(CNetConBase* pBase, const int iSocket)
{
	CSocketCreator* pCreator = pBase->GetSocketCreator();
	Assert(iSocket >= 0 && (pCreator->GetAcceptedSocketCount() == 0
		|| iSocket < pCreator->GetAcceptedSocketCount()));

	if (!pCreator->GetAcceptedSocketCount())
	{
		return nullptr;
	}

	return &pCreator->GetAcceptedSocketData(iSocket);
}

//-----------------------------------------------------------------------------
// Purpose: gets the netconsole socket
// Input  : *pBase - 
//			iSocket - 
// Output : SOCKET_ERROR (-1) on failure
//-----------------------------------------------------------------------------
SocketHandle_t NetconShared_GetSocketHandle(CNetConBase* pBase, const int iSocket)
{
	const CConnectedNetConsoleData* pData = NetconShared_GetConnData(pBase, iSocket);
	if (!pData)
	{
		return SOCKET_ERROR;
	}

	return pData->m_hSocket;
}

#ifndef _TOOLS

#ifndef CLIENT_DLL
#include "engine/server/sv_rcon.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "engine/client/cl_rcon.h"
#endif // !DEDICATED

void RCON_KeyChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData);
void RCON_PasswordChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData);

ConVar rcon_debug("rcon_debug", "0", FCVAR_RELEASE, "Show rcon debug information ( !slower! )");
ConVar rcon_encryptframes("rcon_encryptframes", "1", FCVAR_RELEASE, "Whether to encrypt RCON messages");
ConVar rcon_key("rcon_key", "", FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Base64 remote server access encryption key (random if empty or invalid)", &RCON_KeyChanged_f);

//-----------------------------------------------------------------------------
// Purpose: change RCON key on server and client
//-----------------------------------------------------------------------------
void RCON_KeyChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData)
{
	if (ConVar* pConVarRef = g_pCVar->FindVar(pConVar->GetName()))
	{
		const char* pNewString = pConVarRef->GetString();

		if (strcmp(pOldString, pNewString) == NULL)
			return; // Same key.


#if !defined(DEDICATED) && !defined(CLIENT_DLL)
		RCONServer()->SetKey(pNewString);
		RCONClient()->SetKey(RCONServer()->GetKey()); // Sync server & client keys

		Msg(eDLL_T::ENGINE, "Installed RCON Key: %s'%s%s%s'\n",
			g_svReset.c_str(), g_svGreyB.c_str(), RCONClient()->GetKey(), g_svReset.c_str());
#else
#ifdef DEDICATED
		RCONServer()->SetKey(pNewString);

		Msg(eDLL_T::SERVER, "Installed RCON Key: %s'%s%s%s'\n",
			g_svReset.c_str(), g_svGreyB.c_str(), RCONServer()->GetKey(), g_svReset.c_str());
#endif // DEDICATED
#ifdef CLIENT_DLL
		RCONClient()->SetKey(pNewString);

		Msg(eDLL_T::CLIENT, "Installed RCON Key: %s'%s%s%s'\n",
			g_svReset.c_str(), g_svGreyB.c_str(), RCONClient()->GetKey(), g_svReset.c_str());
#endif // CLIENT_DLL

#endif // !DEDICATED && !CLIENT_DLL
	}
}

#ifndef CLIENT_DLL
void RCON_InitServerAndTrySyncKeys(const char* pPassword)
{
#ifndef DEDICATED
	RCONServer()->Init(pPassword, rcon_key.GetString());

	if (RCONServer()->IsInitialized())
	{
		// Sync server & client keys
		RCONClient()->SetKey(RCONServer()->GetKey());
	}
#else
	RCONServer()->Init(pPassword, rcon_key.GetString());
#endif // !DEDICATED
}
#endif // !CLIENT_DLL

#ifndef DEDICATED
void RCON_InitClientAndTrySyncKeys()
{
#ifndef CLIENT_DLL
	if (RCONServer()->IsInitialized())
	{
		// Sync server & client keys
		RCONClient()->Init(RCONServer()->GetKey());
	}
	else
#endif // !CLIENT_DLL
	{
		RCONClient()->Init(rcon_key.GetString());
	}
}
#endif // !DEDICATED

#endif // !_TOOLS
