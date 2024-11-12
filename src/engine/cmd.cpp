#include "core/stdafx.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "tier1/commandbuffer.h"
#include "engine/cmd.h"

CCommandBuffer** s_pCommandBuffer = nullptr; // array size = ECommandTarget_t::CBUF_COUNT.
LPCRITICAL_SECTION s_pCommandBufferMutex = nullptr;

//=============================================================================
// List of execution markers
//=============================================================================
CUtlVector<int>* g_pExecutionMarkers = nullptr;

//-----------------------------------------------------------------------------
// Purpose: checks if there's room left for execution markers
// Input  : cExecutionMarkers - 
// Output : true if there's room for execution markers, false otherwise
//-----------------------------------------------------------------------------
bool Cbuf_HasRoomForExecutionMarkers(const int cExecutionMarkers)
{
	return (g_pExecutionMarkers->Count() + cExecutionMarkers) < MAX_EXECUTION_MARKERS;
}

//-----------------------------------------------------------------------------
// Purpose: adds command text at the end of the command buffer with execution markers
// Input  : *pText      - 
//          markerLeft  - 
//          markerRight - 
// Output : true if there's room for execution markers, false otherwise
//-----------------------------------------------------------------------------
bool Cbuf_AddTextWithMarkers(const char* const pText, const ECmdExecutionMarker markerLeft, const ECmdExecutionMarker markerRight)
{
	if (Cbuf_HasRoomForExecutionMarkers(2))
	{
		Cbuf_AddExecutionMarker(Cbuf_GetCurrentPlayer(), markerLeft);
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), pText, cmd_source_t::kCommandSrcCode);
		Cbuf_AddExecutionMarker(Cbuf_GetCurrentPlayer(), markerRight);

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: adds command text at the end of the buffer
//-----------------------------------------------------------------------------
//void Cbuf_AddText(ECommandTarget_t eTarget, const char* pText, int nTickDelay)
//{
//	LOCK_COMMAND_BUFFER();
//	if (!s_pCommandBuffer[(int)eTarget]->AddText(pText, nTickDelay, cmd_source_t::kCommandSrcInvalid))
//	{
//		Error(eDLL_T::ENGINE, NO_ERROR, "%s: buffer overflow\n", __FUNCTION__);
//	}
//}

//-----------------------------------------------------------------------------
// Purpose: Sends the entire command line over to the server
// Input  : *args - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
#ifndef DEDICATED
ConVar cl_quota_stringCmdsPerSecond("cl_quota_stringCmdsPerSecond", "32", FCVAR_RELEASE, "How many string commands per second user is allowed to submit, 0 to allow all submissions.", true, 0.f, false, 0.f);
#endif // DEDICATED

bool Cmd_ForwardToServer(const CCommand* args)
{
#ifndef DEDICATED
	// Client -> Server command throttling.
	static double flForwardedCommandQuotaStartTime = -1;
	static int nForwardedCommandQuotaCount = 0;

	// No command to forward.
	if (args->ArgC() == 0)
		return false;

	const double flStartTime = Plat_FloatTime();
	const int nCmdQuotaLimit = cl_quota_stringCmdsPerSecond.GetInt();
	const char* pszCmdString = nullptr;

	// Special case: "cmd whatever args..." is forwarded as "whatever args...";
	// in this case we strip "cmd" from the input.
	if (Q_strcasecmp(args->Arg(0), "cmd") == 0)
		pszCmdString = args->ArgS();
	else
		pszCmdString = args->GetCommandString();

	if (nCmdQuotaLimit)
	{
		if (flStartTime - flForwardedCommandQuotaStartTime >= 1.0)
		{
			flForwardedCommandQuotaStartTime = flStartTime;
			nForwardedCommandQuotaCount = 0;
		}
		++nForwardedCommandQuotaCount;

		if (nForwardedCommandQuotaCount > nCmdQuotaLimit)
		{
			// If we are over quota commands per second, dump this on the floor.
			// If we spam the server with too many commands, it will kick us.
			Warning(eDLL_T::CLIENT, "Command '%s' ignored (submission quota of '%d' per second exceeded!)\n", pszCmdString, nCmdQuotaLimit);
			return false;
		}
	}
	return v_Cmd_ForwardToServer(args);
#else // !DEDICATED
	Assert(0);
	return false; // Client only.
#endif // DEDICATED
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: execute commands directly (ignores all protection flags)
// Input  : *pCommandString - 
//          *pValueString   - 
// Output : true on success, false otherwise
// 
// NOTE   : this function is dangerous, as it allows execution of any command
//          without restrictions. Currently, this is only enabled on the
//          dedicated server for the local console input and RCON, as they both
//          are considered secure (local console needs physical access to the
//          terminal application, RCON requires authentication and its protocol
//          is secure. Do not use this anywhere else without a valid reason !!!
// 
// NOTE   : if client support is ever considered (unlikely), then the convar
//          flag 'FCVAR_MATERIAL_THREAD_MASK' probably needs to be taken into
//          account as well, also, change the DLL context of the warning to
//          ENGINE if the client ever utilizes this.
//-----------------------------------------------------------------------------
bool Cmd_ExecuteUnrestricted(const char* const pCommandString, const char* const pValueString)
{
	ConCommandBase* const pCommandBase = g_pCVar->FindCommandBase(pCommandString);

	if (!pCommandBase)
	{
		// Found nothing.
		Warning(eDLL_T::SERVER, "Command '%s' doesn't exist; request '%s' ignored\n", pCommandString, pValueString);
		return false;
	}

	if (pCommandBase->IsFlagSet(FCVAR_SERVER_FRAME_THREAD))
		ThreadJoinServerJob();

	if (!pCommandBase->IsCommand())
	{
		// Here we want to skip over the command string in the value buffer.
		// So if we got 'sv_cheats 1' in our value buffer, we want to skip
		// over 'sv_cheats ', so that we are pointing directly to the value.
		const char* pFound = V_strstr(pValueString, pCommandString);
		const char* pValue = nullptr;

		if (pFound)
		{
			pValue = pFound + V_strlen(pCommandString);

			// Skip any leading space characters.
			while (*pValue == ' ')
			{
				++pValue;
			}
		}

		ConVar* const pConVar = reinterpret_cast<ConVar*>(pCommandBase);
		pConVar->SetValue(pValue ? pValue : pValueString);
	}
	else // Invoke command callback directly.
	{
		CCommand cmd;

		// Only tokenize if we actually have strings in the value buffer, some
		// commands (like 'status') don't need any additional parameters.
		if (VALID_CHARSTAR(pValueString))
		{
			cmd.Tokenize(pValueString, cmd_source_t::kCommandSrcCode);
		}

		v_Cmd_Dispatch(ECommandTarget_t::CBUF_SERVER, pCommandBase, &cmd, false);
	}

	return true;
}
#endif // !CLIENT_DLL

///////////////////////////////////////////////////////////////////////////////
void VCmd::Detour(const bool bAttach) const
{
	DetourSetup(&v_Cmd_ForwardToServer, &Cmd_ForwardToServer, bAttach);
}
