//===============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//
//===============================================================================//
// netmessages.cpp: implementation of the CNetMessage types.
//
///////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "engine/net.h"
#include "common/netmessages.h"
#include "common/callback.h"
#include "game/shared/usermessages.h"

///////////////////////////////////////////////////////////////////////////////////
// re-implementation of 'SVC_Print::Process'
///////////////////////////////////////////////////////////////////////////////////
bool SVC_Print::ProcessImpl()
{
	if (this->m_szText)
	{
		Assert(m_szText == m_szTextBuffer); // Should always point to 'm_szTextBuffer'.

		size_t len = strnlen_s(m_szText, sizeof(m_szTextBuffer));
		Assert(len < sizeof(m_szTextBuffer));

		if (len < sizeof(m_szTextBuffer))
		{
			Msg(eDLL_T::SERVER, m_szText[len-1] == '\n' ? "%s" : "%s\n", m_szText);
		}
	}

	return true; // Original just return true also.
}

///////////////////////////////////////////////////////////////////////////////////
// re-implementation of 'SVC_UserMessage::Process'
///////////////////////////////////////////////////////////////////////////////////
bool SVC_UserMessage::ProcessImpl()
{
	if (m_nMsgType == UserMessages_t::TextMsg)
	{
		bf_read buf = m_DataIn;
		byte type = byte(buf.ReadByte());

		if (type == HUD_PRINTCONSOLE ||
			type == HUD_PRINTCENTER)
		{
			char text[MAX_USER_MSG_DATA];
			int len;

			buf.ReadString(text, sizeof(text), false, &len);
			Assert(len < sizeof(text));

			if (len && len < sizeof(text))
			{
				Msg(eDLL_T::SERVER, text[len - 1] == '\n' ? "%s" : "%s\n", text);
			}
		}
	}

	return SVC_UserMessage_Process(this); // Need to return original.
}

///////////////////////////////////////////////////////////////////////////////////
// Net message to change class settings vars on the client
///////////////////////////////////////////////////////////////////////////////////
bool SVC_SetClassVar::ReadFromBuffer(bf_read* buffer)
{
	const bool key = buffer->ReadString(m_szKey, sizeof(m_szKey));
	const bool val = buffer->ReadString(m_szValue, sizeof(m_szValue));

	return key && val;
}
bool SVC_SetClassVar::WriteToBuffer(bf_write* buffer)
{
	const bool key = buffer->WriteString(m_szKey);
	const bool val = buffer->WriteString(m_szValue);

	return key && val;
}
bool SVC_SetClassVar::Process(void)
{
	const char* pArgs[3] = {
		"_setClassVarClient",
		m_szKey,
		m_szValue
	};

	CCommand command((int)V_ARRAYSIZE(pArgs), pArgs, cmd_source_t::kCommandSrcCode);
	v__setClassVarClient_f(command);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////
// Below functions are hooked as playlist overrides can be abused from the client.
// The client could basically manage the server's playlists. Only allow read/write
// when cheats are enabled.
///////////////////////////////////////////////////////////////////////////////////
bool CLC_SetPlaylistVarOverride::ReadFromBufferImpl(CLC_SetPlaylistVarOverride* thisptr, bf_read* buffer)
{
	// Abusable netmsg; only allow if cheats are enabled.
	if (!sv_cheats->GetBool())
	{
		return false;
	}

	return CLC_SetPlaylistVarOverride_ReadFromBuffer(thisptr, buffer);
}
bool CLC_SetPlaylistVarOverride::WriteToBufferImpl(CLC_SetPlaylistVarOverride* thisptr, bf_write* buffer)
{
	// Abusable netmsg; only allow if cheats are enabled.
	if (!sv_cheats->GetBool())
	{
		return false;
	}

	return CLC_SetPlaylistVarOverride_WriteToBuffer(thisptr, buffer);
}

static ConVar enable_CmdKeyValues("enable_CmdKeyValues", "0", FCVAR_DEVELOPMENTONLY, "Toggle CmdKeyValues transmit and receive.");

///////////////////////////////////////////////////////////////////////////////////
// below functions are hooked as 'CmdKeyValues' isn't really used in this game, but
// still exploitable on the server. the 'OnPlayerAward' command calls the function
// 'UTIL_SendClientCommandKVToPlayer' which forwards the keyvalues to all connected clients.
///////////////////////////////////////////////////////////////////////////////////
bool Base_CmdKeyValues::ReadFromBufferImpl(Base_CmdKeyValues* thisptr, bf_read* buffer)
{
	// Abusable netmsg; only allow if explicitly enabled by the client.
	if (!enable_CmdKeyValues.GetBool())
	{
		return false;
	}

	return Base_CmdKeyValues_ReadFromBuffer(thisptr, buffer);
}
bool Base_CmdKeyValues::WriteToBufferImpl(Base_CmdKeyValues* thisptr, bf_write* buffer)
{
	// Abusable netmsg; only allow if explicitly enabled by the client.
	if (!enable_CmdKeyValues.GetBool())
	{
		return false;
	}

	return Base_CmdKeyValues_WriteToBuffer(thisptr, buffer);
}

///////////////////////////////////////////////////////////////////////////////////
// determine whether or not the message can be copied into the replay buffer,
// regardless of the 'CNetMessage::m_Group' type.
///////////////////////////////////////////////////////////////////////////////////
bool CanReplayMessage(const CNetMessage* msg)
{
	switch (msg->GetType())
	{
	// String commands can be abused in a way they get executed
	// on the client that is watching a replay. This happens as
	// the server copies the message into the replay buffer from
	// the client that initially submitted it. Its group type is
	// 'None', so call this to determine whether or not to set
	// the group type to 'NoReplay'. This exploit has been used
	// to connect clients to an arbitrary server during replay.
	case NetMessageType::net_StringCmd:
	// Print and user messages sometimes make their way to the
	// client that is watching a replay, while it should only
	// be broadcasted to the target client. This happens for the 
	// same reason as the 'net_StringCmd' above.
	case NetMessageType::svc_Print:
	{
		return false;
	}
	case NetMessageType::svc_UserMessage:
	{
		SVC_UserMessage* userMsg = (SVC_UserMessage*)msg;

		// Just don't replay console prints.
		if (userMsg->m_nMsgType == UserMessages_t::TextMsg)
		{
			return false;
		}

		return true;
	}
	default:
	{
		return true;
	}
	}
}

void V_NetMessages::Detour(const bool bAttach) const
{
	if (bAttach)
	{
		auto hk_SVCPrint_Process = &SVC_Print::ProcessImpl;
		auto hk_SVCUserMessage_Process = &SVC_UserMessage::ProcessImpl;

		CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VFTable, (LPVOID&)hk_SVCPrint_Process, NetMessageVtbl::Process, (LPVOID*)&SVC_Print_Process);
		CMemory::HookVirtualMethod((uintptr_t)g_pSVC_UserMessage_VFTable, (LPVOID&)hk_SVCUserMessage_Process, NetMessageVtbl::Process, (LPVOID*)&SVC_UserMessage_Process);
		CMemory::HookVirtualMethod((uintptr_t)g_pBase_CmdKeyValues_VFTable, (LPVOID*)&Base_CmdKeyValues::ReadFromBufferImpl, NetMessageVtbl::ReadFromBuffer, (LPVOID*)&Base_CmdKeyValues_ReadFromBuffer);
		CMemory::HookVirtualMethod((uintptr_t)g_pBase_CmdKeyValues_VFTable, (LPVOID*)&Base_CmdKeyValues::WriteToBufferImpl, NetMessageVtbl::WriteToBuffer, (LPVOID*)&Base_CmdKeyValues_WriteToBuffer);
		CMemory::HookVirtualMethod((uintptr_t)g_pCLC_SetPlaylistVarOverride_VFTable, (LPVOID*)&CLC_SetPlaylistVarOverride::ReadFromBufferImpl, NetMessageVtbl::ReadFromBuffer, (LPVOID*)&CLC_SetPlaylistVarOverride_ReadFromBuffer);
		CMemory::HookVirtualMethod((uintptr_t)g_pCLC_SetPlaylistVarOverride_VFTable, (LPVOID*)&CLC_SetPlaylistVarOverride::WriteToBufferImpl, NetMessageVtbl::WriteToBuffer, (LPVOID*)&CLC_SetPlaylistVarOverride_WriteToBuffer);
	}
	else
	{
		void* hkRestore = nullptr;
		CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VFTable, (LPVOID)SVC_Print_Process, NetMessageVtbl::Process, (LPVOID*)&hkRestore);
		CMemory::HookVirtualMethod((uintptr_t)g_pSVC_UserMessage_VFTable, (LPVOID)SVC_UserMessage_Process, NetMessageVtbl::Process, (LPVOID*)&hkRestore);
		CMemory::HookVirtualMethod((uintptr_t)g_pBase_CmdKeyValues_VFTable, (LPVOID)Base_CmdKeyValues_ReadFromBuffer, NetMessageVtbl::ReadFromBuffer, (LPVOID*)&hkRestore);
		CMemory::HookVirtualMethod((uintptr_t)g_pBase_CmdKeyValues_VFTable, (LPVOID)Base_CmdKeyValues_WriteToBuffer, NetMessageVtbl::WriteToBuffer, (LPVOID*)&hkRestore);
		CMemory::HookVirtualMethod((uintptr_t)g_pCLC_SetPlaylistVarOverride_VFTable, (LPVOID)CLC_SetPlaylistVarOverride_ReadFromBuffer, NetMessageVtbl::ReadFromBuffer, (LPVOID*)&hkRestore);
		CMemory::HookVirtualMethod((uintptr_t)g_pCLC_SetPlaylistVarOverride_VFTable, (LPVOID)CLC_SetPlaylistVarOverride_WriteToBuffer, NetMessageVtbl::WriteToBuffer, (LPVOID*)&hkRestore);
	}
}
