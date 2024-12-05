#pragma once
#include "public/edict.h"
#include "public/eiface.h"

class CVEngineServer : public IVEngineServer
{
public:
	static bool PersistenceAvailable(CVEngineServer* const thisptr, const int clientidx);
	// Implementation in GameDLL.
};
extern CVEngineServer* g_pEngineServer;
extern IVEngineServer* g_pEngineServerVFTable;

/* ==== CVENGINESERVER ================================================================================================================================================== */
inline bool(*CVEngineServer__PersistenceAvailable)(CVEngineServer* const thisptr, const int clientidx);

///////////////////////////////////////////////////////////////////////////////
inline bool* m_bIsDedicated = nullptr;

///////////////////////////////////////////////////////////////////////////////
class HVEngineServer : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CVEngineServer::`vftable'", g_pEngineServerVFTable);
		LogFunAdr("CVEngineServer::PersistenceAvailable", CVEngineServer__PersistenceAvailable);
		LogVarAdr("m_bIsDedicated", m_bIsDedicated); // !TODO: part of CServer!
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("3B 15 ?? ?? ?? ?? 7D 33").GetPtr(CVEngineServer__PersistenceAvailable);
	}
	virtual void GetVar(void) const
	{
		CMemory pEngineServerVFTable = g_GameDll.GetVirtualMethodTable(".?AVCVEngineServer@@", 0);

		g_pEngineServerVFTable = pEngineServerVFTable.RCast<CVEngineServer*>();
		m_bIsDedicated = pEngineServerVFTable.WalkVTableSelf(3).DerefSelf().ResolveRelativeAddress(0x3, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
