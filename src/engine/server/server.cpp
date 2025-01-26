//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// server.cpp: implementation of the CServer class.
//
/////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "common/protocol.h"
#include "tier0/frametask.h"
#include "tier1/cvar.h"
#include "tier1/strtools.h"
#include "engine/server/sv_main.h"
#include "engine/server/server.h"
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"
#include "ebisusdk/EbisuSDK.h"
#include "public/edict.h"
#include "pluginsystem/pluginsystem.h"
#include "rtech/liveapi/liveapi.h"
#include "game/server/gameinterface.h"

//---------------------------------------------------------------------------------
// Console variables
//---------------------------------------------------------------------------------
ConVar sv_showconnecting("sv_showconnecting", "1", FCVAR_RELEASE, "Logs information about the connecting client to the console");

ConVar sv_pylonVisibility("sv_pylonVisibility", "0", FCVAR_RELEASE, "Determines the visibility to the Pylon master server.", "0 = Offline, 1 = Hidden, 2 = Public.");
ConVar sv_pylonRefreshRate("sv_pylonRefreshRate", "5.0", FCVAR_DEVELOPMENTONLY, "Pylon host refresh rate (seconds).");

ConVar sv_globalBanlist("sv_globalBanlist", "1", FCVAR_RELEASE, "Determines whether or not to use the global banned list.", false, 0.f, false, 0.f, "0 = Disable, 1 = Enable.");
ConVar sv_banlistRefreshRate("sv_banlistRefreshRate", "30.0", FCVAR_DEVELOPMENTONLY, "Banned list refresh rate (seconds).", true, 1.f, false, 0.f);

static ConVar sv_validatePersonaName("sv_validatePersonaName", "1", FCVAR_RELEASE, "Validate the client's textual persona name on connect.");
static ConVar sv_minPersonaNameLength("sv_minPersonaNameLength", "4", FCVAR_RELEASE, "The minimum length of the client's textual persona name.", true, 0.f, false, 0.f);
static ConVar sv_maxPersonaNameLength("sv_maxPersonaNameLength", "16", FCVAR_RELEASE, "The maximum length of the client's textual persona name.", true, 0.f, false, 0.f);

//---------------------------------------------------------------------------------
// Purpose: Gets the number of human players on the server
// Output : int
//---------------------------------------------------------------------------------
int CServer::GetNumHumanPlayers(void) const
{
	int nHumans = 0;
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		const CClient* const pClient = g_pServer->GetClient(i);

		if (pClient->IsHumanPlayer())
			nHumans++;
	}

	return nHumans;
}

//---------------------------------------------------------------------------------
// Purpose: Gets the number of fake clients on the server
// Output : int
//---------------------------------------------------------------------------------
int CServer::GetNumFakeClients(void) const
{
	int nBots = 0;
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		const CClient* const pClient = g_pServer->GetClient(i);

		if (pClient->IsConnected() && pClient->IsFakeClient())
			nBots++;
	}

	return nBots;
}

//---------------------------------------------------------------------------------
// Purpose: Gets the number of clients on the server
// Output : int
//---------------------------------------------------------------------------------
int CServer::GetNumClients(void) const
{
	int nClients = 0;
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		const CClient* const pClient = g_pServer->GetClient(i);

		if (pClient->IsConnected())
			nClients++;
	}

	return nClients;
}

//---------------------------------------------------------------------------------
// Purpose: Rejects connection request and sends back a message
// Input  : iSocket - 
//			*pChallenge - 
//			*szMessage - 
//---------------------------------------------------------------------------------
void CServer::RejectConnection(int iSocket, netadr_t* pNetAdr, const char* szMessage)
{
	CServer__RejectConnection(this, iSocket, pNetAdr, szMessage);
}

//---------------------------------------------------------------------------------
// Purpose: Initializes a CSVClient for a new net connection. This will only be called
//			once for a player each game, not once for each level change.
// Input  : *pServer - 
//			*pChallenge - 
// Output : pointer to client instance on success, nullptr on failure
//---------------------------------------------------------------------------------
CClient* CServer::ConnectClient(CServer* pServer, user_creds_s* pChallenge)
{
	if (pServer->m_State < server_state_t::ss_active)
		return nullptr;

	char* pszPersonaName = pChallenge->personaName;
	NucleusID_t nNucleusID = pChallenge->personaId;

	char pszAddresBuffer[128]; // Render the client's address.
	pChallenge->netAdr.ToString(pszAddresBuffer, sizeof(pszAddresBuffer), true);

	const bool bEnableLogging = sv_showconnecting.GetBool();
	const int nPort = int(ntohs(pChallenge->netAdr.GetPort()));

	if (bEnableLogging)
		Msg(eDLL_T::SERVER, "Processing connectionless challenge for '[%s]:%i' ('%llu')\n",
			pszAddresBuffer, nPort, nNucleusID);

	bool bValidName = false;

	if (VALID_CHARSTAR(pszPersonaName) &&
		V_IsValidUTF8(pszPersonaName))
	{
		if (sv_validatePersonaName.GetBool() && 
			!IsValidPersonaName(pszPersonaName, sv_minPersonaNameLength.GetInt(), sv_maxPersonaNameLength.GetInt()))
		{
			bValidName = false;
		}
		else
		{
			bValidName = true;
		}
	}

	// Only proceed connection if the client's name is valid and UTF-8 encoded.
	if (!bValidName)
	{
		pServer->RejectConnection(pServer->m_Socket, &pChallenge->netAdr, "#Valve_Reject_Invalid_Name");
		if (bEnableLogging)
			Warning(eDLL_T::SERVER, "Connection rejected for '[%s]:%i' ('%llu' has an invalid name!)\n",
				pszAddresBuffer, nPort, nNucleusID);

		return nullptr;
	}

	if (g_BanSystem.IsBanListValid())
	{
		if (g_BanSystem.IsBanned(pszAddresBuffer, nNucleusID))
		{
			pServer->RejectConnection(pServer->m_Socket, &pChallenge->netAdr, "#Valve_Reject_Banned");
			if (bEnableLogging)
				Warning(eDLL_T::SERVER, "Connection rejected for '[%s]:%i' ('%llu' is banned from this server!)\n",
					pszAddresBuffer, nPort, nNucleusID);

			return nullptr;
		}
	}

	CClient* pClient = CServer__ConnectClient(pServer, pChallenge);

	for (auto& callback : !g_PluginSystem.GetConnectClientCallbacks())
	{
		if (!callback.Function()(pServer, pClient, pChallenge))
		{
			pClient->Disconnect(REP_MARK_BAD, "#Valve_Reject_Banned");
			return nullptr;
		}
	}

	if (pClient && sv_globalBanlist.GetBool())
	{
		if (!pClient->GetNetChan()->GetRemoteAddress().IsLoopback())
		{
			const string addressBufferCopy(pszAddresBuffer);
			const string personaNameCopy(pszPersonaName);

			std::thread th(SV_CheckForBanAndDisconnect, pClient, addressBufferCopy, nNucleusID, personaNameCopy, nPort);
			th.detach();
		}
	}

	return pClient;
}

//---------------------------------------------------------------------------------
// Purpose: Sends netmessage to all active clients
// Input  : *msg       -
//          onlyActive - 
//          reliable   - 
//---------------------------------------------------------------------------------
void CServer::BroadcastMessage(CNetMessage* const msg, const bool onlyActive, const bool reliable)
{
	CServer__BroadcastMessage(this, msg, onlyActive, reliable);
}

//---------------------------------------------------------------------------------
// Purpose: Runs the server frame job
// Input  : flFrameTime - 
//			bRunOverlays - 
//			bUpdateFrame - 
//---------------------------------------------------------------------------------
void CServer::FrameJob(double flFrameTime, bool bRunOverlays, bool bUpdateFrame)
{
	CServer__FrameJob(flFrameTime, bRunOverlays, bUpdateFrame);
	LiveAPISystem()->RunFrame();
}

//---------------------------------------------------------------------------------
// Purpose: Runs the server frame
// Input  : *pServer - 
//---------------------------------------------------------------------------------
void CServer::RunFrame(CServer* pServer)
{
	CServer__RunFrame(pServer);
}

///////////////////////////////////////////////////////////////////////////////
void VServer::Detour(const bool bAttach) const
{
	DetourSetup(&CServer__RunFrame, &CServer::RunFrame, bAttach);
	DetourSetup(&CServer__ConnectClient, &CServer::ConnectClient, bAttach);
	DetourSetup(&CServer__FrameJob, &CServer::FrameJob, bAttach);
}


///////////////////////////////////////////////////////////////////////////////
CServer* g_pServer = nullptr;
CClientExtended CServer::sm_ClientsExtended[MAX_PLAYERS];
