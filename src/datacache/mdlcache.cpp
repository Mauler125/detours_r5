//=====================================================================================//
//
// model loading and caching
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "tier0/threadtools.h"
#include "tier1/cvar.h"
#include "tier1/utldict.h"
#include "datacache/mdlcache.h"
#include "datacache/imdlcache.h"
#include "datacache/idatacache.h"
#include "rtech/pak/paktools.h"
#include "public/studio.h"

CStudioFallbackHandler g_StudioMdlFallbackHandler;

//-----------------------------------------------------------------------------
// Purpose: finds an MDL
// Input  : *this - 
//          handle - 
//          *a3 - 
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::FindMDL(CMDLCache* const cache, const MDLHandle_t handle, void* a3)
{
    studiodata_t* const studioData = cache->GetStudioData(handle);

    if (!studioData)
    {
        studiohdr_t* const pStudioHdr = GetErrorModel();

        if (!IsKnownBadModel(handle))
        {
            if (!pStudioHdr)
                Error(eDLL_T::ENGINE, EXIT_FAILURE, "Model with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);
            else
                Error(eDLL_T::ENGINE, NO_ERROR, "Model with handle \"%hu\" not found; replacing with \"%s\".\n", handle, ERROR_MODEL);
        }

        return pStudioHdr;
    }

    studiomodelcache_t* modelCache = studioData->GetModelCache();

    // Store error and empty fallback models.
    if (IsValidDataCacheHandle(modelCache))
    {
        studiohdr_t* const studioHdr = studioData->GetModelCache()->GetStudioHdr();

        if (studioHdr)
        {
            // Typically, you would only check for '(m_nFlags & STUDIODATA_ERROR_MODEL)',
            // but for some reason this game doesn't have this flag set on that model.
            if (!HasErrorModel() && ((studioData->flags & STUDIODATA_ERROR_MODEL) 
                || V_ComparePath(studioHdr->name, ERROR_MODEL)))
            {
                g_StudioMdlFallbackHandler.SetFallbackModel(studioHdr, handle);
            }
        }
    }

    const int nFlags = STUDIOHDR_FLAGS_NEEDS_DEFERRED_ADDITIVE | STUDIOHDR_FLAGS_OBSOLETE;

    if ((studioData->flags & nFlags))
    {
        if (IsValidDataCacheHandle(modelCache))
        {
            if (a3)
            {
                FindCachedMDL(cache, studioData, a3);
                modelCache = studioData->GetModelCache();
            }

            studiohdr_t* const pStudioHdr = modelCache->GetStudioHdr();

            if (pStudioHdr)
                return pStudioHdr;

            return FindUncachedMDL(cache, handle, studioData, a3);
        }

        studioanimcache_t* const animCache = studioData->GetAnimCache();

        if (IsValidDataCacheHandle(animCache))
        {
            studiohdr_t* const pStudioHdr = animCache->GetStudioHdr();

            if (pStudioHdr)
                return pStudioHdr;
        }
    }
    return FindUncachedMDL(cache, handle, studioData, a3);
}

//-----------------------------------------------------------------------------
// Purpose: finds an MDL cached
// Input  : *this - 
//          *pStudioData - 
//          *a3 - 
//-----------------------------------------------------------------------------
void CMDLCache::FindCachedMDL(CMDLCache* const cache, studiodata_t* const pStudioData, void* a3)
{
    if (a3)
    {
        AUTO_LOCK(pStudioData->mutex);

        *(_QWORD*)((int64_t)a3 + 0x880) = *(_QWORD*)&pStudioData->pad[0x24];
        int64_t v6 = *(_QWORD*)&pStudioData->pad[0x24];
        if (v6)
            *(_QWORD*)(v6 + 0x878) = (int64_t)a3;
        *(_QWORD*)&pStudioData->pad[0x24] = (int64_t)a3;
        *(_QWORD*)((int64_t)a3 + 0x870) = (int64_t)cache;
        *(_WORD*)((int64_t)a3 + 0x888) = pStudioData->modelHandle;
    }
}

//-----------------------------------------------------------------------------
// Purpose: finds an MDL uncached
// Input  : *this - 
//          handle - 
//          *pStudioData - 
//          *a4 - 
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::FindUncachedMDL(CMDLCache* const cache, const MDLHandle_t handle, studiodata_t* pStudioData, void* a4)
{
    AUTO_LOCK(pStudioData->mutex);

    const char* modelName = cache->GetModelName(handle);
    const size_t fileNameLen = strlen(modelName);

    studiohdr_t* studioHdr = nullptr;

    if (fileNameLen < 5 ||
        (Q_stricmp(&modelName[fileNameLen - 5], ".rmdl") != 0) &&
        (Q_stricmp(&modelName[fileNameLen - 5], ".rrig") != 0) &&
        (Q_stricmp(&modelName[fileNameLen - 5], ".rpak") != 0))
    {
        studioHdr = GetErrorModel();
        if (!IsKnownBadModel(handle))
        {
            if (!studioHdr)
                Error(eDLL_T::ENGINE, EXIT_FAILURE, "Attempted to load old model \"%s\" and \"%s\" couldn't be loaded.\n", modelName, ERROR_MODEL);
            else
                Error(eDLL_T::ENGINE, NO_ERROR, "Attempted to load old model \"%s\"; replacing with \"%s\".\n", modelName, ERROR_MODEL);
        }

        return studioHdr;
    }

    pStudioData->processing = true;
    Pak_StringToGuid(modelName);
    pStudioData->processing = false;

    studiomodelcache_t* const modelCache = pStudioData->GetModelCache();

    if (IsValidDataCacheHandle(modelCache))
    {
        FindCachedMDL(cache, pStudioData, a4);
        studioHdr = modelCache->GetStudioHdr();
    }
    else
    {
        // Attempt to get studio header from anim cache.
        studioanimcache_t* const animCache = pStudioData->GetAnimCache();

        if (IsValidDataCacheHandle(animCache))
        {
            studioHdr = animCache->GetStudioHdr();
        }
        else
        {
            studioHdr = GetErrorModel();

            if (!IsKnownBadModel(handle))
            {
                if (!studioHdr)
                    Error(eDLL_T::ENGINE, EXIT_FAILURE, "Model \"%s\" not found and \"%s\" couldn't be loaded.\n", modelName, ERROR_MODEL);
                else
                    Error(eDLL_T::ENGINE, NO_ERROR, "Model \"%s\" not found; replacing with \"%s\".\n", modelName, ERROR_MODEL);
            }
        }
    }

    assert(studioHdr);
    return studioHdr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the model cache by handle
// Input  : handle - 
// Output : a pointer to the studiomodelcache_t object
//-----------------------------------------------------------------------------
studiomodelcache_t* CMDLCache::GetModelCache(const MDLHandle_t handle)
{
    if (handle == MDLHANDLE_INVALID)
        return nullptr;

    const studiodata_t* const studioData = GetStudioData(handle);

    if (!studioData)
        return nullptr;

    studiomodelcache_t* const modelCache = studioData->GetModelCache();
    return modelCache;
}

//-----------------------------------------------------------------------------
// Purpose: gets the vcollide data from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the vcollide_t object
//-----------------------------------------------------------------------------
vcollide_t* CMDLCache::GetVCollide(CMDLCache* const cache, const MDLHandle_t handle)
{
    studiomodelcache_t* const modelCache = cache->GetModelCache(handle);

    if (!IsValidDataCacheHandle(modelCache))
    {
        Warning(eDLL_T::ENGINE, "Attempted to load collision data on model \"%s\" with invalid studio data!\n", cache->GetModelName(handle));
        return nullptr;
    }

    studiophysicsref_t* const physicsCache = modelCache->GetPhysicsCache();

    if (!physicsCache)
        return nullptr;

    CStudioVCollide* const pVCollide = physicsCache->GetStudioVCollide();

    if (!pVCollide)
        return nullptr;

    return pVCollide->GetVCollide();
}

//-----------------------------------------------------------------------------
// Purpose: gets the physics geometry data from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the physics geometry descriptor
//-----------------------------------------------------------------------------
void* CMDLCache::GetPhysicsGeometry(CMDLCache* const cache, const MDLHandle_t handle)
{
    studiomodelcache_t* const modelCache = cache->GetModelCache(handle);

    if (!IsValidDataCacheHandle(modelCache))
    {
        Warning(eDLL_T::ENGINE, "Attempted to load physics geometry on model \"%s\" with invalid studio data!\n", cache->GetModelName(handle));
        return nullptr;
    }

    studiophysicsref_t* const physicsRef = modelCache->GetPhysicsCache();

    if (!physicsRef)
        return nullptr;

    CStudioPhysicsGeoms* const physicsGeoms = physicsRef->GetPhysicsGeoms();

    if (!physicsGeoms)
        return nullptr;

    return physicsGeoms->GetGeometryData();
}

//-----------------------------------------------------------------------------
// Purpose: gets the studio hardware data from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the studiohwdata_t object
//-----------------------------------------------------------------------------
studiohwdata_t* CMDLCache::GetHardwareData(CMDLCache* const cache, const MDLHandle_t handle)
{
    const studiodata_t* studioData = nullptr; cache->GetStudioData(handle);
    const studiomodelcache_t* modelCache = cache->GetModelCache(handle);

    if (!IsValidDataCacheHandle(modelCache))
    {
        if (!HasErrorModel())
        {
            Error(eDLL_T::ENGINE, NO_ERROR, "Studio hardware for model \"%s\" not found and \"%s\" couldn't be loaded!\n",
                cache->GetModelName(handle), ERROR_MODEL);

            assert(0); // Should never be hit!
            return nullptr;
        }
        else
        {
            // Only spew the message once.
            if (g_StudioMdlFallbackHandler.AddToSuppressionList(handle))
            {
                Warning(eDLL_T::ENGINE, "Studio hardware for model \"%s\" not found; replacing with \"%s\".\n",
                    cache->GetModelName(handle), ERROR_MODEL);
            }
        }

        studioData = cache->GetStudioData(GetErrorModelHandle());
        modelCache = studioData->GetModelCache();
    }
    else
    {
        studioData = cache->GetStudioData(handle);
    }

    studiophysicsref_t* const physicsRef = modelCache->GetPhysicsCache();

    AcquireSRWLockExclusive(g_pMDLLock);
    CMDLCache__CheckData(physicsRef, 1i64); // !!! DECLARED INLINE IN < S3 !!!
    ReleaseSRWLockExclusive(g_pMDLLock);

    if ((studioData->flags & STUDIODATA_FLAGS_STUDIOMESH_LOADED))
        return studioData->GetHardwareDataRef()->GetHardwareData();

    return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the error model
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::GetErrorModel(void)
{
    return g_StudioMdlFallbackHandler.GetFallbackModelHeader();
}
const char* CMDLCache::GetErrorModelName(void)
{
    const studiohdr_t* const errorStudioHdr = g_StudioMdlFallbackHandler.GetFallbackModelHeader();
    assert(errorStudioHdr);

    return errorStudioHdr ? errorStudioHdr->name : "(invalid)";
}
MDLHandle_t CMDLCache::GetErrorModelHandle(void)
{
    return g_StudioMdlFallbackHandler.GetFallbackModelHandle();
}
bool CMDLCache::HasErrorModel(void)
{
    return g_StudioMdlFallbackHandler.HasFallbackModel();
}

//-----------------------------------------------------------------------------
// Purpose: checks if this model handle is within the set of bad models
// Input  : handle - 
// Output : true if exist, false otherwise
//-----------------------------------------------------------------------------
bool CMDLCache::IsKnownBadModel(const MDLHandle_t handle)
{
    // Only adds if it didn't exist yet, else it returns false.
    return g_StudioMdlFallbackHandler.AddBadModelHandle(handle);
}

void VMDLCache::Detour(const bool bAttach) const
{
    DetourSetup(&CMDLCache__FindMDL, &CMDLCache::FindMDL, bAttach);
    DetourSetup(&CMDLCache__FindCachedMDL, &CMDLCache::FindCachedMDL, bAttach);
    DetourSetup(&CMDLCache__FindUncachedMDL, &CMDLCache::FindUncachedMDL, bAttach);

    DetourSetup(&CMDLCache__GetVCollide, &CMDLCache::GetVCollide, bAttach);
    DetourSetup(&CMDLCache__GetPhysicsGeometry, &CMDLCache::GetPhysicsGeometry, bAttach);

    DetourSetup(&CMDLCache__GetHardwareData, &CMDLCache::GetHardwareData, bAttach);
}
