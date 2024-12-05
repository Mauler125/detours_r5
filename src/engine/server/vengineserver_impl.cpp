//=============================================================================//
//
// Purpose: Interface the engine exposes to the game DLL
//
//=============================================================================//

#include "vengineserver_impl.h"

//-----------------------------------------------------------------------------
// Purpose: sets the persistence var in the CClient instance to 'ready'
//-----------------------------------------------------------------------------
bool CVEngineServer::PersistenceAvailable(CVEngineServer* const thisptr, const int clientidx)
{
	///////////////////////////////////////////////////////////////////////////
	return CVEngineServer__PersistenceAvailable(thisptr, clientidx);
}

void HVEngineServer::Detour(const bool bAttach) const
{
	DetourSetup(&CVEngineServer__PersistenceAvailable, &CVEngineServer::PersistenceAvailable, bAttach);
}

IVEngineServer* g_pEngineServerVFTable = nullptr;
CVEngineServer* g_pEngineServer = reinterpret_cast<CVEngineServer*>(&g_pEngineServerVFTable);
