//===========================================================================//
// 
// Purpose: Hook interface
// 
//===========================================================================//
#include <cassert>
#include <vector>
#ifdef _DEBUG
#include <unordered_set>
#endif // _DEBUG
#include <Windows.h>
#include "../include/detours.h"
#include "../include/idetour.h"

//-----------------------------------------------------------------------------
// Contains a VFTable pointer, and the class instance. A VFTable can only be
// used by one class instance. This is to avoid duplicate registrations.
//-----------------------------------------------------------------------------
std::vector<IDetour*> g_DetourVec;
#ifdef _DEBUG
static std::unordered_set<const IDetour*> s_DetourSet;
#endif // _DEBUG

//-----------------------------------------------------------------------------
// Purpose: adds a detour context to the list
//-----------------------------------------------------------------------------
std::size_t AddDetour(IDetour* const pDetour)
{
#ifdef _DEBUG
	const IDetour* const pVFTable = reinterpret_cast<IDetour**>(pDetour)[0];
	const auto p = s_DetourSet.insert(pVFTable); // Track duplicate registrations.

	assert(p.second); // Code bug: duplicate registration!!! (called 'REGISTER(...)' from a header file?).
#endif // _DEBUG
	g_DetourVec.push_back(pDetour);
	return g_DetourVec.size();
}
