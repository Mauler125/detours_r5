#pragma once
#include "tier1/NetAdr.h"
#include "networksystem/pylon.h"
#include "engine/client/client.h"
#include "engine/networkstringtable.h"
#include "public/iserver.h"
#ifndef CLIENT_DLL
#include "server/vengineserver_impl.h"
#endif // !CLIENT_DLL

enum class server_state_t
{
	ss_dead = 0,	// Dead
	ss_loading,		// Spawning
	ss_active,		// Running
	ss_paused,		// Running, but paused
};

struct user_creds_s
{
	netadr_t netAdr;
	int32_t  protocolVer;
	int32_t  challenge;
	uint32_t reservation;
	uint64_t personaId;
	char* personaName;
};

class CServer : public IConnectionlessPacketHandler
{
public:
	int	GetTick(void) const { return m_nTickCount; }
#ifndef CLIENT_DLL // Only the connectionless packet handler is implemented on the client via the IServer base class.
	int GetNumHumanPlayers(void) const;
	int GetNumFakeClients(void) const;
	int GetNumClients(void) const;
	const char* GetMapName(void) const { return m_szMapname; }
	const char* GetMapGroupName(void) const { return m_szMapGroupName; }
	int GetNumClasses(void) const { return m_nServerClasses; }
	int GetClassBits(void) const { return m_nServerClassBits; }
	float GetCPUUsage(void) const { return m_fCPUPercent; }
	bool IsActive(void) const { return m_State >= server_state_t::ss_active; }
	bool IsLoading(void) const { return m_State == server_state_t::ss_loading; }
	bool IsDedicated(void) const { return m_bIsDedicated; }
	void RejectConnection(int iSocket, netadr_t* pNetAdr, const char* szMessage);
	static CClient* ConnectClient(CServer* pServer, user_creds_s* pChallenge);
	static void RunFrame(CServer* pServer);
	static void FrameJob(double flFrameTime, bool bRunOverlays, bool bUniformSnapshotInterval);
#endif // !CLIENT_DLL

private:
	server_state_t                m_State;                       // some actions are only valid during load
	int                           m_Socket;                      // network socket 
	int                           m_nTickCount;                  // current server tick
	bool                          m_bResetMaxTeams;              // reset max players on the server
	char                          m_szMapname[MAX_MAP_NAME];     // map name and path without extension
	char                          m_szMapGroupName[64];          // map group name
	char                          m_szPassword[32];              // server password
	uint32_t                      m_WorldmapCRC;                 // for detecting that client has a hacked local copy of map, the client will be dropped if this occurs.
	uint32_t                      m_ClientDllCRC;                // the dll that this server is expecting clients to be using.
	CNetworkStringTableContainer* m_StringTables;                // network string table container
	CNetworkStringTable*          m_pInstanceBaselineTable;      // instancebaseline
	CNetworkStringTable*          m_pLightStyleTable;            // lightstyles
	CNetworkStringTable*          m_pUserInfoTable;              // userinfo
	CNetworkStringTable*          m_pServerQueryTable;           // server_query_inf
	bool                          m_bReplay;                     // MAYBE
	bool                          m_bUpdateFrame;                // perform snapshot update
	bool                          m_bUseReputation;              // use of player reputation on the server
	bool                          m_bSimulating;                 // are we simulating or not
	bf_write                      m_Signon;                      // signon bitbuf
	CUtlMemory<byte>              m_SignonBuffer;                // signon memory
	int                           m_nServerClasses;              // number of unique server classes
	int                           m_nServerClassBits;            // log2 of serverclasses
	char                          m_szHostInfo[128];             // see '[r5apex_ds.exe + 0x237740]' for more details. fmt: '[IPv6]:PORT:TIMEi64u'
	char                          m_nGap0[520];
	int                           m_nSpawnCount;
	char                          m_nGap1[116];
	float                         m_fCPUPercent;
	float                         m_fStartTime;
	float                         m_fLastCPUCheckTime;
	char                          m_nGap2[303108];               // TODO: Reverse the rest in this gap.
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	char                          m_nGap3[0x80];
#endif
};
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
static_assert(sizeof(CServer) == 0x4A440);
#else
static_assert(sizeof(CServer) == 0x4A4C0);
#endif

extern CServer* g_pServer;

/* ==== CSERVER ========================================================================================================================================================= */
inline CMemory p_CServer_FrameJob;
inline auto v_CServer_FrameJob = p_CServer_FrameJob.RCast<void (*)(double flFrameTime, bool bRunOverlays, bool bUniformSnapshotInterval)>();

inline CMemory p_CServer_RunFrame;
inline auto v_CServer_RunFrame = p_CServer_RunFrame.RCast<void (*)(CServer* pServer)>();

inline CMemory p_CServer_ConnectClient;
inline auto v_CServer_ConnectClient = p_CServer_ConnectClient.RCast<CClient* (*)(CServer* pServer, user_creds_s* pCreds)>();

inline CMemory p_CServer_RejectConnection;
inline auto v_CServer_RejectConnection = p_CServer_RejectConnection.RCast<void* (*)(CServer* pServer, int iSocket, netadr_t* pNetAdr, const char* szMessage)>();

///////////////////////////////////////////////////////////////////////////////
class VServer : public IDetour
{
	virtual void GetAdr(void) const
	{
#ifndef CLIENT_DLL
		LogFunAdr("CServer::FrameJob", p_CServer_FrameJob.GetPtr());
		LogFunAdr("CServer::RunFrame", p_CServer_RunFrame.GetPtr());
		LogFunAdr("CServer::ConnectClient", p_CServer_ConnectClient.GetPtr());
		LogFunAdr("CServer::RejectConnection", p_CServer_RejectConnection.GetPtr());
		LogVarAdr("g_Server[128]", reinterpret_cast<uintptr_t>(g_pServer));
#endif // !CLIENT_DLL
	}
	virtual void GetFun(void) const
	{
#ifndef CLIENT_DLL
		p_CServer_FrameJob = g_GameDll.FindPatternSIMD("48 89 6C 24 ?? 56 41 54 41 56");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CServer_ConnectClient = g_GameDll.FindPatternSIMD("44 89 44 24 ?? 55 56 57 48 8D AC 24 ?? ?? ?? ??");
#elif defined (GAMEDLL_S2)
		p_CServer_ConnectClient = g_GameDll.FindPatternSIMD("44 89 44 24 ?? 56 57 48 81 EC ?? ?? ?? ??");
#else
		p_CServer_ConnectClient = g_GameDll.FindPatternSIMD("40 55 57 41 55 41 57 48 8D AC 24 ?? ?? ?? ??");
#endif

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CServer_RunFrame = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 56 57 48 81 EC ?? ?? ?? ?? 0F 29 B4 24 ?? ?? ?? ??");
#else
		p_CServer_RunFrame = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 88 05 ?? ?? ?? ??").FollowNearCallSelf();
#endif
		p_CServer_RejectConnection = g_GameDll.FindPatternSIMD("4C 89 4C 24 ?? 53 55 56 57 48 81 EC ?? ?? ?? ?? 49 8B D9");

		v_CServer_FrameJob = p_CServer_FrameJob.RCast<void (*)(double, bool, bool)>();                                       /*48 89 6C 24 ?? 56 41 54 41 56*/
		v_CServer_RunFrame = p_CServer_RunFrame.RCast<void (*)(CServer*)>();
		v_CServer_ConnectClient = p_CServer_ConnectClient.RCast<CClient* (*)(CServer*, user_creds_s*)>();                     /*40 55 57 41 55 41 57 48 8D AC 24 ?? ?? ?? ??*/
		v_CServer_RejectConnection = p_CServer_RejectConnection.RCast<void* (*)(CServer*, int, netadr_t*, const char*)>();   /*4C 89 4C 24 ?? 53 55 56 57 48 81 EC ?? ?? ?? ?? 49 8B D9*/
#endif // !CLIENT_DLL
	}
	virtual void GetVar(void) const
	{
#ifndef CLIENT_DLL
		g_pServer = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 0F BF D1").FindPatternSelf("48 8D 3D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CServer*>();
#endif // !CLIENT_DLL
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
