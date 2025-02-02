#include "cliententitylist.h"

// Note: 'entList' points directly at the vtable member
// of CClientEntityList; sub classed data is truncated.
static IClientNetworkable* ClientEntityList_GetClientNetworkable(IClientEntityList* const entList, const int entNum)
{
	// entNum is used to index into m_EntPtrArray, which is of size
	// NUM_ENT_ENTRIES. However, both the lower and upper bounds
	// checks were missing; check it here.
	if (entNum < 0 || entNum >= NUM_ENT_ENTRIES)
	{
		Assert(0);
		return nullptr;
	}

	return v_ClientEntityList_GetClientNetworkable(entList, entNum);
}

// Note: 'entList' points directly at the vtable member
// of CClientEntityList; sub classed data is truncated.
static IClientEntity* ClientEntityList_GetClientEntity(IClientEntityList* const entList, const int entNum)
{
	// Numbers < -2 will be used to index into the array as follows:
	// m_EntPtrArray[ (MAX_EDICTS-2) - entNum ]. However, the code
	// doesn't have a clamp for underflows; check it here. -1 cases
	// are ignored here as they already are handled correctly.
	if (entNum < -(MAX_EDICTS - 2))
	{
		Assert(0);
		return nullptr;
	}

	// m_EntPtrArray is as large as NUM_ENT_ENTRIES, but there is no
	// overflow clamp; check it here.
	if (entNum >= NUM_ENT_ENTRIES)
	{
		Assert(0);
		return nullptr;
	}

	return v_ClientEntityList_GetClientEntity(entList, entNum);
}

void VClientEntityList::Detour(const bool bAttach) const
{
	DetourSetup(&v_ClientEntityList_GetClientNetworkable, &ClientEntityList_GetClientNetworkable, bAttach);
	DetourSetup(&v_ClientEntityList_GetClientEntity, &ClientEntityList_GetClientEntity, bAttach);
}

//-----------------------------------------------------------------------------
// Purpose: a global list of all the entities in the game. All iteration through
//          entities is done through this object.
//-----------------------------------------------------------------------------
CClientEntityList* g_clientEntityList = nullptr;
