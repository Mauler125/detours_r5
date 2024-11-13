//=============================================================================//
//
// Purpose: AI system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/fasttimer.h"
#include "tier1/cvar.h"
#include "mathlib/bitvec.h"
#include "engine/server/server.h"
#include "public/edict.h"
#include "game/server/detour_impl.h"
#include "game/server/ai_networkmanager.h"
#include "game/shared/util_shared.h"

#include "vscript/languages/squirrel_re/vsquirrel.h"

#include "ai_basenpc.h"

static ConVar navmesh_always_reachable("navmesh_always_reachable", "0", FCVAR_DEVELOPMENTONLY, "Marks goal poly from agent poly as reachable by ignoring static pathing ( !slower! )");

//-----------------------------------------------------------------------------
// Purpose: gets the navmesh by type from global array [small, med_short, medium, large, extra_large]
// Input  : navMeshType - 
// Output : pointer to navmesh
//-----------------------------------------------------------------------------
dtNavMesh* Detour_GetNavMeshByType(const NavMeshType_e navMeshType)
{
    Assert(navMeshType >= NULL && navMeshType < NAVMESH_COUNT); // Programmer error.
    return g_pNavMesh[navMeshType];
}

//-----------------------------------------------------------------------------
// Purpose: free's the navmesh by type from global array [small, med_short, medium, large, extra_large]
// Input  : navMeshType - 
//-----------------------------------------------------------------------------
void Detour_FreeNavMeshByType(const NavMeshType_e navMeshType)
{
    Assert(navMeshType >= NULL && navMeshType < NAVMESH_COUNT); // Programmer error.
    dtNavMesh* const nav = g_pNavMesh[navMeshType];

    if (nav) // Only free if NavMesh for type is loaded.
    {
        // Frees tiles, polys, tris, anything dynamically
        // allocated for this navmesh, and the navmesh itself.
        delete nav;
        g_pNavMesh[navMeshType] = nullptr;
    }
}

//-----------------------------------------------------------------------------
// Purpose: determines whether goal poly is reachable from agent poly
//          (only checks static pathing)
// Input  : *nav - 
//			fromRef - 
//			goalRef - 
//			animType - 
// Output : value if reachable, false otherwise
//-----------------------------------------------------------------------------
bool Detour_IsGoalPolyReachable(dtNavMesh* const nav, const dtPolyRef fromRef, 
    const dtPolyRef goalRef, const TraverseAnimType_e animType)
{
    if (navmesh_always_reachable.GetBool())
        return true;

    const bool hasAnimType = animType != ANIMTYPE_NONE;
    const int traverseTableIndex = hasAnimType
        ? NavMesh_GetTraverseTableIndexForAnimType(animType)
        : NULL;

    return nav->isGoalPolyReachable(fromRef, goalRef, !hasAnimType, traverseTableIndex);
}

//-----------------------------------------------------------------------------
// Purpose: finds the nearest polygon to specified center point.
// Input  : *query - 
//          *center - 
//          *halfExtents - 
//          *filter - 
//          *nearestRef - 
//          *nearestPt - 
// Output: The status flags for the query.
//-----------------------------------------------------------------------------
dtStatus Detour_FindNearestPoly(dtNavMeshQuery* query, const float* center, const float* halfExtents,
    const dtQueryFilter* filter, dtPolyRef* nearestRef, float* nearestPt)
{
    return query->findNearestPoly(center, halfExtents, filter, nearestRef, nearestPt);
}

//-----------------------------------------------------------------------------
// Purpose: adds a tile to the NavMesh.
// Input  : *nav - 
//          *unused - 
//          *data - 
//          *dataSize - 
//          *flags - 
//          *lastRef - 
// Output: The status flags for the operation.
//-----------------------------------------------------------------------------
dtStatus Detour_AddTile(dtNavMesh* nav, void* unused, unsigned char* data, int dataSize, int flags, dtTileRef lastRef)
{
    return nav->addTile(data, dataSize, flags, lastRef, nullptr);
}

//-----------------------------------------------------------------------------
// Purpose: initialize NavMesh and Detour query singleton for level
//-----------------------------------------------------------------------------
void Detour_LevelInit()
{
    v_Detour_LevelInit();
    Detour_IsLoaded(); // Inform user which NavMesh files had failed to load.
}

//-----------------------------------------------------------------------------
// Purpose: free's the memory used by all valid NavMesh slots
//-----------------------------------------------------------------------------
void Detour_LevelShutdown()
{
    for (int i = 0; i < NAVMESH_COUNT; i++)
    {
        Detour_FreeNavMeshByType(NavMeshType_e(i));
    }
}

//-----------------------------------------------------------------------------
// Purpose: checks if the NavMesh has failed to load
// Output : true if a NavMesh has successfully loaded, false otherwise
//-----------------------------------------------------------------------------
bool Detour_IsLoaded()
{
    int ret = 0;
    for (int i = 0; i < NAVMESH_COUNT; i++)
    {
        const dtNavMesh* nav = Detour_GetNavMeshByType(NavMeshType_e(i));
        if (!nav) // Failed to load...
        {
            Warning(eDLL_T::SERVER, "NavMesh '%s%s_%s%s' not loaded\n", 
                NAVMESH_PATH, gpGlobals->mapName.ToCStr(),
                NavMesh_GetNameForType(NavMeshType_e(i)), NAVMESH_EXT);

            ret++;
        }
    }

    Assert(ret <= NAVMESH_COUNT);
    return (ret != NAVMESH_COUNT);
}

//-----------------------------------------------------------------------------
// Purpose: hot swaps the NavMesh with the current files on the disk
// (All types will be reloaded! If NavMesh for type no longer exist, it will be kept empty!!!)
//-----------------------------------------------------------------------------
void Detour_HotSwap()
{
    Assert(ThreadInMainOrServerFrameThread());
    g_pServerScript->ExecuteCodeCallback("CodeCallback_OnNavMeshHotSwapBegin");

    const dtNavMesh* const queryNav = g_pNavMeshQuery->getAttachedNavMesh();
    NavMeshType_e queryNavType = NAVMESH_INVALID;

    if (queryNav)
    {
        // Figure out which NavMesh type is attached to the global query.
        for (int i = 0; i < NAVMESH_COUNT; i++)
        {
            const NavMeshType_e in = (NavMeshType_e)i;

            if (queryNav != Detour_GetNavMeshByType(in))
                continue;

            queryNavType = in;
            break;
        }
    }

    // Free and re-init NavMesh.
    Detour_LevelShutdown();
    v_Detour_LevelInit();

    if (!Detour_IsLoaded())
        Error(eDLL_T::SERVER, NOERROR, "%s - Failed to hot swap NavMesh: %s\n", __FUNCTION__, 
            "one or more missing NavMesh types, Detour logic may be unavailable");

    // Attach the new NavMesh to the global Detour query.
    if (queryNavType != NAVMESH_INVALID)
    {
        const dtNavMesh* const newQueryNav = Detour_GetNavMeshByType(queryNavType);

        if (newQueryNav)
            g_pNavMeshQuery->attachNavMeshUnsafe(newQueryNav);
        else
            Error(eDLL_T::SERVER, NOERROR, "%s - Failed to hot swap NavMesh: %s\n", __FUNCTION__, 
                "previously attached NavMesh type is no longer available for global Detour query");
    }

    const int numAis = g_AI_Manager->NumAIs();
    CAI_BaseNPC** const pAis = g_AI_Manager->AccessAIs();

    // Reinitialize the AI's navmesh query to update the navmesh cache.
    for (int i = 0; i < numAis; i++)
    {
        CAI_BaseNPC* const npc = pAis[i];
        CAI_Pathfinder* const pathFinder = npc->GetPathfinder();

        const NavMeshType_e navType = NAI_Hull::NavMeshType(npc->GetHullType());
        const dtNavMesh* const navMesh = Detour_GetNavMeshByType(navType);

        if (dtStatusFailed(pathFinder->GetNavMeshQuery()->init(navMesh, 2048)))
            Error(eDLL_T::SERVER, NOERROR, "%s - Failed to initialize Detour NavMesh query for %s\n", __FUNCTION__, UTIL_GetEntityScriptInfo(npc));
    }

    g_pServerScript->ExecuteCodeCallback("CodeCallback_OnNavMeshHotSwapEnd");
}

/*
=====================
Detour_HotSwap_f

  Hot swaps the NavMesh
  while the game is running
=====================
*/
static void Detour_HotSwap_f()
{
    if (!g_pServer->IsActive())
        return; // Only execute if server is initialized and active.

    Msg(eDLL_T::SERVER, "Executing NavMesh hot swap for level '%s'\n",
        gpGlobals->mapName.ToCStr());

    CFastTimer timer;

    timer.Start();
    Detour_HotSwap();

    timer.End();
    Msg(eDLL_T::SERVER, "Hot swap took '%lf' seconds\n", timer.GetDuration().GetSeconds());
}

static ConCommand navmesh_hotswap("navmesh_hotswap", Detour_HotSwap_f, "Hot swap the NavMesh for all hulls", FCVAR_DEVELOPMENTONLY | FCVAR_SERVER_FRAME_THREAD);

///////////////////////////////////////////////////////////////////////////////
void VRecast::Detour(const bool bAttach) const
{
	DetourSetup(&v_Detour_IsGoalPolyReachable, &Detour_IsGoalPolyReachable, bAttach);
	DetourSetup(&v_Detour_LevelInit, &Detour_LevelInit, bAttach);
	//DetourSetup(&dtNavMesh__addTile, &Detour_AddTile, bAttach);
	//DetourSetup(&dtNavMeshQuery__findNearestPoly, &Detour_FindNearestPoly, bAttach);
}
