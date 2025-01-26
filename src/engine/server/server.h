#pragma once
#include "tier0/frametask.h"
#include "tier1/NetAdr.h"
#include "networksystem/pylon.h"
#include "engine/client/client.h"
#include "engine/networkstringtable.h"
#include "public/iserver.h"
#ifndef CLIENT_DLL
#include "vengineserver_impl.h"
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
	NucleusID_t personaId;
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

	inline const char* GetMapName(void) const { return m_szMapname; }
	inline const char* GetMapGroupName(void) const { return m_szMapGroupName; }

	inline int GetNumClasses(void) const { return m_nServerClasses; }
	inline int GetClassBits(void) const { return m_nServerClassBits; }

	inline int GetSpawnCount(void) const { return m_nSpawnCount; }
	inline int GetMaxClients(void) const { return m_nMaxClients; }

	inline int64_t GetMaxTeams(void) const { return m_iMaxTeams; }
	inline CClient* GetClient(const int nIndex) { Assert(nIndex >= NULL && nIndex < MAX_PLAYERS); return &m_Clients[nIndex]; }
	inline CClientExtended* GetClientExtended(const int nIndex) { Assert(nIndex >= NULL && nIndex < MAX_PLAYERS); return &sm_ClientsExtended[nIndex]; }

	inline float GetTime(void) const { return m_nTickCount * m_flTickInterval; }
	inline float GetCPUUsage(void) const { return m_fCPUPercent; }

	inline bool IsActive(void) const { return m_State >= server_state_t::ss_active; }
	inline bool IsLoading(void) const { return m_State == server_state_t::ss_loading; }
	inline bool IsDedicated(void) const { return m_bIsDedicated; }

	void RejectConnection(int iSocket, netadr_t* pNetAdr, const char* szMessage);
	static CClient* ConnectClient(CServer* pServer, user_creds_s* pChallenge);

	void BroadcastMessage(CNetMessage* const msg, const bool onlyActive, const bool reliable);
	static void RunFrame(CServer* pServer);
	static void FrameJob(double flFrameTime, bool bRunOverlays, bool bUpdateFrame);
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
	int                           m_nMaxClients;
	char                          gap_3C0[8];                    // Unknown count (something for teams), see '[r5apex_ds.exe + 0x2777E9]'
	int64_t                       m_iMaxTeams;
	float                         m_flTickInterval;              // Time for 1 tick in seconds
	float                         m_flTimescale;                 // The game time scale (multiplied in conjunction with host_timescale)
	char                          gap_3D8[40];
	CClient                       m_Clients[MAX_PLAYERS];
	char                          gap_25263c0[48];
	float                         m_fCPUPercent;
	float                         m_fStartTime;
	float                         m_fLastCPUCheckTime;
	bool                          m_bTeams[MAX_TEAMS];           // Something with teams, unclear what this does; see '[r5apex_ds.exe + 0x30CE40]'

	// Maps directly to m_Clients, contains extended client data which we
	// cannot add to the CClient class as it would otherwise mismatch the
	// structure in the engine.
	static CClientExtended sm_ClientsExtended[MAX_PLAYERS];
};
static_assert(sizeof(CServer) == 0x25264C0);

extern CServer* g_pServer;

extern ConVar sv_showconnecting;

extern ConVar sv_pylonVisibility;
extern ConVar sv_pylonRefreshRate;

extern ConVar sv_globalBanlist;
extern ConVar sv_banlistRefreshRate;

/* ==== CSERVER ========================================================================================================================================================= */
inline void(*CServer__FrameJob)(double flFrameTime, bool bRunOverlays, bool bUpdateFrame);
inline void(*CServer__RunFrame)(CServer* pServer);
inline CClient*(*CServer__ConnectClient)(CServer* pServer, user_creds_s* pCreds);
inline void*(*CServer__RejectConnection)(CServer* pServer, int iSocket, netadr_t* pNetAdr, const char* szMessage);
inline void (*CServer__BroadcastMessage)(CServer* pServer, CNetMessage* const msg, const bool onlyActive, const bool reliable);
inline bool(*CServer__SpawnServer)(CServer* pServer, const char* pszMapName, const char* pszMapGroupName);

///////////////////////////////////////////////////////////////////////////////
class VServer : public IDetour
{
	virtual void GetAdr(void) const
	{
#ifndef CLIENT_DLL
		LogFunAdr("CServer::FrameJob", CServer__FrameJob);
		LogFunAdr("CServer::RunFrame", CServer__RunFrame);
		LogFunAdr("CServer::ConnectClient", CServer__ConnectClient);
		LogFunAdr("CServer::RejectConnection", CServer__RejectConnection);
		LogFunAdr("CServer::BroadcastMessage", CServer__BroadcastMessage);
		LogFunAdr("CServer::SpawnServer", CServer__SpawnServer);
		LogVarAdr("g_Server", g_pServer);
#endif // !CLIENT_DLL
	}
	virtual void GetFun(void) const
	{
#ifndef CLIENT_DLL
		g_GameDll.FindPatternSIMD("48 89 6C 24 ?? 56 41 54 41 56").GetPtr(CServer__FrameJob);
		g_GameDll.FindPatternSIMD("40 55 57 41 55 41 57 48 8D AC 24 ?? ?? ?? ??").GetPtr(CServer__ConnectClient);

		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 88 05 ?? ?? ?? ??").FollowNearCallSelf().GetPtr(CServer__RunFrame);
		g_GameDll.FindPatternSIMD("4C 89 4C 24 ?? 53 55 56 57 48 81 EC ?? ?? ?? ?? 49 8B D9").GetPtr(CServer__RejectConnection);
		g_GameDll.FindPatternSIMD("4C 8B DC 45 88 43 18 56").GetPtr(CServer__BroadcastMessage);
		g_GameDll.FindPatternSIMD("48 8B C4 53 55 56 57 41 54 41 55 41 57").GetPtr(CServer__SpawnServer);
#endif // !CLIENT_DLL
	}
	virtual void GetVar(void) const
	{
#ifndef CLIENT_DLL
		g_pServer = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 0F BF D1").FindPatternSelf("48 8D 3D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CServer*>();
#endif // !CLIENT_DLL
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
