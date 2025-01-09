//=============================================================================//
// 
// Purpose: Memory allocation override functions
//-----------------------------------------------------------------------------
// The replacement functions use the game's internal memalloc system instead
// of the CRT one, this allows for sharing allocated memory directly between
// the SDK and the engine itself.
// 
//=============================================================================//
#include "memstd.h"
#include "mathlib/mathlib.h"

//-----------------------------------------------------------------------------
// Purpose: initialize the global memory allocator singleton pointer
//-----------------------------------------------------------------------------
static std::atomic_bool s_bAllocatorInitialized = false;
static void InitAllocator()
{
    if (s_bAllocatorInitialized.exchange(true))
        return;

    const QWORD imageBase = CModule::GetProcessEnvironmentBlock()->ImageBaseAddress;

    CreateGlobalMemAlloc = CModule::GetExportedSymbol(imageBase,
        "CreateGlobalMemAlloc").RCast<CStdMemAlloc * (*)(void)>();

    g_pMemAllocSingleton = CModule::GetExportedSymbol(imageBase,
        "g_pMemAllocSingleton").DerefSelf().RCast<CStdMemAlloc*>();
}

//-----------------------------------------------------------------------------
// Purpose: new/delete operator override
//-----------------------------------------------------------------------------
void* operator new(size_t const nSize) noexcept(false)
{
    return malloc(nSize);
}
void* __cdecl operator new(size_t const nSize,
    int const nBlockUse, const char* const pFileName, int const nLine)
{
    NOTE_UNUSED(nBlockUse);

    InitAllocator();
    return MemAllocSingleton()->InternalAlloc(nSize, pFileName, nLine);
}
void operator delete(void* const pBlock) throw()
{
    return free(pBlock);
}

//-----------------------------------------------------------------------------
extern "C"
{
    //-------------------------------------------------------------------------
    // Base overrides
    //-------------------------------------------------------------------------
    __declspec(restrict) void* __cdecl _malloc_base(size_t const nSize)
    {
        InitAllocator();
        return MemAllocSingleton()->Alloc(nSize);
    }
    //-------------------------------------------------------------------------
    __declspec(restrict) void* __cdecl _calloc_base(size_t const nCount,
        size_t const nSize)
    {
        InitAllocator();

        size_t const nTotal = nCount * nSize;
        void* const pNew = MemAllocSingleton()->Alloc(nTotal);

        memset(pNew, NULL, nTotal);
        return pNew;
    }
    //-------------------------------------------------------------------------
    __declspec(restrict) void* __cdecl _realloc_base(void* const pBlock,
        size_t const nSize)
    {
        InitAllocator();

        if (nSize)
            return MemAllocSingleton()->Realloc(pBlock, nSize);
        else
        {
            MemAllocSingleton()->InternalFree(pBlock, "tier0_static128", 0);
            return nullptr;
        }
    }
    //-------------------------------------------------------------------------
    __declspec(restrict) void* __cdecl _recalloc_base(void* const pBlock,
        size_t const nCount, size_t const nSize)
    {
        InitAllocator();

        const size_t nTotal = nCount * nSize;
        void* const pMemOut = MemAllocSingleton()->Realloc(pBlock, nTotal);

        return pMemOut;
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) void __cdecl _free_base(void* const pBlock)
    {
        InitAllocator();
#if !(defined(_DEBUG) && !defined(USE_MEM_DEBUG))
        MemAllocSingleton()->Free(pBlock);
#else
        MemAllocSingleton()->InternalFree(pBlock, "tier0_static128", 0);
#endif // !_DEBUG && !USE_MEM_DEBUG
    }
    //-------------------------------------------------------------------------
    void* __cdecl _expand_base(void* const pBlock, size_t const nNewSize, int const nBlockUse)
    {
        // Expanding isn't supported!!!
        Assert(0);
        return NULL;
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) size_t __cdecl _msize(void* const pBlock)
    {
        InitAllocator();
        return MemAllocSingleton()->GetSize(pBlock);
    }
    //-------------------------------------------------------------------------
    char* __cdecl _strdup(const char* const pString)
    {
        InitAllocator();

        const size_t nLen = strlen(pString) + 1;
        void* const pNew = MemAllocSingleton()->Alloc(nLen);

        if (!pNew)
            return nullptr;

        return reinterpret_cast<char*>(memcpy(pNew, pString, nLen));
    }

    //-------------------------------------------------------------------------
    // Align overrides
    //-------------------------------------------------------------------------
    ALLOC_CALL void* _aligned_malloc_base(size_t const nSize, size_t nAlign)
    {
        InitAllocator();
        unsigned char* pAlloc, * pResult;

        if (!IsPowerOfTwo(nAlign))
            return nullptr;

        nAlign = (nAlign > sizeof(void*) ? nAlign : sizeof(void*)) - 1;

        if ((pAlloc = (unsigned char*)MemAllocSingleton()->Alloc(
            sizeof(void*) + nAlign + nSize)) == (unsigned char*)nullptr)
            return nullptr;

        pResult = (unsigned char*)((size_t)(pAlloc + sizeof(void*) + nAlign) & ~nAlign);
        ((unsigned char**)(pResult))[-1] = pAlloc;

        return (void*)pResult;
    }
    //-------------------------------------------------------------------------
    ALLOC_CALL void* __cdecl _aligned_realloc_base(void* const pBlock,
        size_t const nSize, size_t const nAlign)
    {
        InitAllocator();

        if (!IsPowerOfTwo(nAlign))
            return nullptr;

        // Don't change alignment between allocation + reallocation.
        if (((size_t)pBlock & (nAlign - 1)) != 0)
            return nullptr;

        if (!pBlock)
            return _aligned_malloc_base(nSize, nAlign);

        // Figure out the actual allocation point
        void* pAlloc = pBlock;
        pAlloc = (void*)(((size_t)pAlloc & ~(sizeof(void*) - 1)) - sizeof(void*));
        pAlloc = *((void**)pAlloc);

        // See if we have enough space
        const size_t nOffset = (size_t)pBlock - (size_t)pAlloc;
        const size_t nOldSize = MemAllocSingleton()->GetSize(pAlloc);

        if (nOldSize >= nSize + nOffset)
            return pBlock;

        void* pResult = _aligned_malloc_base(nSize, nAlign);
        memcpy(pResult, pBlock, nOldSize - nOffset);

        MemAllocSingleton()->Free(pAlloc);
        return pResult;
    }
    //-------------------------------------------------------------------------
    ALLOC_CALL void* __cdecl _aligned_recalloc_base(void* const pBlock,
        size_t const nSize, size_t const nAlign)
    {
        NOTE_UNUSED(pBlock);
        NOTE_UNUSED(nSize);
        NOTE_UNUSED(nAlign);

        Assert(0); // Unsupported function.
        Error(eDLL_T::COMMON, EXIT_FAILURE, "Unsupported function\n");

        return NULL;
    }
    //-------------------------------------------------------------------------
    FREE_CALL void __cdecl _aligned_free_base(void* const pBlock)
    {
        InitAllocator();

        if (!pBlock)
            return;

        // pAlloc is the pointer to the start of memory block.
        void* pAlloc = pBlock;

        pAlloc = (void*)(((size_t)pAlloc & ~(sizeof(void*) - 1)) - sizeof(void*));
        pAlloc = *((void**)pAlloc);

        MemAllocSingleton()->Free(pAlloc);
    }
    // aligned ----------------------------------------------------------------
    ALLOC_CALL void* __cdecl _aligned_malloc(size_t const nSize, size_t const nAlign)
    {
        return _aligned_malloc_base(nSize, nAlign);
    }
    ALLOC_CALL void* __cdecl _aligned_realloc(void* const pBlock,
        size_t const nSize, size_t const nAlign)
    {
        return _aligned_realloc_base(pBlock, nSize, nAlign);
    }
    ALLOC_CALL void* __cdecl _aligned_recalloc(void* const pBlock,
        size_t const nCount, size_t const nSize, size_t const nAlign)
    {
        return _aligned_recalloc_base(pBlock, nCount * nSize, nAlign);
    }
    FREE_CALL void __cdecl _aligned_free(void* pBlock)
    {
        _aligned_free_base(pBlock);
    }
    // aligned offset base ----------------------------------------------------
    ALLOC_CALL void* __cdecl _aligned_offset_malloc_base(
        size_t const nSize, size_t const nAlign, size_t const nOffset)
    {
        Assert(IsPC() || 0);
        return NULL;
    }
    ALLOC_CALL void* __cdecl _aligned_offset_realloc_base(
        void* const pBlock, size_t const nSize, size_t const nAlign, size_t const nOffset)
    {
        Assert(IsPC() || 0);
        return NULL;
    }
    ALLOC_CALL void* __cdecl _aligned_offset_recalloc_base(
        void* const pBlock, size_t const nSize, size_t const nAlign, size_t const nOffset)
    {
        Assert(IsPC() || 0);
        return NULL;
    }
    // aligned offset ---------------------------------------------------------
    ALLOC_CALL void* __cdecl _aligned_offset_malloc(
        size_t const nSize, size_t const nAlign, size_t const nOffset)
    {
        return _aligned_offset_malloc_base(nSize, nAlign, nOffset);
    }
    ALLOC_CALL void* __cdecl _aligned_offset_realloc(
        void* const pBlock, size_t const nSize, size_t const nAlign, size_t const nOffset)
    {
        return _aligned_offset_realloc_base(pBlock, nSize, nAlign, nOffset);
    }
    ALLOC_CALL void* __cdecl _aligned_offset_recalloc(
        void* const pBlock, size_t const nCount, size_t const nSize, size_t const nAlign, size_t const nOffset)
    {
        return _aligned_offset_recalloc_base(pBlock, nCount * nSize, nAlign, nOffset);
    }

    //-------------------------------------------------------------------------
    // CRT overrides
    //-------------------------------------------------------------------------
    void* __cdecl _heap_alloc(size_t const nSize)
    {
        return _malloc_base(nSize);
    }
    void* __cdecl _malloc_crt(size_t const nSize)
    {
        return _malloc_base(nSize);
    }
    void* __cdecl _calloc_crt(size_t const nCount, size_t const nSize)
    {
        return _calloc_base(nCount, nSize);
    }
    void* __cdecl _realloc_crt(void* const pBlock, size_t const nSize)
    {
        return _realloc_base(pBlock, nSize);
    }
    void* __cdecl _recalloc_crt(void* const pBlock, size_t const nCount, size_t const nSize)
    {
        return _recalloc_base(pBlock, nSize, nCount);
    }

#if (defined(_DEBUG) || defined(USE_MEM_DEBUG))
    //-------------------------------------------------------------------------
    // Debug overrides
    //-------------------------------------------------------------------------
    __declspec(noinline) void* __cdecl _malloc_dbg(size_t const nSize,
        int const nBlockUse, const char* const pFileName, int const nLine)
    {
        NOTE_UNUSED(nBlockUse);
        InitAllocator();

        return MemAllocSingleton()->InternalAlloc(nSize, pFileName, nLine);
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) void* __cdecl _calloc_dbg(size_t const nCount,
        size_t const nSize, int const nBlockUse, const char* const pFileName,
        int const nLine)
    {
        NOTE_UNUSED(nBlockUse);
        InitAllocator();

        size_t const nTotal = nCount * nSize;
        void* const pNew = MemAllocSingleton()->InternalAlloc(nTotal, pFileName, nLine);

        memset(pNew, NULL, nTotal);
        return pNew;
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) void* __cdecl _realloc_dbg(void* const pBlock,
        size_t const nSize, int const nBlockUse, const char* const pFileName,
        int const nLine)
    {
        NOTE_UNUSED(nBlockUse);
        InitAllocator();

        if (nSize)
            return MemAllocSingleton()->Realloc(pBlock, nSize);
        else
        {
            MemAllocSingleton()->InternalFree(pBlock, pFileName, nLine);
            return nullptr;
        }
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) void* __cdecl _recalloc_dbg(void* const pBlock,
        size_t const nCount, size_t const nSize, int const nBlockUse,
        const char* const pFileName, int const nLine)
    {
        NOTE_UNUSED(nBlockUse);
        InitAllocator();

        const size_t nTotal = nCount * nSize;
        void* const pMemOut = MemAllocSingleton()->InternalRealloc(pBlock, nTotal,
            pFileName, nLine);

        if (!pBlock)
            memset(pMemOut, NULL, nTotal);

        return pMemOut;
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) void __cdecl _free_dbg(void* const pBlock,
        int const nBlockUse)
    {
        NOTE_UNUSED(nBlockUse);
        InitAllocator();

        MemAllocSingleton()->InternalFree(pBlock, "tier0_static128", 0);
    }
    //-------------------------------------------------------------------------
    void* __cdecl _expand_dbg(void* const pBlock, size_t const nNewSize, int const nBlockUse,
        const char* const pFileName, int const nLine)
    {
        NOTE_UNUSED(pFileName);
        NOTE_UNUSED(nLine);

        return _expand_base(pBlock, nNewSize, nBlockUse);
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) size_t __cdecl _msize_dbg(void* const pBlock, int const nBlockUse)
    {
        NOTE_UNUSED(nBlockUse);
        return _msize(pBlock);
    }
    //-------------------------------------------------------------------------
    void* __cdecl _heap_alloc_dbg(size_t const nSize, int const nBlockUse,
        const char* const szFileName, int const nLine)
    {
        return _malloc_dbg(nSize, nBlockUse, szFileName, nLine);
    }

    //-------------------------------------------------------------------------
    // Debug align
    //-------------------------------------------------------------------------
    void* __cdecl _aligned_malloc_dbg(size_t nSize, size_t nAlign,
        const char* const szFileName, int nLine)
    {
        NOTE_UNUSED(szFileName);
        NOTE_UNUSED(nLine);

        return _aligned_malloc(nSize, nAlign);
    }
    void* __cdecl _aligned_realloc_dbg(void* pBlock, size_t nSize, size_t nAlign,
        const char* const szFileName, int nLine)
    {
        NOTE_UNUSED(szFileName);
        NOTE_UNUSED(nLine);

        return _aligned_realloc(pBlock, nSize, nAlign);
    }
    void* __cdecl _aligned_offset_malloc_dbg(size_t const nSize, size_t const nAlign,
        size_t const nOffset, const char* const szFileName, int const nLine)
    {
        NOTE_UNUSED(szFileName);
        NOTE_UNUSED(nLine);

        return _aligned_offset_malloc(nSize, nAlign, nOffset);
    }
    void* __cdecl _aligned_offset_realloc_dbg(void* const pBlock, size_t const nSize,
        size_t const nAlign, size_t const offset, const char* const szFileName, int const nLine)
    {
        NOTE_UNUSED(szFileName);
        NOTE_UNUSED(nLine);

        return _aligned_offset_realloc(pBlock, nSize, nAlign, offset);
    }
    void __cdecl _aligned_free_dbg(void* const pBlock)
    {
        _aligned_free(pBlock);
    }

    //-------------------------------------------------------------------------
    // CRT debug nolocks
    //-------------------------------------------------------------------------
    void __cdecl _free_nolock(void* const pUserData)
    {
        // I don't think the second param is used in memoverride
        _free_dbg(pUserData, 0);
    }
    void __cdecl _free_dbg_nolock(void* const pUserData, int const nBlockUse)
    {
        _free_dbg(pUserData, nBlockUse);
    }

    //-------------------------------------------------------------------------
    // CRT debug stubs
    //-------------------------------------------------------------------------
    _CRT_ALLOC_HOOK __cdecl _CrtGetAllocHook(void)
    {
        Assert(0);
        return NULL;
    }
    int __cdecl CheckBytes(unsigned char* pb, unsigned char bCheck, size_t nSize)
    {
        const int bOkay = TRUE;
        return bOkay;
    }
    _CRT_DUMP_CLIENT __cdecl _CrtGetDumpClient(void)
    {
        Assert(0);
        return NULL;
    }
    void __cdecl _printMemBlockData(_locale_t plocinfo, _CrtMemBlockHeader* pHead)
    {
    }
    void __cdecl _CrtMemDumpAllObjectsSince_stat(const _CrtMemState* state, _locale_t plocinfo)
    {
        // Might need to be renamed to '_CrtMemDumpAllObjectsSince'
        // instead of '_CrtMemDumpAllObjectsSince_stat'.
    }
#endif // _DEBUG || USE_MEM_DEBUG
} // end extern "C"
