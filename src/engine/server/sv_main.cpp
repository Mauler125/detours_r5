//===========================================================================//
//
// Purpose:
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier0/threadtools.h"
#include "tier0/frametask.h"
#include "tier1/cvar.h"
#include "engine/server/sv_main.h"
#include "engine/client/client.h"
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"
#include "engine/client/client.h"
#include "server.h"
#include "game/server/gameinterface.h"

//-----------------------------------------------------------------------------
// Purpose: checks if particular client is banned on the comp server
//-----------------------------------------------------------------------------
void SV_CheckForBanAndDisconnect(CClient* const pClient, const string& svIPAddr,
	const NucleusID_t nNucleusID, const string& svPersonaName, const int nPort)
{
	Assert(pClient != nullptr);

	string svError;
	const bool bCompBanned = g_MasterServer.CheckForBan(svIPAddr, nNucleusID, svPersonaName, svError);

	if (bCompBanned)
	{
		g_TaskQueue.Dispatch([pClient, svError, svIPAddr, nNucleusID, nPort]
			{
				// Make sure client isn't already disconnected,
				// and that if there is a valid netchannel, that
				// it hasn't been taken by a different client by
				// the time this task is getting executed.
				const CNetChan* const pChan = pClient->GetNetChan();
				if (pChan && pClient->GetNucleusID() == nNucleusID)
				{
					const int nUserID = pClient->GetUserID();

					pClient->Disconnect(Reputation_t::REP_MARK_BAD, svError.c_str());
					Warning(eDLL_T::SERVER, "Removed client '[%s]:%i' from slot #%i ('%llu' is banned globally!)\n",
						svIPAddr.c_str(), nPort, nUserID, nNucleusID);
				}
			}, 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if particular client is banned on the master server
//-----------------------------------------------------------------------------
void SV_ProcessBulkCheck(const CBanSystem::BannedList_t* const pBannedVec)
{
	CBanSystem::BannedList_t* outBannedVec = nullptr;

	if (!g_MasterServer.GetBannedList(*pBannedVec, &outBannedVec))
		return;

	g_TaskQueue.Dispatch([outBannedVec]
		{
			SV_CheckClientsForBan(outBannedVec);
			delete outBannedVec;
		}, 0);
}

//-----------------------------------------------------------------------------
// Purpose: creates a snapshot of the currently connected clients
// Input  : *pBannedVec - if passed, will check for bans and kick the clients
//-----------------------------------------------------------------------------
void SV_CheckClientsForBan(const CBanSystem::BannedList_t* const pBannedVec /*= nullptr*/)
{
	Assert(ThreadInMainThread());

	CBanSystem::BannedList_t* bannedVec = !pBannedVec 
		? new CBanSystem::BannedList_t 
		: nullptr;

	for (int c = 0; c < gpGlobals->maxClients; c++) // Loop through all possible client instances.
	{
		CClient* const pClient = g_pServer->GetClient(c);
		const CNetChan* const pNetChan = pClient->GetNetChan();

		if (!pNetChan)
			continue;

		if (!pClient->IsConnected())
			continue;

		if (pNetChan->GetRemoteAddress().IsLoopback())
			continue;

		const char* const szIPAddr = pNetChan->GetAddress(true);
		const NucleusID_t nNucleusID = pClient->GetNucleusID();

		// If no banned list was provided, build one with all clients
		// on the server. This will be used for bulk checking so live
		// bans could be performed, as this function is called periodically.
		if (bannedVec)
			bannedVec->AddToTail(CBanSystem::Banned_t(szIPAddr, nNucleusID));
		else
		{
			// Check if current client is within provided banned list, and
			// prune if so...
			FOR_EACH_VEC(*pBannedVec, i)
			{
				const CBanSystem::Banned_t& banned = (*pBannedVec)[i];

				if (banned.m_NucleusID == pClient->GetNucleusID())
				{
					const int nUserID = pClient->GetUserID();
					const int nPort = pNetChan->GetPort();

					pClient->Disconnect(Reputation_t::REP_MARK_BAD, "%s", banned.m_Address.String());
					Warning(eDLL_T::SERVER, "Removed client '[%s]:%i' from slot #%i ('%llu' is banned globally!)\n",
						szIPAddr, nPort, nUserID, nNucleusID);
				}
			}
		}
	}

	if (bannedVec && !bannedVec->IsEmpty())
	{
		std::thread bulkCheck([bannedVec]()
			{
				SV_ProcessBulkCheck(bannedVec);
				delete bannedVec;
			});

		bulkCheck.detach();
	}
	else if (bannedVec)
	{
		delete bannedVec;
		bannedVec = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Purpose: loads the game .dll
//-----------------------------------------------------------------------------
void SV_InitGameDLL()
{
	v_SV_InitGameDLL();
}

//-----------------------------------------------------------------------------
// Purpose: release resources associated with extension DLLs.
//-----------------------------------------------------------------------------
void SV_ShutdownGameDLL()
{
	v_SV_ShutdownGameDLL();
}

//-----------------------------------------------------------------------------
// Purpose: activates the server
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool SV_ActivateServer()
{
	return v_SV_ActivateServer();
}

//-----------------------------------------------------------------------------
// Purpose: returns whether voice data can be broadcasted from the server
//-----------------------------------------------------------------------------
bool SV_CanBroadcastVoice()
{
	if (IsPartyDedi())
		return false;

	if (IsTrainingDedi())
		return false;

	if (!sv_voiceenable->GetBool())
		return false;

	if (gpGlobals->maxClients <= 0)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: relays voice data to other clients
//-----------------------------------------------------------------------------
void SV_BroadcastVoiceData(CClient* const cl, const int nBytes, char* const data)
{
	if (!SV_CanBroadcastVoice())
		return;

	SVC_VoiceData voiceData(cl->GetUserID(), nBytes, data);

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CClient* const pClient = g_pServer->GetClient(i);

		// is this client fully connected
		if (pClient->GetSignonState() != SIGNONSTATE::SIGNONSTATE_FULL)
			continue;

		// is this client the sender
		if (pClient == cl && !sv_voiceEcho->GetBool())
			continue;

		// is this client on the sender's team
		if (pClient->GetTeamNum() != cl->GetTeamNum() && !sv_alltalk->GetBool())
			continue;

		//if (voice_noxplat->GetBool() && cl->GetXPlatID() != pClient->GetXPlatID())
		//{
		//	if ((cl->GetXPlatID() -1) > 1 || (pClient->GetXPlatID() -1) > 1)
		//		continue;
		//}

		CNetChan* const pNetChan = pClient->GetNetChan();

		if (!pNetChan)
			continue;

		// if voice stream has enough space for new data
		if (pNetChan->GetStreamVoice().GetNumBitsLeft() >= 8 * nBytes + 96)
			pClient->SendNetMsgEx(&voiceData, false, false, true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: relays durango voice data to other clients
//-----------------------------------------------------------------------------
void SV_BroadcastDurangoVoiceData(CClient* const cl, const int nBytes, char* const data,
	const int nXid, const int unknown, const bool useVoiceStream, const bool skipXidCheck)
{
	if (!SV_CanBroadcastVoice())
		return;

	SVC_DurangoVoiceData voiceData(cl->GetUserID(), nBytes, data, unknown, useVoiceStream);

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CClient* const pClient = g_pServer->GetClient(i);

		// is this client fully connected
		if (pClient->GetSignonState() != SIGNONSTATE::SIGNONSTATE_FULL)
			continue;

		// is this client the sender
		if (pClient == cl && !sv_voiceEcho->GetBool())
			continue;

		if (!skipXidCheck && i != nXid)
			continue;

		// is this client on the sender's team
		if (pClient->GetTeamNum() != cl->GetTeamNum() && !sv_alltalk->GetBool())
		{
			// NOTE: on Durango packets, the game appears to bypass the team
			// check if 'useVoiceStream' is false, thus forcing the usage
			// of the reliable stream. Omitted the check as it appears that
			// could be exploited to transmit voice to other teams while cvar
			// 'sv_alltalk' is unset.
			continue;
		}

		// NOTE: xplat code checks disabled; CClient::GetXPlatID() seems to be
		// an enumeration of platforms, but the enum hasn't been reversed yet.
		//if (voice_noxplat->GetBool() && cl->GetXPlatID() != pClient->GetXPlatID())
		//{
		//	if ((cl->GetXPlatID() - 1) > 1 || (pClient->GetXPlatID() - 1) > 1)
		//		continue;
		//}

		CNetChan* const pNetChan = pClient->GetNetChan();

		if (!pNetChan)
			continue;

		// NOTE: the game appears to have the ability to use the unreliable
		// stream as well, but the condition to hit that code path can never
		// evaluate to true - appears to be a compile time option that hasn't
		// been fully optimized away? For now only switch between voice and
		// reliable streams as that is what the original code does.
		const bf_write& stream = useVoiceStream ? pNetChan->GetStreamVoice() : pNetChan->GetStreamReliable();

		// if stream has enough space for new data
		if (stream.GetNumBitsLeft() >= 8 * nBytes + 34)
			pClient->SendNetMsgEx(&voiceData, false, !useVoiceStream, useVoiceStream);
	}
}
