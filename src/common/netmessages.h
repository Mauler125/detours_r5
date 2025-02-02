//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#pragma once
#include "tier1/bitbuf.h"
#include "common/qlimits.h"
#include "public/inetchannel.h"
#include "public/inetmessage.h"
#include "public/inetmsghandler.h"

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

//-------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------
class CNetChan;
class SVC_Print;
class SVC_UserMessage;
class SVC_PlaylistOverrides;
class CLC_SetPlaylistVarOverride;
class Base_CmdKeyValues;

//-------------------------------------------------------------------------
// MM_HEARTBEAT
//-------------------------------------------------------------------------
//inline CMemory MM_Heartbeat__ToString; // server HeartBeat? (baseserver.cpp).

//-------------------------------------------------------------------------
// SVC_Print
//-------------------------------------------------------------------------
inline bool(*SVC_Print_Process)(SVC_Print* thisptr);
inline void* g_pSVC_Print_VFTable = nullptr;

//-------------------------------------------------------------------------
// SVC_UserMessage
//-------------------------------------------------------------------------
inline bool(*SVC_UserMessage_Process)(SVC_UserMessage* thisptr);
inline void* g_pSVC_UserMessage_VFTable = nullptr;

//-------------------------------------------------------------------------
// SVC_ServerTick
//-------------------------------------------------------------------------
inline void* g_pSVC_ServerTick_VFTable = nullptr;

//-------------------------------------------------------------------------
// SVC_VoiceData
//-------------------------------------------------------------------------
inline void* g_pSVC_VoiceData_VFTable = nullptr;

//-------------------------------------------------------------------------
// SVC_PlaylistOverrides
//-------------------------------------------------------------------------
inline void* g_pSVC_PlaylistOverrides_VFTable = nullptr;

//-------------------------------------------------------------------------
// CLC_ClientTick
//-------------------------------------------------------------------------
inline void* g_pCLC_ClientTick_VFTable = nullptr;

//-------------------------------------------------------------------------
// CLC_SetPlaylistVarOverride
//-------------------------------------------------------------------------
inline bool(*CLC_SetPlaylistVarOverride_ReadFromBuffer)(CLC_SetPlaylistVarOverride* thisptr, bf_read* buffer);
inline bool(*CLC_SetPlaylistVarOverride_WriteToBuffer)(CLC_SetPlaylistVarOverride* thisptr, bf_write* buffer);
inline void* g_pCLC_SetPlaylistVarOverride_VFTable = nullptr;

//-------------------------------------------------------------------------
// Base_CmdKeyValues
//-------------------------------------------------------------------------
inline bool(*Base_CmdKeyValues_ReadFromBuffer)(Base_CmdKeyValues* thisptr, bf_read* buffer);
inline bool(*Base_CmdKeyValues_WriteToBuffer)(Base_CmdKeyValues* thisptr, bf_write* buffer);
inline void* g_pBase_CmdKeyValues_VFTable = nullptr;

//-------------------------------------------------------------------------
// Enumeration of the INetMessage class
//-------------------------------------------------------------------------
enum NetMessageVtbl
{
	Destructor = 0,
	SetNetChannel,
	SetReliable,
	Process,
	ReadFromBuffer,
	WriteToBuffer,
	IsReliable,
	GetGroup,
	GetType,
	GetName,
	GetNetChannel,
	ToString,
	GetSize
};

//-------------------------------------------------------------------------
// Enumeration of netmessage types
//-------------------------------------------------------------------------
enum NetMessageType
{
	net_StringCmd                   = 3,
	net_SetConVar                   = 4,
	net_SignonState                 = 5,
	net_MTXUserMsg                  = 6,

	svc_ServerInfo                  = 7,
	svc_SendTable                   = 8,
	svc_ClassInfo                   = 9,
	svc_SetPause                    = 10,
	svc_Playlists                   = 11,
	svc_CreateStringTable           = 12,
	svc_UpdateStringTable           = 13,
	svc_VoiceData                   = 14,
	svc_DurangoVoiceData            = 15,
	svc_Print                       = 16,
	svc_Sounds                      = 17,
	svc_FixAngle                    = 18,
	svc_CrosshairAngle              = 19,
	svc_GrantClientSidePickup       = 20,
	svc_ServerTick                  = 22,
	svc_PersistenceDefFile          = 23,
	svc_UseCachedPersistenceDefFile = 24,
	svc_PersistenceBaseline         = 25,
	svc_PersistenceUpdateVar        = 26,
	svc_PersistenceNotifySaved      = 27,
	svc_DLCNotifyOwnership          = 28,
	svc_MatchmakingETAs             = 29,
	svc_MatchmakingStatus           = 30,
	svc_MTXUserInfo                 = 31,
	svc_PlaylistChange              = 32,
	svc_SetTeam                     = 33,
	svc_PlaylistOverrides           = 34,
	svc_AntiCheat                   = 35,
	svc_AntiCheatChallenge          = 36,
	svc_UserMessage                 = 37,
	svc_Snapshot                    = 40,
	svc_TempEntities                = 41,
	svc_Menu                        = 42,
	svc_CmdKeyValues                = 43,
	svc_DatatableChecksum           = 44,

	clc_ClientInfo                  = 45,
	clc_Move                        = 46,
	clc_VoiceData                   = 47,
	clc_DurangoVoiceData            = 48,
	clc_FileCRCCheck                = 50,
	clc_LoadingProgress             = 52,
	clc_PersistenceRequestSave      = 53,
	clc_PersistenceClientToken      = 54,
	clc_SetClientEntitlements       = 55,
	clc_SetPlaylistVarOverride      = 56,
	clc_ClaimClientSidePickup       = 57,
	clc_CmdKeyValues                = 59,
	clc_ClientTick                  = 60,
	clc_ClientSayText               = 61,
	clc_PINTelemetryData            = 62,
	clc_AntiCheat                   = 63,
	clc_AntiCheatChallenge          = 64,
	clc_GamepadMsg                  = 65,

	// NOTE: if you make new netmessages, the order should be as follows:
	// - NET (messages shared between server & client).
	// - SVC (messages originating from the server).
	// - CLC (messages originating from the client.
	// Reorder message indices to retain consistency here.
	// --------------------------------------------------------------------
	// From here on, SDK netmessages are enumerated.
	svc_SetClassVar                 = 66,
};

//-------------------------------------------------------------------------
// Enumeration of netmessage groups
//-------------------------------------------------------------------------
enum NetMessageGroup
{
	NoReplay = 2,
};

///////////////////////////////////////////////////////////////////////////////////////
class CNetMessage : public INetMessage
{
public:
	virtual void	SetNetChannel(CNetChan* netchan) { m_NetChannel = netchan; }
	virtual void	SetReliable(bool state) { m_bReliable = state; }
	virtual bool	IsReliable(void) const { return m_bReliable; }
	virtual int		GetGroup(void) const { return m_nGroup; }
	virtual CNetChan* GetNetChannel(void) const { return m_NetChannel; }

	int m_nGroup;
	bool m_bReliable;
	CNetChan* m_NetChannel;
	INetChannelHandler* m_pMessageHandler;
};

///////////////////////////////////////////////////////////////////////////////////////
// server messages:
///////////////////////////////////////////////////////////////////////////////////////
class SVC_CreateStringTable : public CNetMessage
{
public:
	const char* m_szTableName;
	int m_nMaxEntries;
	int m_nNumEntries;
	char m_bUserDataFixedSize;
	char _padding0[3];
	int m_nUserDataSize;
	int m_nUserDataSizeBits;
	int m_nDictFlags;
	int m_nLength;
	bf_read m_DataIn;
	bf_write m_DataOut;
	char m_bDataCompressed;
	char m_szTableNameBuffer[260];
};

class SVC_Print : public CNetMessage
{
public:
	virtual	~SVC_Print() {};

	virtual void	SetNetChannel(CNetChan* netchan) = 0;
	virtual void	SetReliable(bool state) = 0;

	virtual bool	Process(void) = 0; bool ProcessImpl(void);

	virtual	bool	ReadFromBuffer(bf_read* buffer) = 0;
	virtual	bool	WriteToBuffer(bf_write* buffer) = 0;

	virtual bool	IsReliable(void) const = 0;

	virtual int          GetGroup(void) const = 0;
	virtual int          GetType(void) const = 0;
	virtual const char*  GetName(void) const = 0;
	virtual CNetChan* GetNetChannel(void) const = 0;
	virtual const char*  ToString(void) const = 0;
	virtual size_t       GetSize(void) const = 0;

	const void* m_pData;
	const char* m_szText;
private:
	char m_szTextBuffer[2048];
};

class SVC_UserMessage : public CNetMessage
{
public:
	virtual	~SVC_UserMessage() {};

	virtual void	SetNetChannel(CNetChan* netchan) = 0;
	virtual void	SetReliable(bool state) = 0;

	virtual bool	Process(void) = 0; bool ProcessImpl(void);

	virtual	bool	ReadFromBuffer(bf_read* buffer) = 0;
	virtual	bool	WriteToBuffer(bf_write* buffer) = 0;

	virtual bool	IsReliable(void) const = 0;

	virtual int          GetGroup(void) const = 0;
	virtual int          GetType(void) const = 0;
	virtual const char*  GetName(void) const = 0;
	virtual CNetChan* GetNetChannel(void) const = 0;
	virtual const char*  ToString(void) const = 0;
	virtual size_t       GetSize(void) const = 0;

	int			m_nMsgType;
	int			m_nLength;	// data length in bits
	bf_read		m_DataIn;
	bf_write	m_DataOut;
};

class SVC_ServerTick : public CNetMessage
{
public:
	SVC_ServerTick() = default;
	SVC_ServerTick(int tick, float hostFrametime, float hostFrametime_stddeviation, uint8_t cpuPercent)
	{
		void** pVFTable = reinterpret_cast<void**>(this);
		*pVFTable = g_pSVC_ServerTick_VFTable;

		m_bReliable = false;
		m_NetChannel = nullptr;
		m_NetTick.m_nServerTick = tick;
		m_NetTick.m_nClientTick = tick;

		m_NetTick.m_flHostFrameTime = hostFrametime;
		m_NetTick.m_flHostFrameTimeStdDeviation = hostFrametime_stddeviation;
		m_NetTick.m_nServerCPU = cpuPercent;
	};

	virtual	~SVC_ServerTick() {};

	virtual void	SetNetChannel(CNetChan* netchan) { m_NetChannel = netchan; }
	virtual void	SetReliable(bool state) { m_bReliable = state; };

	virtual bool	Process(void)
	{
		return CallVFunc<bool>(NetMessageVtbl::Process, this);
	};
	virtual	bool	ReadFromBuffer(bf_read* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::ReadFromBuffer, this, buffer);
	}
	virtual	bool	WriteToBuffer(bf_write* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::WriteToBuffer, this, buffer);
	}

	virtual bool	IsReliable(void) const { return m_bReliable; };

	virtual int          GetGroup(void) const { return m_nGroup; };
	virtual int          GetType(void) const { return NetMessageType::svc_ServerTick; };
	virtual const char* GetName(void) const { return "svc_ServerTick"; };
	virtual CNetChan* GetNetChannel(void) const { return m_NetChannel; };
	virtual const char* ToString(void) const
	{
		static char szBuf[4096];
		V_snprintf(szBuf, sizeof(szBuf), "%s: server tick %i", this->GetName(), m_NetTick.m_nServerTick);

		return szBuf;
	};
	virtual size_t       GetSize(void) const { return sizeof(SVC_ServerTick); };

	nettick_t m_NetTick;
};

struct SVC_VoiceData : public CNetMessage
{
public:
	SVC_VoiceData() = default;
	SVC_VoiceData(int senderClient, int nBytes, char* data)
	{
		void** pVFTable = reinterpret_cast<void**>(this);
		*pVFTable = g_pSVC_VoiceData_VFTable;

		m_bReliable = false;
		m_NetChannel = nullptr;

		m_nFromClient = senderClient;
		m_nLength = nBytes; // length in bits
		m_DataOut = data;

		m_nGroup = 2; // must be set to 2 to avoid being copied into replay buffer
	};

	virtual	~SVC_VoiceData() {};

	virtual void	SetNetChannel(CNetChan* netchan) { m_NetChannel = netchan; }
	virtual void	SetReliable(bool state) { m_bReliable = state; };

	virtual bool	Process(void)
	{
		return CallVFunc<bool>(NetMessageVtbl::Process, this);
	};
	virtual	bool	ReadFromBuffer(bf_read* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::ReadFromBuffer, this, buffer);
	}
	virtual	bool	WriteToBuffer(bf_write* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::WriteToBuffer, this, buffer);
	}

	virtual bool	IsReliable(void) const { return m_bReliable; };

	virtual int          GetGroup(void) const { return m_nGroup; };
	virtual int          GetType(void) const { return NetMessageType::svc_VoiceData; };
	virtual const char* GetName(void) const { return "svc_VoiceData"; };
	virtual CNetChan* GetNetChannel(void) const { return m_NetChannel; };
	virtual const char* ToString(void) const
	{
		static char szBuf[4096];
		V_snprintf(szBuf, sizeof(szBuf), "%s: client %i, bytes %i", this->GetName(), m_nFromClient, ((m_nLength + 7) >> 3));

		return szBuf;
	};
	virtual size_t       GetSize(void) const { return sizeof(SVC_VoiceData); };

	int m_nFromClient;
	int m_nLength;
	bf_read m_DataIn;
	void* m_DataOut;
};

struct SVC_DurangoVoiceData : public CNetMessage
{
public:
	SVC_DurangoVoiceData() = default;
	SVC_DurangoVoiceData(int senderClient, int nBytes, char* data, int unknown, bool useUnreliableStream)
	{
		void** pVFTable = reinterpret_cast<void**>(this);
		*pVFTable = g_pSVC_VoiceData_VFTable;

		m_bReliable = false;
		m_NetChannel = nullptr;

		m_nFromClient = senderClient;
		m_nLength = nBytes; // length in bits

		m_unknown = unknown;
		m_useVoiceStream = useUnreliableStream;

		m_DataOut = data;

		m_nGroup = 2; // must be set to 2 to avoid being copied into replay buffer
	};

	virtual	~SVC_DurangoVoiceData() {};

	virtual void	SetNetChannel(CNetChan* netchan) { m_NetChannel = netchan; }
	virtual void	SetReliable(bool state) { m_bReliable = state; };

	virtual bool	Process(void)
	{
		return CallVFunc<bool>(NetMessageVtbl::Process, this);
	};
	virtual	bool	ReadFromBuffer(bf_read* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::ReadFromBuffer, this, buffer);
	}
	virtual	bool	WriteToBuffer(bf_write* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::WriteToBuffer, this, buffer);
	}

	virtual bool	IsReliable(void) const { return m_bReliable; };

	virtual int          GetGroup(void) const { return m_nGroup; };
	virtual int          GetType(void) const { return NetMessageType::svc_DurangoVoiceData; };
	virtual const char* GetName(void) const { return "svc_DurangoVoiceData"; };
	virtual CNetChan* GetNetChannel(void) const { return m_NetChannel; };
	virtual const char* ToString(void) const
	{
		static char szBuf[4096];
		V_snprintf(szBuf, sizeof(szBuf), "%s: client %i, bytes %i", this->GetName(), m_nFromClient, ((m_nLength + 7) >> 3));

		return szBuf;
	};
	virtual size_t       GetSize(void) const { return sizeof(SVC_DurangoVoiceData); };

	int m_nFromClient;
	int m_nLength;
	int m_unknown;
	bool m_useVoiceStream;
	bf_read m_DataIn;
	void* m_DataOut;
};

class SVC_PlaylistOverrides : public CNetMessage
{
private:
	int			m_nMsgType;
	int			m_nLength;	// data length in bits
	bf_read		m_DataIn;
	bf_write	m_DataOut;
};

class SVC_SetClassVar : public CNetMessage
{
public:
	SVC_SetClassVar() = default;
	SVC_SetClassVar(const char* key, const char* value)
	{
		V_strncpy(m_szKey, key, sizeof(m_szKey));
		V_strncpy(m_szValue, value, sizeof(m_szValue));

		m_szKey[sizeof(m_szKey) - 1] = '\0';
		m_szValue[sizeof(m_szValue) - 1] = '\0';

		m_nGroup = 2; // must be set to 2 to avoid being copied into replay buffer
	}

	virtual bool	ReadFromBuffer(bf_read* buffer);
	virtual bool	WriteToBuffer(bf_write* buffer);

	virtual bool	Process(void);

	virtual int				GetType(void) const { return svc_SetClassVar; };
	virtual const char*		GetName(void) const { return "svc_SetClassVar"; };

	virtual const char*		ToString(void) const
	{
		static char szBuf[4096];
		V_snprintf(szBuf, sizeof(szBuf), "%s: key \"%s\", value \"%s\"", this->GetName(), m_szKey, m_szValue);

		return szBuf;
	};

	virtual size_t			GetSize(void) const { return sizeof(SVC_SetClassVar); }

	char m_szKey[128];
	char m_szValue[128];
};

///////////////////////////////////////////////////////////////////////////////////////
// Client messages:
///////////////////////////////////////////////////////////////////////////////////////

class CLC_VoiceData : public CNetMessage
{
public:
	int m_nLength;
	bf_read m_DataIn;
	bf_write m_DataOut;
	int unk1;
	int unk2;
};

class CLC_DurangoVoiceData : public CNetMessage
{
public:
	int m_nLength;
	bf_read m_DataIn;
	bf_write m_DataOut;
	bool m_skipXidCheck;
	bool m_useVoiceStream;
	int m_xid;
	int m_unknown;
};


class CLC_ClientTick : public CNetMessage
{
public:
	CLC_ClientTick() = default;
	CLC_ClientTick(int deltaTick, int stringTableTick)
	{
		void** pVFTable = reinterpret_cast<void**>(this);
		*pVFTable = g_pCLC_ClientTick_VFTable;

		m_bReliable = false;
		m_NetChannel = nullptr;

		m_nDeltaTick = deltaTick;
		m_nStringTableTick = stringTableTick;
	};

	virtual	~CLC_ClientTick() {};

	virtual void	SetNetChannel(CNetChan* netchan) { m_NetChannel = netchan; }
	virtual void	SetReliable(bool state) { m_bReliable = state; };

	virtual bool	Process(void)
	{
		return CallVFunc<bool>(NetMessageVtbl::Process, this);
	};
	virtual	bool	ReadFromBuffer(bf_read* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::ReadFromBuffer, this, buffer);
	}
	virtual	bool	WriteToBuffer(bf_write* buffer)
	{
		return CallVFunc<bool>(NetMessageVtbl::WriteToBuffer, this, buffer);
	}

	virtual bool	IsReliable(void) const { return m_bReliable; };

	virtual int          GetGroup(void) const { return m_nGroup; };
	virtual int          GetType(void) const { return NetMessageType::clc_ClientTick; };
	virtual const char* GetName(void) const { return "clc_ClientTick"; };
	virtual CNetChan* GetNetChannel(void) const { return m_NetChannel; };
	virtual const char* ToString(void) const
	{
		static char szBuf[4096];
		V_snprintf(szBuf, sizeof(szBuf), "%s: client tick %i", this->GetName(), m_nDeltaTick);

		return szBuf;
	};
	virtual size_t       GetSize(void) const { return sizeof(CLC_ClientTick); };

	int m_nDeltaTick;
	int m_nStringTableTick;
};

class CLC_SetPlaylistVarOverride : public CNetMessage
{
public:
	static	bool	ReadFromBufferImpl(CLC_SetPlaylistVarOverride* thisptr, bf_read* buffer);
	static	bool	WriteToBufferImpl(CLC_SetPlaylistVarOverride* thisptr, bf_write* buffer);

private:
	// !TODO:
};


///////////////////////////////////////////////////////////////////////////////////////
// Shared messages:
///////////////////////////////////////////////////////////////////////////////////////

struct NET_StringCmd : CNetMessage
{
	const char* cmd;
	char buffer[1024];
};

class NET_SetConVar : public CNetMessage
{
public:

	typedef struct cvar_s
	{
		char	name[MAX_OSPATH];
		char	value[MAX_OSPATH];
	} cvar_t;

	CUtlVector<cvar_t> m_ConVars;
};

///////////////////////////////////////////////////////////////////////////////////////
// This message is subclassed by 'SVC_CmdKeyValues' and 'CLC_CmdKeyValues'
class Base_CmdKeyValues : public CNetMessage
{
public:
	virtual	~Base_CmdKeyValues() {};

	virtual void	SetNetChannel(CNetChan* netchan) = 0;
	virtual void	SetReliable(bool state) = 0;

	virtual bool	Process(void) = 0;

	virtual	bool	ReadFromBuffer(bf_read* buffer) = 0;
	static	bool	ReadFromBufferImpl(Base_CmdKeyValues* thisptr, bf_read* buffer);

	virtual	bool	WriteToBuffer(bf_write* buffer) = 0;
	static	bool	WriteToBufferImpl(Base_CmdKeyValues* thisptr, bf_write* buffer);

	virtual bool	IsReliable(void) const = 0;

	virtual int          GetGroup(void) const = 0;
	virtual int          GetType(void) const = 0;
	virtual const char* GetName(void) const = 0;
	virtual CNetChan* GetNetChannel(void) const = 0;
	virtual const char* ToString(void) const = 0;
	virtual size_t       GetSize(void) const = 0;

	int			m_nMsgType;
	int			m_nLength;	// data length in bits
	bf_read		m_DataIn;
	bf_write	m_DataOut;
};

bool CanReplayMessage(const CNetMessage* msg);

///////////////////////////////////////////////////////////////////////////////
class V_NetMessages : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("SVC_Print::`vftable'", g_pSVC_Print_VFTable);
		LogConAdr("SVC_UserMessage::`vftable'", g_pSVC_UserMessage_VFTable);
		LogConAdr("SVC_ServerTick::`vftable'", g_pSVC_ServerTick_VFTable);
		LogConAdr("SVC_VoiceData::`vftable'", g_pSVC_VoiceData_VFTable);
		LogConAdr("SVC_PlaylistOverrides::`vftable'", g_pSVC_PlaylistOverrides_VFTable);
		LogConAdr("CLC_ClientTick::`vftable'", g_pCLC_ClientTick_VFTable);
		LogConAdr("CLC_SetPlaylistVarOverride::`vftable'", g_pCLC_SetPlaylistVarOverride_VFTable);
		LogConAdr("Base_CmdKeyValues::`vftable'", g_pBase_CmdKeyValues_VFTable);
		//LogFunAdr("MM_Heartbeat::ToString", MM_Heartbeat__ToString);
	}
	virtual void GetFun(void) const
	{
		//MM_Heartbeat__ToString = g_GameDll.FindPatternSIMD("48 83 EC 38 E8 ?? ?? ?? ?? 3B 05 ?? ?? ?? ??");
		// 48 83 EC 38 E8 ? ? ? ? 3B 05 ? ? ? ?
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const
	{
		// We get the actual address of the vftable here, not the class instance.
		g_pSVC_Print_VFTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_Print@@");
		g_pSVC_UserMessage_VFTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_UserMessage@@");
		g_pSVC_ServerTick_VFTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_ServerTick@@");
		g_pSVC_VoiceData_VFTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_VoiceData@@");
		g_pSVC_PlaylistOverrides_VFTable = g_GameDll.GetVirtualMethodTable(".?AVSVC_PlaylistOverrides@@");
		g_pCLC_ClientTick_VFTable = g_GameDll.GetVirtualMethodTable(".?AVCLC_ClientTick@@");
		g_pCLC_SetPlaylistVarOverride_VFTable = g_GameDll.GetVirtualMethodTable(".?AVCLC_SetPlaylistVarOverride@@");
		g_pBase_CmdKeyValues_VFTable = g_GameDll.GetVirtualMethodTable(".?AVBase_CmdKeyValues@@");
	}
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

