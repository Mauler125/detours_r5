//=============================================================================//
//
// Purpose: pak file loading and unloading
//
//=============================================================================//
#include "rtech/ipakfile.h"
#include "rtech/async/asyncio.h"

#include "paktools.h"
#include "pakstate.h"
#include "pakpatch.h"
#include "pakalloc.h"
#include "pakparse.h"
#include "pakdecode.h"
#include "pakstream.h"

static ConVar pak_debugrelations("pak_debugrelations", "0", FCVAR_DEVELOPMENTONLY | FCVAR_ACCESSIBLE_FROM_THREADS, "Debug RPAK asset dependency resolving");
static ConVar pak_debugchannel("pak_debugchannel", "4", FCVAR_DEVELOPMENTONLY | FCVAR_ACCESSIBLE_FROM_THREADS, "Log RPAK files loaded or unloaded with this channel ID", false, 0.f, false, 0.f, "0 = disabled, -1 = all");

//-----------------------------------------------------------------------------
// resolve the target guid from lookup table
//-----------------------------------------------------------------------------
static bool Pak_ResolveAssetDependency(const PakFile_s* const pak, PakGuid_t currentGuid,
    const PakGuid_t targetGuid, int& currentIndex, const bool shouldCheckTwo)
{
    while (true)
    {
        if (shouldCheckTwo && currentGuid == 2)
        {
            if (pak->memoryData.pakHeader.assetCount)
                return false;
        }

        currentIndex++;

        if (currentIndex >= PAK_MAX_LOADED_ASSETS)
            return false;

        currentIndex &= PAK_MAX_LOADED_ASSETS_MASK;
        currentGuid = g_pakGlobals->loadedAssets[currentIndex].guid;

        if (currentGuid == targetGuid)
            return true;
    }

    UNREACHABLE();
}

//-----------------------------------------------------------------------------
// resolve guid relations for asset
//-----------------------------------------------------------------------------
void Pak_ResolveAssetRelations(PakFile_s* const pak, const PakAsset_s* const asset)
{
    PakPage_u* const pGuidDescriptors = &pak->memoryData.guidDescriptors[asset->dependenciesIndex];
    uint32_t* const v5 = (uint32_t*)g_pakGlobals->loadedPaks[pak->memoryData.pakId & PAK_MAX_LOADED_PAKS_MASK].qword50;

    if (pak_debugrelations.GetBool())
        Msg(eDLL_T::RTECH, "Resolving relations for asset: '0x%-16llX', dependencies: %-4u; in pak '%s'\n",
            asset->guid, asset->dependenciesCount, pak->memoryData.fileName);

    for (uint32_t i = 0; i < asset->dependenciesCount; i++)
    {
        void** const pCurrentGuid = reinterpret_cast<void**>(pak->memoryData.memPageBuffers[pGuidDescriptors[i].index] + pGuidDescriptors[i].offset);

        // get current guid
        const PakGuid_t targetGuid = reinterpret_cast<uint64_t>(*pCurrentGuid);

        // get asset index
        int currentIndex = targetGuid & PAK_MAX_LOADED_ASSETS_MASK;
        const PakGuid_t currentGuid = g_pakGlobals->loadedAssets[currentIndex].guid;

        const int64_t v9 = 2i64 * InterlockedExchangeAdd(v5, 1u);
        *reinterpret_cast<PakGuid_t*>(const_cast<uint32_t*>(&v5[2 * v9 + 2])) = targetGuid;
        *reinterpret_cast<PakGuid_t*>(const_cast<uint32_t*>(&v5[2 * v9 + 4])) = asset->guid;

        if (currentGuid != targetGuid)
        {
            // are we some special asset with the guid 2?
            if (!Pak_ResolveAssetDependency(pak, currentGuid, targetGuid, currentIndex, true))
            {
                PakAsset_s* assetEntries = pak->memoryData.assetEntries;
                uint64_t a = 0;

                for (; assetEntries->guid != targetGuid; a++, assetEntries++)
                {
                    if (a >= pak->memoryData.pakHeader.assetCount)
                    {
                        if (!Pak_ResolveAssetDependency(pak, currentGuid, targetGuid, currentIndex, false))
                        {
                            // the dependency couldn't be resolved, this state is irrecoverable;
                            // error out
                            Error(eDLL_T::RTECH, EXIT_FAILURE, "Failed to resolve asset dependency %u of %u\n"
                                "pak: '%s'\n"
                                "asset: '0x%llX'\n"
                                "target: '0x%llX'\n",
                                i, asset->dependenciesCount,
                                pak->memoryData.fileName,
                                asset->guid,
                                targetGuid);
                        }

                        break;
                    }
                }

                currentIndex = pak->memoryData.loadedAssetIndices[a];
            }
        }

        // finally write the pointer to the guid entry
        *pCurrentGuid = g_pakGlobals->loadedAssets[currentIndex].head;
    }
}

uint32_t Pak_ProcessRemainingPagePointers(PakFile_s* const pak)
{
    uint32_t processedPointers = 0;

    for (processedPointers = pak->numProcessedPointers; processedPointers < pak->GetPointerCount(); ++processedPointers)
    {
        PakPage_u* const curPage = &pak->memoryData.virtualPointers[processedPointers];
        int curCount = curPage->index - pak->firstPageIdx;

        if (curCount < 0)
            curCount += pak->memoryData.pakHeader.memPageCount;

        if (curCount >= pak->processedPageCount)
            break;

        PakPage_u* const ptr = reinterpret_cast<PakPage_u*>(pak->GetPointerForPageOffset(curPage));
        ptr->ptr = pak->memoryData.memPageBuffers[ptr->index] + ptr->offset;
    }

    return processedPointers;
}

void Pak_RunAssetLoadingJobs(PakFile_s* const pak)
{
    pak->numProcessedPointers = Pak_ProcessRemainingPagePointers(pak);

    const uint32_t numAssets = pak->processedAssetCount;

    if (numAssets == pak->memoryData.pakHeader.assetCount)
        return;

    PakAsset_s* pakAsset = &pak->memoryData.assetEntries[numAssets];

    if (pakAsset->pageEnd > pak->processedPageCount)
        return;

    for (uint32_t currentAsset = numAssets; g_pakGlobals->numAssetLoadJobs <= 0xC8u;)
    {
        pak->memoryData.loadedAssetIndices[currentAsset] = Pak_TrackAsset(pak, pakAsset);

        _InterlockedIncrement16(&g_pakGlobals->numAssetLoadJobs);

        const uint8_t assetBind = pakAsset->HashTableIndexForAssetType();

        if (g_pakGlobals->assetBindings[assetBind].loadAssetFunc)
        {
            const JobTypeID_t jobTypeId = g_pakGlobals->assetBindJobTypes[assetBind];

            // have to cast it to a bigger size to send it as param to JTGuts_AddJob().
            const int64_t pakId = pak->memoryData.pakId;

            JTGuts_AddJob(jobTypeId, pak->memoryData.assetLoadJobId, (void*)pakId, (void*)(uint64_t)currentAsset);
        }
        else
        {
            if (_InterlockedExchangeAdd16((volatile signed __int16*)&pakAsset->numRemainingDependencies, 0xFFFFu) == 1)
                Pak_ProcessAssetRelationsAndResolveDependencies(pak, pakAsset, currentAsset, assetBind);

            _InterlockedDecrement16(&g_pakGlobals->numAssetLoadJobs);
        }

        currentAsset = ++pak->processedAssetCount;

        if (currentAsset == pak->memoryData.pakHeader.assetCount)
        {
            JT_EndJobGroup(pak->memoryData.assetLoadJobId);
            return;
        }

        pakAsset = &pak->memoryData.assetEntries[currentAsset];

        if (pakAsset->pageEnd > pak->processedPageCount)
            return;
    }
}

//-----------------------------------------------------------------------------
// load user-requested pak files on-demand
//-----------------------------------------------------------------------------
PakHandle_t Pak_LoadAsync(const char* const fileName, CAlignedMemAlloc* const allocator, const int logChannel, const bool bUnk)
{
    if (!Pak_FileExists(fileName))
    {
        Error(eDLL_T::RTECH, NO_ERROR, "%s: Failed; file '%s' doesn't exist\n", __FUNCTION__, fileName);
        return PAK_INVALID_HANDLE;
    }

    // NOTE: errors are always logged, regardless of the selected channel ID!
    const int selectedLogChannel = pak_debugchannel.GetInt();

    if (selectedLogChannel && (selectedLogChannel == -1 || logChannel == selectedLogChannel))
        Msg(eDLL_T::RTECH, "Loading pak file: '%s'\n", fileName);

    const PakHandle_t pakId = v_Pak_LoadAsync(fileName, allocator, logChannel, bUnk);

    if (pakId == PAK_INVALID_HANDLE)
        Error(eDLL_T::RTECH, NO_ERROR, "%s: Failed read '%s' results '%d'\n", __FUNCTION__, fileName, pakId);

    return pakId;
}

//-----------------------------------------------------------------------------
// unloads loaded pak files
//-----------------------------------------------------------------------------
void Pak_UnloadAsync(PakHandle_t handle)
{
    const PakLoadedInfo_s* const pakInfo = Pak_GetPakInfo(handle);

    if (pakInfo->fileName)
    {
        const int selectedLogChannel = pak_debugchannel.GetInt();

        if (selectedLogChannel && (selectedLogChannel == -1 || pakInfo->logChannel == selectedLogChannel))
            Msg(eDLL_T::RTECH, "Unloading pak file: '%s'\n", pakInfo->fileName);
    }

    v_Pak_UnloadAsync(handle);
}

#define CMD_INVALID -1

// only patch cmds 4,5,6 use this array to determine their data size
static const int s_patchCmdToBytesToProcess[] = { CMD_INVALID, CMD_INVALID, CMD_INVALID, CMD_INVALID, 3, 7, 6, 0 };
#undef CMD_INVALID
//----------------------------------------------------------------------------------
// loads and processes a pak file (handles decompression and patching)
// TODO: !!! FINISH REBUILD !!!
//----------------------------------------------------------------------------------
bool Pak_ProcessPakFile(PakFile_s* const pak)
{
    PakFileStream_s* const fileStream = &pak->fileStream;
    PakMemoryData_s* const memoryData = &pak->memoryData;

    // first request is always just the header?
    size_t readStart = sizeof(PakFileHeader_s);

    if (fileStream->numDataChunks > 0)
        readStart = fileStream->numDataChunks * PAK_READ_DATA_CHUNK_SIZE;

    for (; fileStream->numDataChunksProcessed != fileStream->numDataChunks; fileStream->numDataChunksProcessed++)
    {
        const int v7 = fileStream->numDataChunksProcessed & 0x1F;
        const uint8_t v8 = fileStream->gap94[v7];
        if (v8 != 1)
        {
            size_t bytesProcessed = 0;
            const char* statusMsg = "(no reason)";
            const uint8_t currentStatus = g_pakLoadApi->CheckAsyncRequest(fileStream->asyncRequestHandles[v7], &bytesProcessed, &statusMsg);

            if (currentStatus == AsyncHandleStatus_t::FS_ASYNC_ERROR)
                Error(eDLL_T::RTECH, EXIT_FAILURE, "Error reading pak file \"%s\" -- %s\n", pak->memoryData.fileName, statusMsg);
            else if (currentStatus == AsyncHandleStatus_t::FS_ASYNC_PENDING)
                break;

            fileStream->bytesStreamed += bytesProcessed;
            if (v8)
            {
                const PakFileHeader_s* pakHeader = &pak->memoryData.pakHeader;
                const uint64_t v16 = fileStream->numDataChunksProcessed * PAK_READ_DATA_CHUNK_SIZE;

                if (v8 == 2)
                {
                    fileStream->bytesStreamed = bytesProcessed + v16;
                    pakHeader = (PakFileHeader_s*)&fileStream->buffer[v16 & fileStream->bufferMask];
                }

                const uint8_t fileIndex = fileStream->numLoadedFiles++ & PAK_MAX_ASYNC_STREAMED_LOAD_REQUESTS_MASK;

                //printf("v16: %lld\n", v16);
                fileStream->descriptors[fileIndex].dataOffset = v16 + sizeof(PakFileHeader_s);
                fileStream->descriptors[fileIndex].compressedSize = v16 + pakHeader->compressedSize;
                fileStream->descriptors[fileIndex].decompressedSize = pakHeader->decompressedSize;
                fileStream->descriptors[fileIndex].compressionMode = pakHeader->GetCompressionMode();
            }
        }
    }

    size_t qword1D0 = memoryData->processedPatchedDataSize;

    for (; pak->byte1F8 != fileStream->numLoadedFiles; pak->byte1F8++)
    {
        PakFileStream_s::Descriptor* v22 = &fileStream->descriptors[pak->byte1F8 & PAK_MAX_ASYNC_STREAMED_LOAD_REQUESTS_MASK];

        if (pak->byte1FD)
        {
            pak->byte1FD = false;
            pak->inputBytePos = v22->dataOffset;

            if (v22->compressionMode != PakDecodeMode_e::MODE_DISABLED)
            {
                pak->isOffsetted_MAYBE = false;
                pak->isCompressed = true;
                memoryData->processedPatchedDataSize = sizeof(PakFileHeader_s);
            }
            else
            {
                pak->isOffsetted_MAYBE = true;
                pak->isCompressed = false;
                //printf("v22->dataOffset: %lld\n", v22->dataOffset);
                memoryData->processedPatchedDataSize = v22->dataOffset;
            }

            if (pak->isCompressed)
            {
                const size_t decompressedSize = Pak_InitDecoder(&pak->pakDecoder,
                    fileStream->buffer, pak->decompBuffer,
                    PAK_DECODE_IN_RING_BUFFER_MASK, PAK_DECODE_OUT_RING_BUFFER_MASK,
                    v22->compressedSize - (v22->dataOffset - sizeof(PakFileHeader_s)),
                    v22->dataOffset - sizeof(PakFileHeader_s), sizeof(PakFileHeader_s), v22->compressionMode);

                if (decompressedSize != v22->decompressedSize)
                    Error(eDLL_T::RTECH, EXIT_FAILURE,
                        "Error reading pak file \"%s\" with decoder \"%s\" -- decompressed size %zu doesn't match expected value %zu\n",
                        pak->memoryData.fileName,
                        Pak_DecoderToString(v22->compressionMode),
                        decompressedSize,
                        pak->memoryData.pakHeader.decompressedSize);
            }
        }

        if (pak->isCompressed)
        {
            qword1D0 = pak->pakDecoder.outBufBytePos;

            if (qword1D0 != pak->pakDecoder.decompSize)
            {
                const bool didDecode = Pak_StreamToBufferDecode(&pak->pakDecoder, 
                    fileStream->bytesStreamed, (memoryData->processedPatchedDataSize + PAK_DECODE_OUT_RING_BUFFER_SIZE), v22->compressionMode);

                qword1D0 = pak->pakDecoder.outBufBytePos;
                pak->inputBytePos = pak->pakDecoder.inBufBytePos;

                if (didDecode)
                    DevMsg(eDLL_T::RTECH, "%s: pak '%s' decoded successfully\n", __FUNCTION__, pak->GetName());
            }
        }
        else
        {
            qword1D0 = Min(v22->compressedSize, fileStream->bytesStreamed);
        }

        if (pak->inputBytePos != v22->compressedSize || memoryData->processedPatchedDataSize != qword1D0)
            break;

        pak->byte1FD = true;
        qword1D0 = memoryData->processedPatchedDataSize;
    }

    size_t numBytesToProcess = qword1D0 - memoryData->processedPatchedDataSize;

    while (memoryData->patchSrcSize + memoryData->field_2A8)
    {
        // if there are no bytes left to process in this patch operation
        if (!memoryData->numBytesToProcess_maybe)
        {
            RBitRead& bitbuf = memoryData->bitBuf;
            bitbuf.ConsumeData(memoryData->patchData, bitbuf.BitsAvailable());

            // advance patch data buffer by the number of bytes that have just been fetched
            memoryData->patchData = &memoryData->patchData[bitbuf.BitsAvailable() >> 3];

            // store the number of bits remaining to complete the data read
            bitbuf.m_bitsAvailable = bitbuf.BitsAvailable() & 7; // number of bits above a whole byte

            const __int8 cmd = memoryData->patchCommands[bitbuf.ReadBits(6)];

            bitbuf.DiscardBits(memoryData->PATCH_field_68[bitbuf.ReadBits(6)]);

            // get the next patch function to execute
            memoryData->patchFunc = g_pakPatchApi[cmd];

            if (cmd <= 3u)
            {
                const uint8_t bitExponent = memoryData->PATCH_unk2[bitbuf.ReadBits(8)]; // number of stored bits for the data size

                bitbuf.DiscardBits(memoryData->PATCH_unk3[bitbuf.ReadBits(8)]);

                memoryData->numBytesToProcess_maybe = (1ull << bitExponent) + bitbuf.ReadBits(bitExponent);

                bitbuf.DiscardBits(bitExponent);
            }
            else
            {
                memoryData->numBytesToProcess_maybe = s_patchCmdToBytesToProcess[cmd];
            }
        }

        if (!pak->memoryData.patchFunc(pak, &numBytesToProcess))
            break;
    }

    if (pak->isOffsetted_MAYBE)
        pak->inputBytePos = memoryData->processedPatchedDataSize;

    if (!fileStream->finishedLoadingPatches)
    {
        const size_t v42 = min(fileStream->numDataChunksProcessed, pak->inputBytePos >> 19);

        //if ((unsigned int)(pak->inputBytePos >> 19) < v42)
        //    v42 = pak->inputBytePos >> 19;

        while (fileStream->numDataChunks != v42 + 32)
        {
            const int8_t requestIdx = fileStream->numDataChunks & 0x1F;
            const size_t readOffsetEnd = (fileStream->numDataChunks + 1ull) * PAK_READ_DATA_CHUNK_SIZE;

            if (fileStream->fileReadStatus == 1)
            {
                fileStream->asyncRequestHandles[requestIdx] = FS_ASYNC_REQ_INVALID;
                fileStream->gap94[requestIdx] = 1;

                if (((requestIdx + 1) & PAK_MAX_ASYNC_STREAMED_LOAD_REQUESTS_MASK) == 0)
                    fileStream->fileReadStatus = 2;

                ++fileStream->numDataChunks;
                readStart = readOffsetEnd;
            }
            else
            {
                if (readStart < fileStream->fileSize)
                {
                    const size_t lenToRead = Min(fileStream->fileSize, readOffsetEnd);

                    const size_t readOffset = readStart - fileStream->qword0;
                    const size_t readSize = lenToRead - readStart;

                    fileStream->asyncRequestHandles[requestIdx] = v_FS_ReadAsyncFile(
                        fileStream->fileHandle,
                        readOffset,
                        readSize,
                        &fileStream->buffer[readStart & fileStream->bufferMask],
                        0,
                        0,
                        4);

                    fileStream->gap94[requestIdx] = fileStream->fileReadStatus;
                    fileStream->fileReadStatus = 0;

                    ++fileStream->numDataChunks;
                    readStart = readOffsetEnd;
                }
                else
                {
                    if (pak->patchCount >= pak->memoryData.pakHeader.patchIndex)
                    {
                        FS_CloseAsyncFile(fileStream->fileHandle);
                        fileStream->fileHandle = PAK_INVALID_HANDLE;
                        fileStream->qword0 = 0;
                        fileStream->finishedLoadingPatches = true;

                        return memoryData->patchSrcSize == 0;
                    }

                    if (!pak->dword14)
                        return memoryData->patchSrcSize == 0;

                    char pakPatchPath[MAX_PATH] = {};
                    sprintf(pakPatchPath, PAK_PLATFORM_PATH"%s", pak->memoryData.fileName);

                    // get path of next patch rpak to load
                    if (pak->memoryData.patchIndices[pak->patchCount])
                    {
                        char* pExtension = nullptr;

                        char* it = pakPatchPath;
                        while (*it)
                        {
                            if (*it == '.')
                                pExtension = it;
                            else if (*it == '\\' || *it == '/')
                                pExtension = nullptr;

                            ++it;
                        }

                        if (pExtension)
                            it = pExtension;

                        // replace extension '.rpak' with '(xx).rpak'
                        snprintf(it, &pakPatchPath[sizeof(pakPatchPath)] - it,
                            "(%02u).rpak", pak->memoryData.patchIndices[pak->patchCount]);
                    }

                    const int patchFileHandle = FS_OpenAsyncFile(pakPatchPath, 5, &numBytesToProcess);

                    //printf("[%s] Opened pak '%s' with file handle %i\n", pak->GetName(), pakPatchPath, patchFileHandle);

                    if (patchFileHandle == FS_ASYNC_FILE_INVALID)
                        Error(eDLL_T::RTECH, EXIT_FAILURE, "Couldn't open file \"%s\".\n", pakPatchPath);

                    if (numBytesToProcess < pak->memoryData.patchHeaders[pak->patchCount].compressedSize)
                        Error(eDLL_T::RTECH, EXIT_FAILURE, "File \"%s\" appears truncated; read size: %zu < expected size: %zu.\n",
                            pakPatchPath, numBytesToProcess, pak->memoryData.patchHeaders[pak->patchCount].compressedSize);

                    FS_CloseAsyncFile(fileStream->fileHandle);

                    fileStream->fileHandle = patchFileHandle;

                    const size_t v58 = ALIGN_VALUE(fileStream->numDataChunks, 8ull) * PAK_READ_DATA_CHUNK_SIZE;
                    fileStream->fileReadStatus = (fileStream->numDataChunks == ALIGN_VALUE(fileStream->numDataChunks, 8ull)) + 1;

                    //printf("[%s] dwordB8: %i, v58: %lld, byteBC: %i, numFiles: %i\n", pak->GetName(), fileStream->numDataChunks, v58, fileStream->byteBC, fileStream->numLoadedFiles);
                    fileStream->qword0 = v58;
                    fileStream->fileSize = v58 + pak->memoryData.patchHeaders[pak->patchCount].compressedSize;

                    pak->patchCount++;
                }
            }
        }
    }

    return memoryData->patchSrcSize == 0;
}

// sets patch variables for copying the next unprocessed page into the relevant segment buffer
// if this is a header page, fetch info from the next unprocessed asset and copy over the asset's header
bool SetupNextPageForPatching(PakLoadedInfo_s* a1, PakFile_s* pak)
{
    Pak_RunAssetLoadingJobs(pak);

    // numProcessedPointers has just been set in the above function call
    pak->memoryData.numShiftedPointers = pak->numProcessedPointers;

    if (pak->processedPageCount == pak->GetPageCount())
        return false;
    //break;

    const uint32_t highestProcessedPageIdx = pak->processedPageCount + pak->firstPageIdx;
    pak->processedPageCount++;

    int v26 = highestProcessedPageIdx - pak->GetPageCount();
    if (highestProcessedPageIdx < pak->GetPageCount())
        v26 = highestProcessedPageIdx;

    const PakPageHeader_s* const nextMemPageHeader = &pak->memoryData.pageHeaders[v26];
    if ((pak->memoryData.segmentHeaders[nextMemPageHeader->segmentIdx].typeFlags & (SF_TEMP | SF_CPU)) != 0)
    {
        pak->memoryData.patchSrcSize = nextMemPageHeader->dataSize;
        pak->memoryData.patchDstPtr = reinterpret_cast<char*>(pak->memoryData.memPageBuffers[v26]);

        return true;
        //continue;
    }

    // headers
    PakAsset_s* pakAsset = pak->memoryData.ppAssetEntries[pak->memoryData.someAssetCount];

    pak->memoryData.patchSrcSize = pakAsset->headerSize;
    int assetTypeIdx = pakAsset->HashTableIndexForAssetType();

    pak->memoryData.patchDstPtr = reinterpret_cast<char*>(a1->segmentBuffers[0]) + pak->memoryData.unkAssetTypeBindingSizes[assetTypeIdx];
    pak->memoryData.unkAssetTypeBindingSizes[assetTypeIdx] += g_pakGlobals->assetBindings[assetTypeIdx].nativeClassSize;

    return true;
}

bool Pak_ProcessAssets(PakLoadedInfo_s* const a1)
{
    PakFile_s* const pak = a1->pakFile;
    while (pak->processedAssetCount != pak->GetAssetCount())
    {
        // TODO: invert condition and make the branch encompass the whole loop
        if (!(pak->memoryData.patchSrcSize + pak->memoryData.field_2A8))
        {
            const bool res = SetupNextPageForPatching(a1, pak);

            if (res)
                continue;
            else
                break;
        }

        if (!Pak_ProcessPakFile(pak))
            return false;

        // processedPageCount must be greater than 0 here otherwise the page index will be negative and cause a crash
        // if this happens, something probably went wrong with the patch data condition at the start of the loop, as that
        // function call should increment processedPageCount if it succeeded
        assert(pak->processedPageCount > 0);

        const uint32_t pageCount = pak->GetPageCount();
        const uint32_t v4 = (pak->firstPageIdx - 1) + pak->processedPageCount;

        uint32_t shiftedPageIndex = v4;

        if (v4 >= pageCount)
            shiftedPageIndex -= pageCount;

        // if "temp_" segment
        if ((pak->memoryData.segmentHeaders[pak->memoryData.pageHeaders[shiftedPageIndex].segmentIdx].typeFlags & (SF_TEMP | SF_CPU)) != 0)
        {
            const bool res = SetupNextPageForPatching(a1, pak);

            if (res)
                continue;
            else
                break;
        }

        PakAsset_s* asset = pak->memoryData.ppAssetEntries[pak->memoryData.someAssetCount];
        const uint32_t headPageOffset = asset->headPtr.offset;
        char* v8 = pak->memoryData.patchDstPtr - asset->headerSize;

        uint32_t newOffsetFromSegmentBufferToHeader = LODWORD(pak->memoryData.patchDstPtr)
            - asset->headerSize
            - LODWORD(a1->segmentBuffers[0]);
        asset->headPtr.offset = newOffsetFromSegmentBufferToHeader;

        uint32_t offsetSize = newOffsetFromSegmentBufferToHeader - headPageOffset;

        for (uint32_t i = pak->memoryData.numShiftedPointers; i < pak->GetPointerCount(); pak->memoryData.numShiftedPointers = i)
        {
            PakPage_u* ptr = &pak->memoryData.virtualPointers[i];

            ASSERT_PAKPTR_VALID(pak, ptr);

            if (ptr->index != shiftedPageIndex)
                break;

            const uint32_t offsetToPointer = ptr->offset - headPageOffset;
            if (offsetToPointer >= asset->headerSize)
                break;

            PakPage_u* pagePtr = reinterpret_cast<PakPage_u*>(v8 + offsetToPointer);

            ASSERT_PAKPTR_VALID(pak, ptr);

            ptr->offset += offsetSize;

            if (pagePtr->index == shiftedPageIndex)
                pagePtr->offset += offsetSize;

            i = pak->memoryData.numShiftedPointers + 1;
        }

        for (uint32_t j = 0; j < asset->dependenciesCount; ++j)
        {
            PakPage_u* descriptor = &pak->memoryData.guidDescriptors[asset->dependenciesIndex + j];

            if (descriptor->index == shiftedPageIndex)
                descriptor->offset += offsetSize;
        }

        const uint32_t v16 = ++pak->memoryData.someAssetCount;

        PakAsset_s* v17 = nullptr;
        if (v16 < pak->GetAssetCount() && (v17 = pak->memoryData.ppAssetEntries[v16], v17->headPtr.index == shiftedPageIndex))
        {
            pak->memoryData.field_2A8 = v17->headPtr.offset - headPageOffset - asset->headerSize;
            pak->memoryData.patchSrcSize = v17->headerSize;
            const uint8_t assetTypeIdx = v17->HashTableIndexForAssetType();

            pak->memoryData.patchDstPtr = reinterpret_cast<char*>(a1->segmentBuffers[0]) + pak->memoryData.unkAssetTypeBindingSizes[assetTypeIdx];

            pak->memoryData.unkAssetTypeBindingSizes[assetTypeIdx] += g_pakGlobals->assetBindings[assetTypeIdx].nativeClassSize;
        }
        else
        {
            bool res = SetupNextPageForPatching(a1, pak);

            if (res)
                continue;
            else
                break;
        }
    }

    if (!JT_IsJobDone(pak->memoryData.assetLoadJobId))
        return false;

    uint32_t i = 0;
    PakAsset_s* pAsset = nullptr;

    for (int j = pak->memoryData.pakId & PAK_MAX_LOADED_PAKS_MASK; i < pak->GetHeader().assetCount; a1->assetGuids[i - 1] = pAsset->guid)
    {
        pAsset = &pak->memoryData.assetEntries[i];
        if (pAsset->numRemainingDependencies)
        {
            //printf("[%s] processing deps for %llX (%.4s)\n", pak->GetName(), pAsset->guid, (char*)&pAsset->magic);
            Pak_ResolveAssetRelations(pak, pAsset);

            const int assetIndex = pak->memoryData.loadedAssetIndices[i];
            const PakAssetShort_s& loadedAsset = g_pakGlobals->loadedAssets[assetIndex];

            if (g_pakGlobals->trackedAssets[loadedAsset.trackerIndex].loadedPakIndex == j)
            {
                PakTracker_s* pakTracker = g_pakGlobals->pakTracker;

                if (pakTracker)
                {
                    if (pakTracker->numPaksTracked)
                    {
                        int* trackerIndices = g_pakGlobals->pakTracker->loadedAssetIndices;
                        uint32_t count = 0;

                        while (*trackerIndices != assetIndex)
                        {
                            ++count;
                            ++trackerIndices;

                            if (count >= pakTracker->numPaksTracked)
                                goto LABEL_41;
                        }

                        goto LABEL_42;
                    }
                }
                else
                {
                   pakTracker = reinterpret_cast<PakTracker_s*>(AlignedMemAlloc()->Alloc(sizeof(PakTracker_s), 8));
                   pakTracker->numPaksTracked = 0;
                   pakTracker->unk_4 = 0;
                   pakTracker->unk_8 = 0;

                   g_pakGlobals->pakTracker = pakTracker;
                }
            LABEL_41:

                pakTracker->loadedAssetIndices[pakTracker->numPaksTracked] = assetIndex;
                ++pakTracker->numPaksTracked;
            }
        }
    LABEL_42:
        ++i;
    }

    if (g_pakGlobals->pakTracker)
        sub_14043D870(a1, 0);

    a1->status = PakStatus_e::PAK_STATUS_LOADED;

    return true;
}

void Pak_StubInvalidAssetBinds(PakFile_s* const pak, PakSegmentDescriptor_s* const desc)
{
    for (uint32_t i = 0; i < pak->GetAssetCount(); ++i)
    {
        PakAsset_s* const asset = &pak->memoryData.assetEntries[i];
        pak->memoryData.ppAssetEntries[i] = asset;

        const uint8_t assetTypeIndex = asset->HashTableIndexForAssetType();
        desc->assetTypeCount[assetTypeIndex]++;

        PakAssetBinding_s* const assetBinding = &g_pakGlobals->assetBindings[assetTypeIndex];

        if (assetBinding->type == PakAssetBinding_s::NONE)
        {
            assetBinding->extension = asset->magic;
            assetBinding->version = asset->version;
            assetBinding->description = "<unknown>";
            assetBinding->loadAssetFunc = nullptr;
            assetBinding->unloadAssetFunc = nullptr;
            assetBinding->replaceAssetFunc = nullptr;
            assetBinding->allocator = AlignedMemAlloc();
            assetBinding->headerSize = asset->headerSize;
            assetBinding->nativeClassSize = asset->headerSize;
            assetBinding->headerAlignment = pak->memoryData.pageHeaders[asset->headPtr.index].pageAlignment;
            assetBinding->type = PakAssetBinding_s::STUB;
        }

        // this is dev only because it could spam a lot on older paks
        // which isn't much help to the average user that can't rebuild other people's paks
        if (asset->version != assetBinding->version)
        {
            FourCCString_t assetMagic;
            FourCCToString(assetMagic, asset->magic);

            DevWarning(eDLL_T::RTECH,
                "Unexpected asset version for \"%s\" (%.4s) asset with guid 0x%llX (asset %u in pakfile '%s'). Expected %u, found %u.\n",
                assetBinding->description,
                assetMagic,
                asset->guid,
                i, pak->GetName(),
                assetBinding->version, asset->version
            );
        }
    }
}

bool Pak_StartLoadingPak(PakLoadedInfo_s* const loadedInfo)
{
    PakFile_s* const pakFile = loadedInfo->pakFile;

    if (pakFile->memoryData.patchSrcSize && !Pak_ProcessPakFile(pakFile))
        return false;

    PakSegmentDescriptor_s pakDescriptor = {};

    Pak_StubInvalidAssetBinds(pakFile, &pakDescriptor);

    const uint32_t numAssets = pakFile->GetAssetCount();

    if (pakFile->memoryData.pakHeader.patchIndex)
        pakFile->firstPageIdx = pakFile->memoryData.patchDataHeader->pageCount;

    sub_140442740(pakFile->memoryData.ppAssetEntries, &pakFile->memoryData.ppAssetEntries[numAssets], numAssets, pakFile);

    // pak must have no more than PAK_MAX_SEGMENTS segments as otherwise we will overrun the above "segmentSizes" array
    // and write to arbitrary locations on the stack
    if (pakFile->GetSegmentCount() > PAK_MAX_SEGMENTS)
    {
        Error(eDLL_T::RTECH, EXIT_FAILURE, "Too many segments in pakfile '%s'. Max %i, found %i.\n", pakFile->GetName(), PAK_MAX_SEGMENTS, pakFile->GetSegmentCount());
        return false;
    }

    Pak_AlignSegmentHeaders(pakFile, &pakDescriptor);
    Pak_AlignSegments(pakFile, &pakDescriptor);

    // allocate segment buffers with predetermined alignments; pages will be
    // copied into here
    for (int8_t i = 0; i < PAK_SEGMENT_BUFFER_TYPES; ++i)
    {
        if (pakDescriptor.segmentSizeForType[i])
            loadedInfo->segmentBuffers[i] = AlignedMemAlloc()->Alloc(pakDescriptor.segmentSizeForType[i], pakDescriptor.segmentAlignmentForType[i]);
    }

    Pak_CopyPagesToSegments(pakFile, loadedInfo, &pakDescriptor);

    const PakFileHeader_s& pakHdr = pakFile->GetHeader();

    if (Pak_StreamingEnabled())
        Pak_LoadStreamingData(loadedInfo);

    const __int64 v106 = pakHdr.descriptorCount + 2 * (pakHdr.patchIndex + pakHdr.assetCount + 4ull * pakHdr.assetCount + pakHdr.virtualSegmentCount);
    const __int64 patchDestOffset = pakHdr.GetTotalHeaderSize() + 2 * (pakHdr.patchIndex + 6ull * pakHdr.memPageCount + 4 * v106);

    pakFile->dword14 = 1;

    PakMemoryData_s& memoryData = pakFile->memoryData;

    memoryData.patchSrcSize = pakFile->memoryData.qword2D0 - patchDestOffset;
    memoryData.patchDstPtr = (char*)&pakHdr + patchDestOffset;

    loadedInfo->status = PakStatus_e::PAK_STATUS_LOAD_PAKHDR;

    return true;
}

// #STR: "(%02u).rpak", "Couldn't read package file \"%s\".\n", "Error 0x%08x", "paks\\Win64\\%s"
//bool Pak_SetupBuffersAndLoad(PakHandle_t pakId)
//{
//    __int64 v2; // rdi
//    const char* pakFilePath; // rsi
//    unsigned int v7; // ebx
//    unsigned int v8; // r14d
//    char** v9; // r12
//    int v10; // eax
//    __int64 v11; // r11
//    char v12; // al
//    char* i; // rdx
//    uint32_t pakFileHandle; // eax
//    bool result; // al
//    int AsyncFile; // bl
//    signed __int64 v19; // rbx
//    char v20; // r14
//    __int64 v21; // rdx
//    const char* v22; // rax
//    __int64 v23; // rbx
//    __int64 asset_entry_count_var; // rax MAPDST
//    __int64 patchIndex; // r11
//    __int64 memPageCount; // r9
//    __int64 virtualSegmentCount; // rcx
//    __int64 v32; // r8
//    __int64 v33; // rcx
//    __int64 v34; // r14
//    __int64 v35; // r12
//    uint64_t ringBufferStreamSize; // rsi
//    uint64_t ringBufferOutSize; // rax
//    PakFile_t* pak; // rax MAPDST
//    _DWORD* v40; // rax
//    __int64 v43; // rdx
//    uint8_t** v44; // rcx
//    PakAsset_t** v45; // rdx
//    PakPatchFileHeader_t* p_patchHeader; // rcx
//    uint16_t v47; // ax
//    PakPatchFileHeader_t* p_decompressedSize; // rdx
//    __int64 v49; // rcx
//    __int64 v50; // rax
//    unsigned __int16* v51; // rcx
//    char* v52; // rcx
//    char* v53; // rdx
//    __int64 v54; // rax
//    PakSegmentHeader_t* v55; // rdx
//    __int64 v56; // rcx
//    __int64 v57; // rax
//    PakPageHeader_t* v58; // rcx
//    __int64 descriptorCount; // rdx
//    PakPage_t* v60; // rcx
//    __int64 assetCount; // rax
//    PakAsset_t* v62; // r8
//    __int64 guidDescriptorCount; // r9
//    PakPage_t* v64; // rdx
//    __int64 relationsCounts; // rcx
//    uint32_t* v66; // rax
//    __int64 v67; // rdx
//    uint32_t* v68; // rcx
//    __int64 v69; // r8
//    uint32_t* v70; // rax
//    __int64 v71; // rcx
//    PakPatchDataHeader_t* patchDataHeader; // rax
//    size_t v73; // rdx
//    uint8_t* ringBuffer; // rax
//    bool v75; // zf
//    __int64 v76; // r10
//    unsigned __int64 v77; // r8
//    __int64 v78; // rax
//    PakFileHeader_t pakHdr; // [rsp+40h] [rbp-C0h] BYREF
//    __int64 v80; // [rsp+C0h] [rbp-40h]
//    __int64 v81; // [rsp+C8h] [rbp-38h]
//    char relativeFilePath[260]; // [rsp+E0h] [rbp-20h] BYREF
//    char v84[12]; // [rsp+1E4h] [rbp+E4h] BYREF
//    CHAR LibFileName[4]; // [rsp+1F0h] [rbp+F0h] BYREF
//    char v86[8252]; // [rsp+1F4h] [rbp+F4h]
//    size_t pak_file_size_var; // [rsp+2248h] [rbp+2148h] BYREF
//    __int64 v89; // [rsp+2250h] [rbp+2150h]
//    uint64_t v90; // [rsp+2258h] [rbp+2158h]
//
//    v2 = 0i64;
//
//    PakLoadedInfo_t* const loadedInfo = Pak_GetPakInfo(pakId);
//
//    loadedInfo->status = PAK_STATUS_LOAD_STARTING;
//    pakFilePath = loadedInfo->fileName;
//
//    //
//    assert(pakFilePath);
//
//    const char* nameUnqualified = V_UnqualifiedFileName(pakFilePath);
//
//    if (nameUnqualified != pakFilePath)
//        goto LABEL_27;
//
//    snprintf(relativeFilePath, 0x100ui64, "paks\\Win64\\%s", pakFilePath);
//
//    v7 = g_nPatchEntryCount;
//    v8 = 0;
//
//    // if this pak is patched, load the last patch file first before proceeding
//    // with any other pak that is getting patched. note that the patch number
//    // does not indicate which pak file is the actual last patch file; a patch
//    // with number (01) can also patch (02) and base, these details are
//    // determined from the pak file header
//    if (g_nPatchEntryCount)
//    {
//        v9 = g_pszPakPatchList;
//
//        int n;
//        while (1)
//        {
//            n = V_stricmp(pakFilePath, v9[(v7 + v8) >> 1]);
//
//            if (n >= 0)
//                break;
//
//            v7 = v11;
//        LABEL_13:
//            if (v8 >= v7)
//                goto SKIP_PATCH_LOADING;
//        }
//
//        if (n > 0)
//        {
//            v8 = v11 + 1;
//            goto LABEL_13;
//        }
//
//        const int patchNumber = g_pnPatchNumbers[v11];
//
//        if (patchNumber)
//        {
//            char* const extension = (char*)V_GetFileExtension(pakFilePath, true);
//            snprintf(extension, &relativeFilePath[sizeof(relativeFilePath)] - extension, "(%02u).rpak", patchNumber);
//        }
//    }
//
//    if (!g_nPatchEntryCount)
//        goto SKIP_PATCH_LOADING;
//
//    v9 = g_pszPakPatchList;
//    while (1)
//    {
//        LOBYTE(v10) = stricmp(pakFilePath, v9[(v7 + v8) >> 1]);
//        if (v10 >= 0)
//            break;
//        v7 = v11;
//    LABEL_13:
//        if (v8 >= v7)
//            goto SKIP_PATCH_LOADING;
//    }
//    if (v10 > 0)
//    {
//        v8 = v11 + 1;
//        goto LABEL_13;
//    }
//
//    const int patchNumber = g_pnPatchNumbers[v11];
//
//    if (patchNumber)
//    {
//        char* const extension = (char*)V_GetFileExtension(pakFilePath, true);
//        snprintf(extension, &relativeFilePath[sizeof(relativeFilePath)] - extension, "(%02u).rpak", patchNumber);
//    }
//
//SKIP_PATCH_LOADING:
//    pakFilePath = relativeFilePath;
//LABEL_27:
//    pakFileHandle = FS_OpenAsyncFile(pakFilePath, loadedInfo->logLevel, &pak_file_size_var);
//
//    if (pakFileHandle == FS_ASYNC_FILE_INVALID)
//    {
//        if (async_debug_level.GetInt() >= loadedInfo->logLevel)
//            Error(eDLL_T::RTECH, NO_ERROR, "Couldn't read package file \"%s\".\n", pakFilePath);
//
//        loadedInfo->status = PAK_STATUS_ERROR;
//        return false;
//    }
//
//    loadedInfo->fileHandle = pakFileHandle;
//
//    // file appears truncated/corrupt
//    if (pak_file_size_var < sizeof(PakFileHeader_t))
//    {
//        loadedInfo->status = PAK_STATUS_ERROR;
//        return false;
//    }
//
//    AsyncFile = v_FS_ReadAsyncFile(pakFileHandle, 0i64, sizeof(PakFileHeader_t), &pakHdr, 0i64, 0i64, 4);
//    v_FS_WaitForAsyncRead(AsyncFile);
//
//    size_t bytesProcessed = 0;
//    const char* statusMsg = "(no reason)";
//    const uint8_t currentStatus = g_pakLoadApi->CheckAsyncRequest(AsyncFile, &bytesProcessed, &statusMsg);
//
//    if (currentStatus == AsyncHandleStatus_t::FS_ASYNC_ERROR)
//    {
//        Error(eDLL_T::RTECH, EXIT_FAILURE, "Error reading pak file \"%s\" -- %s\n", pak->memoryData.fileName, statusMsg);
//
//        loadedInfo->status = PAK_STATUS_ERROR;
//        return false;
//    }
//
//    if (pakHdr.magic != PAK_HEADER_MAGIC || pakHdr.version != PAK_HEADER_VERSION)
//    {
//        loadedInfo->status = PAK_STATUS_ERROR;
//        return false;
//    }
//
//    if (pakHdr.flags & PAK_HEADER_FLAGS_HAS_MODULE_EXTENDED)
//    {
//        v22 = V_GetFileExtension(pakFilePath);
//        v23 = v22 - pakFilePath;
//
//        if ((unsigned __int64)(v22 - pakFilePath + 4) >= 0x2000)
//        {
//            loadedInfo->status = PAK_STATUS_ERROR;
//            return false;
//        }
//
//        memcpy(LibFileName, pakFilePath, v22 - pakFilePath);
//
//        *(_DWORD*)&LibFileName[v23] = *(_DWORD*)".dll";
//        v86[v23] = '\0';
//
//        const HMODULE hModule = LoadLibraryA(LibFileName);
//        loadedInfo->hModule = hModule;
//
//        if (!hModule)
//        {
//            loadedInfo->status = PAK_STATUS_ERROR;
//            return false;
//        }
//    }
//
//    loadedInfo->fileTime = pakHdr.fileTime;
//    asset_entry_count_var = pakHdr.assetCount;
//
//    loadedInfo->assetCount = pakHdr.assetCount;
//
//    asset_entry_count_var = pakHdr.assetCount;
//    patchIndex = pakHdr.patchIndex;
//    memPageCount = pakHdr.memPageCount;
//    virtualSegmentCount = pakHdr.virtualSegmentCount;
//    v32 = *(unsigned int*)&pakHdr.unk2[4];
//
//    loadedInfo->assetGuids = (PakGuid_t*)loadedInfo->allocator->Alloc(sizeof(PakGuid_t) * asset_entry_count_var, 8);
//
//    size_t streamingFilesBuifSize = pakHdr.streamingFilesBufSize[STREAMING_SET_OPTIONAL] + pakHdr.streamingFilesBufSize[STREAMING_SET_MANDATORY];
//
//    v81 = 8 * memPageCount;
//
//    pak_file_size_var = streamingFilesBuifSize
//        + ((_WORD)patchIndex != 0 ? 8 : 0)
//        + 2
//        * (patchIndex
//            + 2
//            * (pakHdr.relationsCounts
//                + *(unsigned int*)pakHdr.unk2
//                + 3 * memPageCount
//                + 2 * (pakHdr.descriptorCount + pakHdr.guidDescriptorCount + 16i64 + 2 * (asset_entry_count_var + patchIndex + 4 * asset_entry_count_var + virtualSegmentCount))))
//        + v32;
//
//    v80 = 4 * asset_entry_count_var;
//    v90 = pak_file_size_var + 2080;
//    v33 = -((_DWORD)pak_file_size_var + 2080 + 4 * (_DWORD)asset_entry_count_var) & 7;
//    v89 = v33;
//    v34 = 4 * asset_entry_count_var + pak_file_size_var + 2080 + v33 + 8 * memPageCount + 12 * asset_entry_count_var;
//    v35 = (-(4 * (_DWORD)asset_entry_count_var + (_DWORD)pak_file_size_var + 2080 + (_DWORD)v33 + 8 * (_DWORD)memPageCount + 12 * (_DWORD)asset_entry_count_var) & 7) + 4088i64;
//
//    if ((pakHdr.flags & 0x100) != 0)
//    {
//        ringBufferStreamSize = PAK_DECODE_IN_RING_BUFFER_SIZE;
//        ringBufferOutSize = PAK_DECODE_OUT_RING_BUFFER_SIZE;
//
//        if (pakHdr.compressedSize < PAK_DECODE_IN_RING_BUFFER_SIZE && !(_WORD)patchIndex)
//            ringBufferStreamSize = (pakHdr.compressedSize + PAK_DECODE_IN_RING_BUFFER_SMALL_MASK) & 0xFFFFFFFFFFFFF000ui64;
//    }
//    else
//    {
//        ringBufferStreamSize = 0i64;
//        ringBufferOutSize = PAK_DECODE_IN_RING_BUFFER_SIZE;
//    }
//
//    if (ringBufferOutSize > pakHdr.decompressedSize && !(_WORD)patchIndex)
//        ringBufferOutSize = (pakHdr.decompressedSize + PAK_DECODE_IN_RING_BUFFER_SMALL_MASK) & 0xFFFFFFFFFFFFF000ui64;
//
//    pak = (PakFile_t*)AlignedMemAlloc()->Alloc(v34 + v35 + ringBufferOutSize + ringBufferStreamSize, 8);
//
//    if (pak)
//    {
//        loadedInfo->pakFile = pak;
//
//        *(_QWORD*)&pak->processedPageCount = 0i64;
//        *(_QWORD*)&pak->patchCount = 0i64;
//        *(_QWORD*)&pak->numProcessedPointers = 0i64;
//        *(_QWORD*)&pak->memoryData.someAssetCount = 0i64;
//
//        v40 = (_DWORD*)loadedInfo->allocator->Alloc(16i64 * pakHdr.guidDescriptorCount + 8, 8i64);
//        loadedInfo->qword50 = v40;
//
//        *v40 = 0;
//
//        *(_DWORD*)(loadedInfo->qword50 + 4i64) = 0;
//
//        pak->memoryData.pakHeader = pakHdr;
//
//        pak->memoryData.pakId = pakId;
//        pak->memoryData.qword2D0 = pak_file_size_var;
//
//        JobID_t jobId = PAK_DEFAULT_JOB_GROUP_ID;
//
//        if (pakHdr.assetCount)
//            jobId = JT_BeginJobGroup(0);
//
//        pak->memoryData.unkJobID = jobId;
//
//
//        v43 = v81;
//        v44 = (uint8_t**)((char*)pak + v89 + v90 + v80);
//        pak->memoryData.memPageBuffers = v44;
//        v45 = (PakAsset_t**)((char*)v44 + v43);
//        pak->memoryData.ppAssetEntries = v45;
//        pak->memoryData.qword2E0 = (int*)&v45[pakHdr.assetCount];
//        p_patchHeader = &pak->memoryData.patchHeader;
//        v47 = pak->memoryData.pakHeader.patchIndex;
//
//        p_decompressedSize = (PakPatchFileHeader_t*)&pak->memoryData.patchHeader.decompressedSize;
//        if (!v47)
//            p_decompressedSize = &pak->memoryData.patchHeader;
//        if (!v47)
//            p_patchHeader = 0i64;
//
//        pak->memoryData.patchDataHeader = (PakPatchDataHeader_t*)p_patchHeader;
//        v49 = pak->memoryData.pakHeader.patchIndex;
//        pak->memoryData.patchHeaders = p_decompressedSize;
//        v50 = pak->memoryData.pakHeader.patchIndex;
//        v51 = (unsigned __int16*)&p_decompressedSize[v49];
//        pak->memoryData.patchIndices = v51;
//        v52 = (char*)&v51[v50];
//        v53 = &v52[pak->memoryData.pakHeader.streamingFilesBufSize[0]];
//        pak->memoryData.streamingFilePaths[0] = v52;
//        v54 = pak->memoryData.pakHeader.streamingFilesBufSize[1];
//        pak->memoryData.streamingFilePaths[1] = v53;
//        v55 = (PakSegmentHeader_t*)&v53[v54];
//        v56 = pak->memoryData.pakHeader.virtualSegmentCount;
//        pak->memoryData.segmentHeaders = v55;
//        v57 = pak->memoryData.pakHeader.memPageCount;
//        v58 = (PakPageHeader_t*)&v55[v56];
//        pak->memoryData.pageHeaders = v58;
//        descriptorCount = pak->memoryData.pakHeader.descriptorCount;
//        v60 = (PakPage_t*)&v58[v57];
//        pak->memoryData.virtualPointers = v60;
//        assetCount = pak->memoryData.pakHeader.assetCount;
//        v62 = (PakAsset_t*)&v60[descriptorCount];
//        pak->memoryData.assetEntries = v62;
//        guidDescriptorCount = pak->memoryData.pakHeader.guidDescriptorCount;
//        v64 = (PakPage_t*)&v62[assetCount];
//        pak->memoryData.guidDescriptors = v64;
//        relationsCounts = pak->memoryData.pakHeader.relationsCounts;
//        v66 = (uint32_t*)&v64[guidDescriptorCount];
//        pak->memoryData.fileRelations = v66;
//        v67 = *(unsigned int*)pak->memoryData.pakHeader.unk2;
//        v68 = &v66[relationsCounts];
//        *(_QWORD*)pak->memoryData.gap5E0 = v68;
//        v69 = *(unsigned int*)&pak->memoryData.pakHeader.unk2[4];
//        *(_QWORD*)&pak->memoryData.gap5E0[16] = 0i64;
//        v70 = &v68[v67];
//        *(_QWORD*)&pak->memoryData.gap5E0[8] = v70;
//        v71 = (__int64)v70 + v69;
//        *(_QWORD*)&pak->memoryData.gap5E0[24] = (char*)v70 + v69;
//        if (pak->memoryData.pakHeader.patchIndex)
//        {
//            patchDataHeader = pak->memoryData.patchDataHeader;
//            if (patchDataHeader->editStreamSize)
//            {
//                *(_QWORD*)&pak->memoryData.gap5E0[16] = v71;
//                *(_QWORD*)&pak->memoryData.gap5E0[24] = v71 + (unsigned int)patchDataHeader->editStreamSize;
//            }
//        }
//        v73 = PAK_DECODE_IN_RING_BUFFER_MASK;
//
//        pak->memoryData.fileName = loadedInfo->fileName;
//        pak->fileStream.qword0 = 0i64;
//        pak->fileStream.fileSize = pakHdr.compressedSize;
//        pak->fileStream.fileHandle = loadedInfo->fileHandle;
//        loadedInfo->fileHandle = FS_ASYNC_FILE_INVALID;
//
//        pak->fileStream.bufferMask = PAK_DECODE_IN_RING_BUFFER_MASK;
//
//        ringBuffer = (uint8_t*)(((unsigned __int64)pak + v35 + v34) & 0xFFFFFFFFFFFFF000ui64);
//
//        pak->fileStream.buffer = ringBuffer;
//        pak->fileStream.numDataChunksProcessed = 0;
//        pak->fileStream.numDataChunks = 0;
//        pak->fileStream.fileReadStatus = 3;
//        pak->fileStream.finishedLoadingPatches = 0;
//        pak->fileStream.numLoadedFiles = 0;
//        pak->fileStream.bytesStreamed = sizeof(PakFileHeader_t);
//        pak->decompBuffer = &ringBuffer[ringBufferStreamSize];
//        pak->inputBytePos = sizeof(PakFileHeader_t);
//        pak->byte1F8 = 0;
//        pak->byte1FD = 1;
//        pak->isOffsetted_MAYBE = 0;
//        pak->isCompressed = 0;
//
//        // FINISHME: this means if the pak file is not encoded, but we should also
//        // check on the zstd flags
//        v75 = (pakHdr.flags & 0x100) == 0;
//
//        pak->qword298 = 128i64;
//
//        if (!v75)
//            v73 = PAK_DECODE_OUT_RING_BUFFER_MASK;
//
//        pak->maxCopySize = v73;
//        memset(&pak->pakDecoder, 0, sizeof(pak->pakDecoder));
//
//        pak->pakDecoder.outBufBytePos = 128i64;
//        pak->pakDecoder.decompSize = 128i64;
//        pak->memoryData.processedPatchedDataSize = 128i64;
//        v76 = pakHdr.patchIndex;
//        v77 = pakHdr.descriptorCount + 2 * (pakHdr.patchIndex + (unsigned __int64)pakHdr.assetCount + 4i64 * pakHdr.assetCount + pakHdr.virtualSegmentCount);
//        v78 = pakHdr.memPageCount;
//
//        pak->memoryData.field_2A8 = 0i64;
//        pak->memoryData.patchData = 0i64;
//        pak->memoryData.patchDataPtr = 0i64;
//        pak->memoryData.bitBuf.m_dataBuf = 0i64;
//        pak->memoryData.bitBuf.m_bitsAvailable = 0;
//        pak->memoryData.patchDataOffset = 0;
//        pak->memoryData.patchSrcSize = v82 + ((_WORD)v76 != 0 ? 8 : 0) + 2 * (v76 + 6 * v78 + 4 * v77);
//        pak->memoryData.patchDstPtr = (char*)&pak->memoryData.patchHeader;
//        pak->memoryData.patchFunc = g_pakPatchApi[0];
//
//        LOBYTE(v2) = pakHdr.patchIndex == 0;
//        pak->memoryData.numBytesToProcess_maybe = pak->memoryData.pakHeader.decompressedSize + v2 - 0x80;
//
//        Pak_ProcessPakFile(pak);
//        return true;
//    }
//    else
//    {
//        loadedInfo->status = PAK_STATUS_ERROR;
//        return false;
//    }
//
//    return result;
//}


void V_PakParse::Detour(const bool bAttach) const
{
    DetourSetup(&v_Pak_LoadAsync, &Pak_LoadAsync, bAttach);
    DetourSetup(&v_Pak_UnloadAsync, &Pak_UnloadAsync, bAttach);

    DetourSetup(&v_Pak_StartLoadingPak, &Pak_StartLoadingPak, bAttach);

    DetourSetup(&v_Pak_ProcessPakFile, &Pak_ProcessPakFile, bAttach);
    DetourSetup(&v_Pak_ResolveAssetRelations, &Pak_ResolveAssetRelations, bAttach);
    DetourSetup(&v_Pak_ProcessAssets, &Pak_ProcessAssets, bAttach);

    DetourSetup(&v_Pak_RunAssetLoadingJobs, &Pak_RunAssetLoadingJobs, bAttach);
}
