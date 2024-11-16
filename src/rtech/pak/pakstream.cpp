//=============================================================================//
//
// Purpose: pak stream data loading and unloading
//
//=============================================================================//
#include "tier0/commandline.h"
#include "rtech/async/asyncio.h"
#include "rtech/ipakfile.h"

#include "pakparse.h"
#include "pakstate.h"
#include "pakstream.h"

//-----------------------------------------------------------------------------
// determines whether or not to emulate the streaming install, this basically
// tells the system to not load streaming sets at all. note that this only
// applies to optional streaming sets, mandatory ones must be available!
//-----------------------------------------------------------------------------
static bool Pak_ShouldEmulateStreamingInstall()
{
    // don't run the command line check every query
    if (g_pakGlobals->emulateStreamingInstallInit)
        return g_pakGlobals->emulateStreamingInstall;

    const char* value = nullptr;

    if (CommandLine()->CheckParm("-emulate_streaming_install", &value))
    {
        if (value && atoi(value))
            g_pakGlobals->emulateStreamingInstall = true;
    }

    g_pakGlobals->emulateStreamingInstallInit = true;
    return g_pakGlobals->emulateStreamingInstall;
}

//-----------------------------------------------------------------------------
// returns whether the optional streaming sets are finished downloading
//-----------------------------------------------------------------------------
static bool Pak_OptionalStreamingDataDownloaded()
{
    return (!Pak_ShouldEmulateStreamingInstall() && Pak_StreamingDownloadFinished());
}

//-----------------------------------------------------------------------------
// opens all associated streaming assets for this pak
//-----------------------------------------------------------------------------
void Pak_OpenAssociatedStreamingFiles(PakLoadedInfo_s* const loadedInfo, PakLoadedInfo_s::StreamingInfo_t& streamInfo,
    const uint16_t fileNamesBufSize, const PakStreamSet_e set)
{
    assert(set < STREAMING_SET_COUNT);

    const PakMemoryData_s& memoryData = loadedInfo->pakFile->memoryData;
    uint16_t numStreamFiles = 0;

    // load all streaming sets
    for (uint64_t lenRead = 0; lenRead < fileNamesBufSize && numStreamFiles < PAK_MAX_STREAMING_FILE_HANDLES_PER_SET;)
    {
        // read streaming path and advance buffer
        const char* const streamingFilePath = &memoryData.streamingFilePaths[set][lenRead];

        // check if we processed all strings, the buffer is aligned to 4 bytes,
        // so its possible we reach padding before the end of the buffer
        if (!*streamingFilePath)
            break;

        // must advance over null character as well for the next read
        lenRead += strnlen(streamingFilePath, fileNamesBufSize - lenRead) + 1;

        const int fileNumber = FS_OpenAsyncFile(streamingFilePath, loadedInfo->logChannel, nullptr);

        // make sure we successfully loaded mandatory streaming files, as we
        // would otherwise error in the game itself
        if (set == STREAMING_SET_MANDATORY && fileNumber == FS_ASYNC_FILE_INVALID)
            Error(eDLL_T::RTECH, EXIT_FAILURE, "Error opening streaming file '%s'\n", streamingFilePath);

        streamInfo.streamFileNumber[numStreamFiles++] = fileNumber;
    }

    streamInfo.streamFileCount = numStreamFiles;
}

//-----------------------------------------------------------------------------
// allocates the pak string to be used for embedded streaming data
//-----------------------------------------------------------------------------
void Pak_EnableEmbeddedStreamingData(PakLoadedInfo_s* const loadedInfo, PakLoadedInfo_s::StreamingInfo_t& streamInfo)
{
    const char* const baseName = V_UnqualifiedFileName(loadedInfo->fileName);
    const size_t baseNameLen = strlen(baseName);

    // if the path isn't specified, we have to prepend one
    const bool hasPath = (baseName != loadedInfo->fileName);

    const size_t basePathLen = hasPath ? 0 : strlen(PAK_BASE_PATH);
    const size_t totalBufLen = basePathLen + baseNameLen + 1;

    char* const embeddedName = reinterpret_cast<char* const>(loadedInfo->allocator->Alloc(totalBufLen, 1));
    assert(embeddedName);

    // copy the base path if none was found in the file name
    if (!hasPath)
        memcpy(embeddedName, PAK_BASE_PATH, basePathLen);

    memcpy(embeddedName + basePathLen, baseName, baseNameLen);

    // at this point we shouldn't have read loose streaming data, we only
    // should be looking for embedded if there are no external ones !!!
    assert(streamInfo.streamFileCount == 0);

    streamInfo.streamFileCount = 1;
}

//-----------------------------------------------------------------------------
// parse and open all streaming files
//-----------------------------------------------------------------------------
void Pak_LoadStreamingData(PakLoadedInfo_s* const loadedInfo)
{
    const PakFileHeader_s& pakHeader = loadedInfo->pakFile->GetHeader();

    for (int i = 0; i < STREAMING_SET_COUNT; i++)
    {
        PakLoadedInfo_s::StreamingInfo_t& streamInfo = loadedInfo->streamInfo[i];
        streamInfo.Reset();

        const bool optional = (i == STREAMING_SET_OPTIONAL);

        // NOTE: mandatory streaming data must be available at this point!
        const bool disableStreaming = optional ? !Pak_OptionalStreamingDataDownloaded() : false;
        streamInfo.streamingDisabled = disableStreaming;

        // don't attempt to open the streaming file if it isn't downloaded yet
        if (disableStreaming)
            continue;

        const uint16_t filesBufLen = pakHeader.streamingFilesBufSize[i];
        const uint64_t embeddedStreamingDataSize = pakHeader.embeddedStreamingDataSize[i];

        if (filesBufLen > 0)
        {
            // embedded streaming data won't be loaded if the pak is linked to
            // external streaming files; mistake while building the pak?
            assert(!embeddedStreamingDataSize);

            Pak_OpenAssociatedStreamingFiles(loadedInfo, streamInfo, filesBufLen, PakStreamSet_e(i));
        }
        else if (embeddedStreamingDataSize > 0)
        {
            Pak_EnableEmbeddedStreamingData(loadedInfo, streamInfo);
        }
    }
}
