//===========================================================================//
//
// Purpose: RapidJSON allocator class
//
//===========================================================================//
#ifndef TIER2_JSONALLOC_H
#define TIER2_JSONALLOC_H

// 16 byte alignment as we only support up to 128 bits SIMD.
#define JSON_SIMD_ALIGNMENT 16

class JSONAllocator
{
public:
    static const bool kNeedFree;    //!< Whether this allocator needs to call Free().

    // Allocate a memory block.
    // \param size of the memory block in bytes.
    // \returns pointer to the memory block.
    void* Malloc(size_t size)
    {
        if (!size)
            return nullptr;

#ifdef RAPIDJSON_SIMD
        return _aligned_malloc(AlignValue(size, JSON_SIMD_ALIGNMENT), JSON_SIMD_ALIGNMENT);
#else
        return malloc(size);
#endif
    }

    // Resize a memory block.
    // \param originalPtr The pointer to current memory block. Null pointer is permitted.
    // \param originalSize The current size in bytes. (Design issue: since some allocator may not book-keep this, explicitly pass to it can save memory.)
    // \param newSize the new size in bytes.
    void* Realloc(void* originalPtr, size_t originalSize, size_t newSize)
    {
        (void)originalSize;

        if (newSize == 0)
        {
            Free(originalPtr);
            return nullptr;
        }

#ifdef RAPIDJSON_SIMD
        return _aligned_realloc(originalPtr, AlignValue(newSize, JSON_SIMD_ALIGNMENT), JSON_SIMD_ALIGNMENT);
#else
        return realloc(originalPtr, newSize);
#endif
    }

    // Free a memory block.
    // \param pointer to the memory block. Null pointer is permitted.
    static void Free(void* ptr) noexcept
    {
#ifdef RAPIDJSON_SIMD
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    bool operator==(const JSONAllocator&) const noexcept
    {
        return true;
    }
    bool operator!=(const JSONAllocator&) const noexcept
    {
        return false;
    }
};

#endif // TIER2_JSONALLOC_H
