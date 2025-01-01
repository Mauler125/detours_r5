//=============================================================================//
//
// Purpose: pak page allocation and alignment
//
//=============================================================================//
#include "rtech/ipakfile.h"
#include "pakstate.h"
#include "pakalloc.h"

//-----------------------------------------------------------------------------
// aligns the slab headers for each asset type
//-----------------------------------------------------------------------------
void Pak_AlignSlabHeaders(PakFile_s* const pak, PakSlabDescriptor_s* const desc)
{
    uint64_t headersSize = 0;
    uint32_t slabHeaderAlignment = desc->slabAlignmentForType[SF_HEAD];

    for (uint8_t i = 0; i < PAK_MAX_TRACKED_TYPES; ++i)
    {
        const PakAssetBinding_s& binding = g_pakGlobals->assetBindings[i];

        if (desc->assetTypeCount[i])
        {
            assert(binding.headerAlignment > 0 && IsPowerOfTwo(binding.headerAlignment));
            const size_t alignedSize = ALIGN_VALUE(headersSize, static_cast<size_t>(binding.headerAlignment));

            pak->memoryData.unkAssetTypeBindingSizes[i] = alignedSize;
            headersSize = alignedSize + (desc->assetTypeCount[i] * binding.nativeClassSize);

            desc->slabSizeForType[SF_HEAD] = headersSize;

            slabHeaderAlignment = Max(slabHeaderAlignment, binding.headerAlignment);
            desc->slabAlignmentForType[SF_HEAD] = slabHeaderAlignment;
        }
    }
}

//-----------------------------------------------------------------------------
// aligns each individual non-header segment
//-----------------------------------------------------------------------------
void Pak_AlignSlabData(PakFile_s* const pak, PakSlabDescriptor_s* const desc)
{
    for (uint16_t i = 0; i < pak->GetSlabCount(); ++i)
    {
        const PakSlabHeader_s* const slabHeader = pak->GetSlabHeader(i);
        const uint8_t slabType = slabHeader->typeFlags & (SF_CPU | SF_TEMP);

        if (slabType != SF_HEAD) // if not a header slab
        {
            // should this be a hard error on release?
            // slab alignment must not be 0 and must be a power of two
            assert(slabHeader->dataAlignment > 0 && IsPowerOfTwo(slabHeader->dataAlignment));
            const size_t alignedSlabSize = ALIGN_VALUE(desc->slabSizeForType[slabType], static_cast<size_t>(slabHeader->dataAlignment));

            desc->slabSizes[i] = alignedSlabSize;
            desc->slabSizeForType[slabType] = alignedSlabSize + slabHeader->dataSize;

            // check if this slab's alignment is higher than the previous highest for this type
            // if so, increase the alignment to accommodate this slab
            desc->slabAlignmentForType[slabType] = Max(desc->slabAlignmentForType[slabType], slabHeader->dataAlignment);
        }
    }
}

//-----------------------------------------------------------------------------
// copy's pages into pre-allocated and aligned slabs
//-----------------------------------------------------------------------------
void Pak_CopyPagesToSlabs(PakFile_s* const pak, PakLoadedInfo_s* const loadedInfo, PakSlabDescriptor_s* const desc)
{
    for (uint32_t i = 0; i < pak->GetPageCount(); ++i)
    {
        const PakPageHeader_s* const pageHeader = pak->GetPageHeader(i);
        const uint32_t slabIndex = pageHeader->slabIndex;

        const PakSlabHeader_s* const slabHeader = pak->GetSlabHeader(slabIndex);
        const int typeFlags = slabHeader->typeFlags;

        // check if header page
        if ((typeFlags & (SF_CPU | SF_TEMP)) != 0)
        {
            // align the slab's current size to the alignment of the new page to get copied in
            // this ensures that the location holding the page is aligned as required
            // 
            // since the slab will always have alignment equal to or greater than the page, and that alignment will always be a power of 2
            // the page does not have to be aligned to the same alignment as the slab, as aligning it to its own alignment is sufficient as long as
            // every subsequent page does the same thing
            const size_t alignedSlabSize = ALIGN_VALUE(desc->slabSizes[slabIndex], static_cast<size_t>(pageHeader->pageAlignment));

            // get a pointer to the newly aligned location within the slab for this page
            pak->memoryData.memPageBuffers[i] = reinterpret_cast<uint8_t*>(loadedInfo->slabBuffers[typeFlags & (SF_CPU | SF_TEMP)]) + alignedSlabSize;

            // update the slab size to reflect the new alignment and page size
            desc->slabSizes[slabIndex] = alignedSlabSize + pak->memoryData.pageHeaders[i].dataSize;
        }
        else
        {
            // all headers go into one slab and are dealt with separately in Pak_ProcessPakFile
            // since headers must be copied individually into a buffer that is big enough for the "native class" version of the header
            // instead of just the file version
            pak->memoryData.memPageBuffers[i] = reinterpret_cast<uint8_t*>(loadedInfo->slabBuffers[SF_HEAD]);
        }
    }
}
