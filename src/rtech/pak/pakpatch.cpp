//=============================================================================//
//
// Purpose: pak page patching
//
//=============================================================================//
#include "rtech/ipakfile.h"
#include "pakpatch.h"

static bool PATCH_CMD_0(PakFile_s* const pak, size_t* const numAvailableBytes)
{
    unsigned __int64 m_numBytesToProcess_maybe; // r9
    unsigned __int64 v4; // rdi
    unsigned __int64 v6; // rcx
    unsigned __int64 v7; // r8
    uint64_t m_processedPatchedDataSize; // rax
    uint64_t v9; // rdx
    size_t m_maxCopySize; // rax
    size_t m_patchSrcSize; // rsi
    char* m_patchDstPtr; // rcx
    size_t v13; // r14
    char* m_decompBuffer; // rdx
    size_t v15; // r8

    m_numBytesToProcess_maybe = pak->memoryData.numPatchBytesToProcess;
    v4 = *numAvailableBytes;
    v6 = *numAvailableBytes;
    v7 = pak->memoryData.field_2A8;

    if (m_numBytesToProcess_maybe < *numAvailableBytes)
        v6 = m_numBytesToProcess_maybe;

    if (v7)
    {
        m_processedPatchedDataSize = pak->memoryData.processedPatchedDataSize;

        if (v6 <= v7)
        {
            pak->memoryData.processedPatchedDataSize = v6 + m_processedPatchedDataSize;
            pak->memoryData.field_2A8 = v7 - v6;
            pak->memoryData.numPatchBytesToProcess = m_numBytesToProcess_maybe - v6;
            *numAvailableBytes = v4 - v6;
            return pak->memoryData.numPatchBytesToProcess == 0;
        }

        pak->memoryData.field_2A8 = 0i64;
        pak->memoryData.processedPatchedDataSize = v7 + m_processedPatchedDataSize;
        v6 -= v7;
        pak->memoryData.numPatchBytesToProcess = m_numBytesToProcess_maybe - v7;
        v4 -= v7;
    }

    v9 = pak->memoryData.processedPatchedDataSize;
    m_maxCopySize = pak->maxCopySize;
    m_patchSrcSize = pak->memoryData.patchSrcSize;

    if (v6 < m_patchSrcSize)
        m_patchSrcSize = v6;

    m_patchDstPtr = pak->memoryData.patchDstPtr;
    v13 = (m_maxCopySize & ~v9) + 1; // maxCopySize minus processedPatchedDataSize with a minimum of 1
    m_decompBuffer = (char*)pak->decompBuffer + (m_maxCopySize & v9);

    if (m_patchSrcSize > v13)
    {
        memcpy(m_patchDstPtr, m_decompBuffer, v13);
        m_decompBuffer = (char*)pak->decompBuffer;
        v15 = m_patchSrcSize - v13;
        m_patchDstPtr = &pak->memoryData.patchDstPtr[v13];
    }
    else
    {
        v15 = m_patchSrcSize;
    }

    memcpy(m_patchDstPtr, m_decompBuffer, v15);
    pak->memoryData.processedPatchedDataSize += m_patchSrcSize;
    pak->memoryData.patchSrcSize -= m_patchSrcSize;
    pak->memoryData.patchDstPtr += m_patchSrcSize;
    pak->memoryData.numPatchBytesToProcess -= m_patchSrcSize;
    *numAvailableBytes = v4 - m_patchSrcSize;


    return pak->memoryData.numPatchBytesToProcess == 0;
}

static bool PATCH_CMD_1(PakFile_s* const pak, size_t* const pNumBytesAvailable)
{
    const size_t numBytesToProcess = pak->memoryData.numPatchBytesToProcess;
    const size_t numBytesAvailable = *pNumBytesAvailable;
    const size_t processedPatchedDataSize = pak->memoryData.processedPatchedDataSize;

    if (*pNumBytesAvailable > numBytesToProcess)
    {
        pak->memoryData.numPatchBytesToProcess = 0ull;
        pak->memoryData.processedPatchedDataSize += numBytesToProcess;
        *pNumBytesAvailable = numBytesAvailable - numBytesToProcess;

        return true;
    }
    else
    {
        pak->memoryData.processedPatchedDataSize += numBytesAvailable;
        pak->memoryData.numPatchBytesToProcess -= numBytesAvailable;
        *pNumBytesAvailable = NULL;

        return false;
    }
}

static bool PATCH_CMD_2(PakFile_s* const pak, size_t* const pNumBytesAvailable)
{
    NOTE_UNUSED(pNumBytesAvailable);

    size_t numBytesToProcess = pak->memoryData.numPatchBytesToProcess;
    const size_t v3 = pak->memoryData.field_2A8;

    if (v3)
    {
        if (numBytesToProcess <= v3)
        {
            pak->memoryData.numPatchBytesToProcess = 0ull;
            pak->memoryData.patchDataPtr += numBytesToProcess;
            pak->memoryData.field_2A8 = v3 - numBytesToProcess;

            return true;
        }

        pak->memoryData.field_2A8 = 0i64;
        numBytesToProcess -= v3;
        pak->memoryData.patchDataPtr += v3;
        pak->memoryData.numPatchBytesToProcess = numBytesToProcess;
    }

    const size_t patchSrcSize = Min(numBytesToProcess, pak->memoryData.patchSrcSize);

    memcpy(pak->memoryData.patchDstPtr, pak->memoryData.patchDataPtr, patchSrcSize);

    pak->memoryData.patchDataPtr += patchSrcSize;
    pak->memoryData.patchSrcSize -= patchSrcSize;
    pak->memoryData.patchDstPtr += patchSrcSize;
    pak->memoryData.numPatchBytesToProcess -= patchSrcSize;

    return pak->memoryData.numPatchBytesToProcess == 0;
}

static bool PATCH_CMD_3(PakFile_s* const pak, size_t* const pNumBytesAvailable)
{
    const size_t numBytesLeft = Min(*pNumBytesAvailable, pak->memoryData.numPatchBytesToProcess);
    const size_t patchSrcSize = Min(numBytesLeft, pak->memoryData.patchSrcSize);

    memcpy(pak->memoryData.patchDstPtr, pak->memoryData.patchDataPtr, patchSrcSize);
    pak->memoryData.patchDataPtr += patchSrcSize;
    pak->memoryData.processedPatchedDataSize += patchSrcSize;
    pak->memoryData.patchSrcSize -= patchSrcSize;
    pak->memoryData.patchDstPtr += patchSrcSize;
    pak->memoryData.numPatchBytesToProcess -= patchSrcSize;
    *pNumBytesAvailable = *pNumBytesAvailable - patchSrcSize;

    return pak->memoryData.numPatchBytesToProcess == 0;
}

static bool PATCH_CMD_4_5(PakFile_s* const pak, size_t* const pNumBytesAvailable)
{
    const size_t numBytesAvailable = *pNumBytesAvailable;

    if (!numBytesAvailable)
        return false;

    *pak->memoryData.patchDstPtr = *(_BYTE*)pak->memoryData.patchDataPtr++;
    ++pak->memoryData.processedPatchedDataSize;
    --pak->memoryData.patchSrcSize;
    ++pak->memoryData.patchDstPtr;
    pak->memoryData.patchFunc = PATCH_CMD_0;
    *pNumBytesAvailable = numBytesAvailable - 1;

    return PATCH_CMD_0(pak, pNumBytesAvailable);
}

static bool PATCH_CMD_6(PakFile_s* const pak, size_t* const pNumBytesAvailable)
{
    const size_t numBytesAvailable = *pNumBytesAvailable;
    size_t numBytesToSkip = 2;

    if (*pNumBytesAvailable < 2)
    {
        if (!*pNumBytesAvailable)
            return false;

        numBytesToSkip = *pNumBytesAvailable;
    }

    const void* const patchDataPtr = (const void*)pak->memoryData.patchDataPtr;
    const size_t patchSrcSize = pak->memoryData.patchSrcSize;
    char* const patchDstPtr = pak->memoryData.patchDstPtr;

    if (numBytesToSkip > patchSrcSize)
    {
        memcpy(patchDstPtr, patchDataPtr, patchSrcSize);
        pak->memoryData.patchDataPtr += patchSrcSize;
        pak->memoryData.processedPatchedDataSize += patchSrcSize;
        pak->memoryData.patchSrcSize -= patchSrcSize;
        pak->memoryData.patchDstPtr += patchSrcSize;
        pak->memoryData.patchFunc = PATCH_CMD_4_5;
        *pNumBytesAvailable = numBytesAvailable - patchSrcSize;
    }
    else
    {
        memcpy(patchDstPtr, patchDataPtr, numBytesToSkip);
        pak->memoryData.patchDataPtr += numBytesToSkip;
        pak->memoryData.processedPatchedDataSize += numBytesToSkip;
        pak->memoryData.patchSrcSize -= numBytesToSkip;
        pak->memoryData.patchDstPtr += numBytesToSkip;

        if (numBytesAvailable >= 2)
        {
            pak->memoryData.patchFunc = PATCH_CMD_0;
            *pNumBytesAvailable = numBytesAvailable - numBytesToSkip;

            return PATCH_CMD_0(pak, pNumBytesAvailable);
        }

        pak->memoryData.patchFunc = PATCH_CMD_4_5;
        *pNumBytesAvailable = NULL;
    }

    return false;
}

const PakPatchFuncs_s g_pakPatchApi
{
    PATCH_CMD_0,
    PATCH_CMD_1,
    PATCH_CMD_2,
    PATCH_CMD_3,
    PATCH_CMD_4_5,
    PATCH_CMD_4_5,
    PATCH_CMD_6,
};
