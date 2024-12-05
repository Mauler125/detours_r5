//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// clientstate.cpp: implementation of the CClientState class.
//
/////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "mathlib/bitvec.h"
#include "tier0/frametask.h"
#include "engine/common.h"
#include "engine/host.h"
#include "engine/host_cmd.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // !CLIENT_DLL
#include "clientstate.h"
#include "common/callback.h"
#include "cdll_engine_int.h"
#include "vgui/vgui_baseui_interface.h"
#include "rtech/playlists/playlists.h"
#include <ebisusdk/EbisuSDK.h>
#include <engine/cmd.h>

//------------------------------------------------------------------------------
// Purpose: console command callbacks
//------------------------------------------------------------------------------
static void SetName_f(const CCommand& args)
{
    if (args.ArgC() < 2)
        return;

    if (!IsOriginDisabled())
        return;

    const char* pszName = args.Arg(1);

    if (!pszName[0])
        pszName = "unnamed";

    const size_t nLen = strlen(pszName);

    if (nLen >= MAX_PERSONA_NAME_LEN)
        return;

    // Update nucleus name.
    strncpy(g_PersonaName, pszName, nLen+1);
    name_cvar->SetValue(pszName);
}
static void Reconnect_f(const CCommand& args)
{
    g_pClientState->Reconnect();
}

//------------------------------------------------------------------------------
// Purpose: console commands
//------------------------------------------------------------------------------
static ConCommand cl_setname("cl_setname", SetName_f, "Sets the client's persona name", FCVAR_RELEASE);
static ConCommand reconnect("reconnect", Reconnect_f, "Reconnect to current server.", FCVAR_DONTRECORD|FCVAR_RELEASE);

//------------------------------------------------------------------------------
// Purpose: returns true if client simulation is paused
//------------------------------------------------------------------------------
bool CClientState::IsPaused() const
{
	return m_bPaused || !*host_initialized || g_pEngineVGui->ShouldPause();
}

//------------------------------------------------------------------------------
// Purpose: returns true if client is fully connected and active
//------------------------------------------------------------------------------
bool CClientState::IsActive(void) const
{
    return m_nSignonState == SIGNONSTATE::SIGNONSTATE_FULL;
};

//------------------------------------------------------------------------------
// Purpose: returns true if client connected but not active
//------------------------------------------------------------------------------
bool CClientState::IsConnected(void) const
{
    return m_nSignonState >= SIGNONSTATE::SIGNONSTATE_CONNECTED;
};

//------------------------------------------------------------------------------
// Purpose: returns true if client is still connecting
//------------------------------------------------------------------------------
bool CClientState::IsConnecting(void) const
{
    return m_nSignonState >= SIGNONSTATE::SIGNONSTATE_NONE;
}

//------------------------------------------------------------------------------
// Purpose: gets the client time
// Technically doesn't belong here
//------------------------------------------------------------------------------
float CClientState::GetClientTime() const
{
    if (m_bClockCorrectionEnabled)
    {
        return (float)m_ClockDriftMgr.m_nSimulationTick * g_pCommonHostState->interval_per_tick;
    }
    else
    {
        return m_flClockDriftFrameTime;
    }
}

//------------------------------------------------------------------------------
// Purpose: gets the simulation tick count
//------------------------------------------------------------------------------
int CClientState::GetTick() const
{
    return m_ClockDriftMgr.m_nSimulationTick;
}

//------------------------------------------------------------------------------
// Purpose: gets the last-received server tick count
//------------------------------------------------------------------------------
int CClientState::GetServerTickCount() const
{
    return m_ClockDriftMgr.m_nServerTick;
}

//------------------------------------------------------------------------------
// Purpose: sets the server tick count
//------------------------------------------------------------------------------
void CClientState::SetServerTickCount(int tick)
{
    m_ClockDriftMgr.m_nServerTick = tick;
}

//------------------------------------------------------------------------------
// Purpose: gets the client tick count
//------------------------------------------------------------------------------
int CClientState::GetClientTickCount() const
{
    return m_ClockDriftMgr.m_nClientTick;
}

//------------------------------------------------------------------------------
// Purpose: sets the client tick count
//------------------------------------------------------------------------------
void CClientState::SetClientTickCount(int tick)
{
    m_ClockDriftMgr.m_nClientTick = tick;
}

//------------------------------------------------------------------------------
// Purpose: gets the client frame time
//------------------------------------------------------------------------------
float CClientState::GetFrameTime() const
{
    if (IsPaused())
    {
        return 0.0f;
    }

    return m_flFrameTime;
}

//---------------------------------------------------------------------------------
// Purpose: registers net messages
// Input  : *pClient - 
//			*pChan - 
// Output : true if setup was successful, false otherwise
//---------------------------------------------------------------------------------
bool CClientState::VConnectionStart(CClientState* pClient, CNetChan* pChan)
{
    pClient->RegisterNetMsgs(pChan);
    return CClientState__ConnectionStart(pClient, pChan);
}

//------------------------------------------------------------------------------
// Purpose: called when connection to the server has been closed
//------------------------------------------------------------------------------
void CClientState::VConnectionClosing(CClientState* thisptr, const char* szReason)
{
    CClientState__ConnectionClosing(thisptr, szReason);

    // Delay execution to the next frame; this is required to avoid a rare crash.
    // Cannot reload playlists while still disconnecting.
    g_TaskQueue.Dispatch([]()
        {
            // Reload the local playlist to override the cached
            // one from the server we got disconnected from.
            v_Playlists_Download_f();
            Playlists_SDKInit();
        }, 0);
}

//------------------------------------------------------------------------------
// Purpose: called when a SVC_ServerTick messages comes in.
// This function has an additional check for the command tick against '-1',
// if it is '-1', we process statistics only. This is required as the game
// no longer can process server ticks every frame unlike previous games.
// Without this, the server CPU and frame time don't get updated to the client.
//------------------------------------------------------------------------------
bool CClientState::VProcessServerTick(CClientState* thisptr, SVC_ServerTick* msg)
{
    if (msg->m_NetTick.m_nCommandTick != -1)
    {
        // Updates statistics and updates clockdrift.
        return CClientState__ProcessServerTick(thisptr, msg);
    }
    else // Statistics only.
    {
        CClientState* const thisptr_ADJ = thisptr->GetShiftedBasePointer();

        if (thisptr_ADJ->IsConnected())
        {
            CNetChan* const pChan = thisptr_ADJ->m_NetChannel;

            pChan->SetRemoteFramerate(msg->m_NetTick.m_flHostFrameTime, msg->m_NetTick.m_flHostFrameTimeStdDeviation);
            pChan->SetRemoteCPUStatistics(msg->m_NetTick.m_nServerCPU);
        }

        return true;
    }
}

//------------------------------------------------------------------------------
// Purpose: processes string commands sent from server
// Input  : *thisptr - 
//          *msg     - 
// Output : true on success, false otherwise
//------------------------------------------------------------------------------
bool CClientState::_ProcessStringCmd(CClientState* thisptr, NET_StringCmd* msg)
{
    CClientState* const thisptr_ADJ = thisptr->GetShiftedBasePointer();

    if (thisptr_ADJ->m_bRestrictServerCommands
#ifndef CLIENT_DLL
        // Don't restrict commands if we are on our own listen server
        && !g_pServer->IsActive()
#endif // !CLIENT_DLL
        )
    {
        CCommand args;
        args.Tokenize(msg->cmd, cmd_source_t::kCommandSrcInvalid);

        if (args.ArgC() > 0)
        {
            if (!Cbuf_AddTextWithMarkers(msg->cmd,
                eCmdExecutionMarker_Enable_FCVAR_SERVER_CAN_EXECUTE,
                eCmdExecutionMarker_Disable_FCVAR_SERVER_CAN_EXECUTE))
            {
                DevWarning(eDLL_T::CLIENT, "%s: No room for %i execution markers; command \"%s\" ignored\n",
                    __FUNCTION__, 2, msg->cmd);
            }

            return true;
        }
    }
    else
    {
        Cbuf_AddText(Cbuf_GetCurrentPlayer(), msg->cmd, cmd_source_t::kCommandSrcCode);
    }

    return true;
}

//------------------------------------------------------------------------------
// Purpose: create's string tables from string table data sent from server
// Input  : *thisptr - 
//          *msg     - 
// Output : true on success, false otherwise
//------------------------------------------------------------------------------
bool CClientState::_ProcessCreateStringTable(CClientState* thisptr, SVC_CreateStringTable* msg)
{
    CClientState* const cl = thisptr->GetShiftedBasePointer();

    if (!cl->IsConnected())
        return false;

    CNetworkStringTableContainer* const container = cl->m_StringTableContainer;

    // Must have a string table container at this point!
    if (!container)
    {
        Assert(0);

        COM_ExplainDisconnection(true, "String table container missing.\n");
        v_Host_Disconnect(true);

        return false;
    }

    container->AllowCreation(true);
    const ssize_t startbit = msg->m_DataIn.GetNumBitsRead();

    CNetworkStringTable* const table = (CNetworkStringTable*)container->CreateStringTable(false, msg->m_szTableName,
        msg->m_nMaxEntries, msg->m_nUserDataSize, msg->m_nUserDataSizeBits, msg->m_nDictFlags);

    table->SetTick(cl->GetServerTickCount());
    CClientState__HookClientStringTable(cl, msg->m_szTableName);

    if (msg->m_bDataCompressed)
    {
        // TODO[ AMOS ]: check sizes before proceeding to decode
        // the string tables
        unsigned int msgUncompressedSize = msg->m_DataIn.ReadLong();
        unsigned int msgCompressedSize = msg->m_DataIn.ReadLong();

        size_t uncompressedSize = msgUncompressedSize;
        size_t compressedSize = msgCompressedSize;

        bool bSuccess = false;

        // TODO[ AMOS ]: this could do better. The engine does UINT_MAX-3
        // which doesn't look very great. Clamp to more reasonable values
        // than UINT_MAX-3 or UINT_MAX/2? The largest string tables sent
        // are settings layout string tables which are roughly 256KiB
        // compressed with LZSS. perhaps clamp this to something like 16MiB?
        if (msg->m_DataIn.TotalBytesAvailable() > 0 && 
            msgCompressedSize <= (unsigned int)msg->m_DataIn.TotalBytesAvailable() &&
            msgCompressedSize < UINT_MAX / 2 && msgUncompressedSize < UINT_MAX / 2)
        {
            // allocate buffer for uncompressed data, align to 4 bytes boundary
            uint8_t* const uncompressedBuffer = new uint8_t[PAD_NUMBER(msgUncompressedSize, 4)];
            uint8_t* const compressedBuffer = new uint8_t[PAD_NUMBER(msgCompressedSize, 4)];

            msg->m_DataIn.ReadBytes(compressedBuffer, msgCompressedSize);

            // uncompress data
            bSuccess = NET_BufferToBufferDecompress(compressedBuffer, compressedSize, uncompressedBuffer, uncompressedSize);
            bSuccess &= (uncompressedSize == msgUncompressedSize);

            if (bSuccess)
            {
                bf_read data(uncompressedBuffer, (int)uncompressedSize);
                table->ParseUpdate(data, msg->m_nNumEntries);
            }

            delete[] uncompressedBuffer;
            delete[] compressedBuffer;
        }

        if (!bSuccess)
        {
            Assert(false);
            DevWarning(eDLL_T::CLIENT, "%s: Received malformed string table message!\n", __FUNCTION__);
        }
    }
    else
    {
        table->ParseUpdate(msg->m_DataIn, msg->m_nNumEntries);
    }

    container->AllowCreation(false);
    const ssize_t endbit = msg->m_DataIn.GetNumBitsRead();

    return (endbit - startbit) == msg->m_nLength;
}

//------------------------------------------------------------------------------
// Purpose: processes user message data
// Input  : *thisptr - 
//          *msg     - 
// Output : true on success, false otherwise
//------------------------------------------------------------------------------
bool CClientState::_ProcessUserMessage(CClientState* thisptr, SVC_UserMessage* msg)
{
    CClientState* const cl = thisptr->GetShiftedBasePointer();

    if (!cl->IsConnected())
        return false;

    // buffer for incoming user message
    ALIGN4 byte userdata[MAX_USER_MSG_DATA] ALIGN4_POST = { 0 };
    bf_read userMsg("UserMessage(read)", userdata, sizeof(userdata));

    int bitsRead = msg->m_DataIn.ReadBitsClamped(userdata, msg->m_nLength);
    userMsg.StartReading(userdata, Bits2Bytes(bitsRead));

    // dispatch message to client.dll
    if (!g_pHLClient->DispatchUserMessage(msg->m_nMsgType, &userMsg))
    {
        Warning(eDLL_T::CLIENT, "Couldn't dispatch user message (%i)\n", msg->m_nMsgType);
        return false;
    }

    return true;
}

static ConVar cl_onlineAuthEnable("cl_onlineAuthEnable", "1", FCVAR_RELEASE, "Enables the client-side online authentication system");

static ConVar cl_onlineAuthToken("cl_onlineAuthToken", "", FCVAR_HIDDEN | FCVAR_USERINFO | FCVAR_DONTRECORD | FCVAR_SERVER_CANNOT_QUERY | FCVAR_PLATFORM_SYSTEM, "The client's online authentication token");
static ConVar cl_onlineAuthTokenSignature1("cl_onlineAuthTokenSignature1", "", FCVAR_HIDDEN | FCVAR_USERINFO | FCVAR_DONTRECORD | FCVAR_SERVER_CANNOT_QUERY | FCVAR_PLATFORM_SYSTEM, "The client's online authentication token signature", false, 0.f, false, 0.f, "Primary");
static ConVar cl_onlineAuthTokenSignature2("cl_onlineAuthTokenSignature2", "", FCVAR_HIDDEN | FCVAR_USERINFO | FCVAR_DONTRECORD | FCVAR_SERVER_CANNOT_QUERY | FCVAR_PLATFORM_SYSTEM, "The client's online authentication token signature", false, 0.f, false, 0.f, "Secondary");

//------------------------------------------------------------------------------
// Purpose: get authentication token for current connection context
// Input  : *connectParams - 
//          *reasonBuf     - 
//          reasonBufLen   - 
// Output : true on success, false otherwise
//------------------------------------------------------------------------------
bool CClientState::Authenticate(connectparams_t* connectParams, char* const reasonBuf, const size_t reasonBufLen) const
{
#define FORMAT_ERROR_REASON(fmt, ...) V_snprintf(reasonBuf, reasonBufLen, fmt, ##__VA_ARGS__);

    string msToken; // token returned by the masterserver authorising the client to play online
    string message; // message returned by the masterserver about the result of the auth

    // verify that the client is not lying about their account identity
    // code is immediately discarded upon verification

    const bool ret = g_MasterServer.AuthForConnection(*g_NucleusID, connectParams->netAdr, g_OriginAuthCode, msToken, message);
    if (!ret)
    {
        FORMAT_ERROR_REASON("%s", message.c_str());
        return false;
    }

    // get full token
    const char* token = msToken.c_str();

    // get a pointer to the delimiter that begins the token's signature
    const char* tokenSignatureDelim = strrchr(token, '.');

    if (!tokenSignatureDelim)
    {
        FORMAT_ERROR_REASON("Invalid token returned by MS");
        return false;
    }

    // replace the delimiter with a null char so the first cvar only takes the header and payload data
    *(char*)tokenSignatureDelim = '\0';
    const size_t sigLength = strlen(tokenSignatureDelim) - 1;

    cl_onlineAuthToken.SetValue(token);

    if (sigLength > 0)
    {
        // get a pointer to the first part of the token signature to store in cl_onlineAuthTokenSignature1
        const char* tokenSignaturePart1 = tokenSignatureDelim + 1;

        cl_onlineAuthTokenSignature1.SetValue(tokenSignaturePart1);

        if (sigLength > 255)
        {
            // get a pointer to the rest of the token signature to store in cl_onlineAuthTokenSignature2
            const char* tokenSignaturePart2 = tokenSignaturePart1 + 255;

            cl_onlineAuthTokenSignature2.SetValue(tokenSignaturePart2);
        }
    }

    return true;
#undef REJECT_CONNECTION
}

bool IsLocalHost(connectparams_t* connectParams)
{
    return (strstr(connectParams->netAdr, "localhost") || strstr(connectParams->netAdr, "127.0.0.1"));
}

void CClientState::VConnect(CClientState* thisptr, connectparams_t* connectParams)
{
    if (cl_onlineAuthEnable.GetBool() && !IsLocalHost(connectParams))
    {
        char authFailReason[512];

        if (!thisptr->Authenticate(connectParams, authFailReason, sizeof(authFailReason)))
        {
            COM_ExplainDisconnection(true, "Failed to authenticate for online play: %s", authFailReason);
            return;
        }
    }

    CClientState__Connect(thisptr, connectParams);
}

//------------------------------------------------------------------------------
// Purpose: reconnects to currently connected server
//------------------------------------------------------------------------------
void CClientState::Reconnect()
{
    if (!IsConnected())
    {
        Warning(eDLL_T::CLIENT, "Attempted to reconnect while unconnected, please defer it.\n");
        return;
    }

    const netadr_t& remoteAdr = m_NetChannel->GetRemoteAddress();

    // NOTE: technically the engine supports running "connect localhost" to
    // reconnect to the listen server without killing it, however when running
    // this, an engine error occurs "Couldn't create a world static vertex buffer\n"
    // Needs investigation.
    if (remoteAdr.IsLoopback() || NET_IsRemoteLocal(remoteAdr))
    {
        Warning(eDLL_T::CLIENT, "Reconnecting to a listen server isn't supported, use \"reload\" instead.\n");
        return;
    }

    char buf[1024];
    V_snprintf(buf, sizeof(buf), "connect \"%s\"", remoteAdr.ToString());

    Cbuf_AddText(ECommandTarget_t::CBUF_FIRST_PLAYER, buf, cmd_source_t::kCommandSrcCode);
}

//---------------------------------------------------------------------------------
// Purpose: registers net messages
// Input  : *chan
//---------------------------------------------------------------------------------
void CClientState::RegisterNetMsgs(CNetChan* chan)
{
    REGISTER_SVC_MSG(SetClassVar);
}

void VClientState::Detour(const bool bAttach) const
{
    DetourSetup(&CClientState__ConnectionStart, &CClientState::VConnectionStart, bAttach);
    DetourSetup(&CClientState__ConnectionClosing, &CClientState::VConnectionClosing, bAttach);
    DetourSetup(&CClientState__ProcessStringCmd, &CClientState::_ProcessStringCmd, bAttach);
    DetourSetup(&CClientState__ProcessServerTick, &CClientState::VProcessServerTick, bAttach);
    DetourSetup(&CClientState__ProcessCreateStringTable, &CClientState::_ProcessCreateStringTable, bAttach);
    DetourSetup(&CClientState__ProcessUserMessage, &CClientState::_ProcessUserMessage, bAttach);
    DetourSetup(&CClientState__Connect, &CClientState::VConnect, bAttach);
}

/////////////////////////////////////////////////////////////////////////////////
CClientState* g_pClientState = nullptr;
CClientState** g_pClientState_Shifted = nullptr;
