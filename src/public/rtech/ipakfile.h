//=============================================================================//
//
// Purpose: pak file constants and types
//
//=============================================================================//
#ifndef RTECH_IPACKFILE_H
#define RTECH_IPACKFILE_H
#include "tier0/jobthread.h"
#include "tier0/tslist.h"

#include "rtech/iasync.h"
#include "rtech/rstdlib.h"

// pak header versions
#define PAK_HEADER_MAGIC (('k'<<24)+('a'<<16)+('P'<<8)+'R')
#define PAK_HEADER_VERSION 8

// pak header flags
#define PAK_HEADER_FLAGS_HAS_MODULE (1<<0)
#define PAK_HEADER_FLAGS_HAS_MODULE_EXTENDED (PAK_HEADER_FLAGS_HAS_MODULE | (1<<1))

#define PAK_HEADER_FLAGS_COMPRESSED (1<<8)

// use the ZStd decoder instead of the RTech one
#define PAK_HEADER_FLAGS_ZSTREAM_ENCODED (1<<9)

// max amount of types at runtime in which assets will be tracked
#define PAK_MAX_TRACKED_TYPES 64
#define PAK_MAX_TRACKED_TYPES_MASK (PAK_MAX_TRACKED_TYPES-1)

// max amount of global pak assets at runtime
#define PAK_MAX_LOADED_ASSETS 0x40000
#define PAK_MAX_LOADED_ASSETS_MASK (PAK_MAX_LOADED_ASSETS-1)

// max amount of global pak assets tracked at runtime
#define PAK_MAX_TRACKED_ASSETS (PAK_MAX_LOADED_ASSETS/2)
#define PAK_MAX_TRACKED_ASSETS_MASK (PAK_MAX_TRACKED_ASSETS-1)

// max amount of segments a pak file could have
#define PAK_MAX_SEGMENTS 20

// max amount of buffers in which segments get copied in
#define PAK_SEGMENT_BUFFER_TYPES 4

// max amount of streaming files that could be opened per set for a pak, so if a
// pak uses more than one set, this number would be used per set
#define PAK_MAX_STREAMING_FILE_HANDLES_PER_SET 4

// max amount of paks that could be loaded at runtime
#define PAK_MAX_LOADED_PAKS 512
#define PAK_MAX_LOADED_PAKS_MASK (PAK_MAX_LOADED_PAKS-1)

// max amount of async streaming requests that could be made per pak file, if a
// pak file has more patches than this number, and is already trying to stream
// this amount in, all remainder patches would have to wait until one slot
// becomes available again
#define PAK_MAX_ASYNC_STREAMED_LOAD_REQUESTS 8
#define PAK_MAX_ASYNC_STREAMED_LOAD_REQUESTS_MASK (PAK_MAX_ASYNC_STREAMED_LOAD_REQUESTS-1)

// the minimum stream ring buffer size when a pak is loaded who's compressed
// size is below PAK_DECODE_IN_RING_BUFFER_SIZE, the system allocates a
// buffer with the size of the compressed pak + the value of this define
// the system should still use the default ring buffer input size for decoding
// as these pak files get decoded in one-go; there is no input buffer wrapping
// going on
#define PAK_DECODE_IN_RING_BUFFER_SMALL_SIZE 0x1000
#define PAK_DECODE_IN_RING_BUFFER_SMALL_MASK (PAK_DECODE_IN_RING_BUFFER_SMALL_SIZE-1)

// the input stream ring buffer size for pak decoder before wrapping around
#define PAK_DECODE_IN_RING_BUFFER_SIZE 0x1000000
#define PAK_DECODE_IN_RING_BUFFER_MASK (PAK_DECODE_IN_RING_BUFFER_SIZE-1)

// the output stream ring buffer size in which input buffer gets decoded to, we
// can only decode up to this many bytes before we have to wrap around
#define PAK_DECODE_OUT_RING_BUFFER_SIZE 0x400000
#define PAK_DECODE_OUT_RING_BUFFER_MASK (PAK_DECODE_OUT_RING_BUFFER_SIZE-1)

// max amount to read per async fs read request
#define PAK_READ_DATA_CHUNK_SIZE (1ull << 19)

// max amount of data chunks per pak stream instance
#define PAK_MAX_DATA_CHUNKS_PER_STREAM 32
#define PAK_MAX_DATA_CHUNKS_PER_STREAM_MASK (PAK_MAX_DATA_CHUNKS_PER_STREAM-1)

// base pak directory containing paks sorted in platform specific subdirectories
#define PAK_BASE_PATH "paks\\"
#define PAK_PLATFORM_PATH PAK_BASE_PATH"Win64\\"

// pak override directory; the system will looks for pak files in this directory
// first before falling back to PAK_PLATFORM_PATH
#define PAK_PLATFORM_OVERRIDE_PATH PAK_BASE_PATH"Win64_override\\"

// the handle that should be returned when a pak failed to load or process
#define PAK_INVALID_HANDLE -1

#define PAK_MAX_DISPATCH_LOAD_JOBS 4
#define PAK_DEFAULT_JOB_GROUP_ID 0x3000

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct PakFile_s;

//-----------------------------------------------------------------------------
// Handle types
//-----------------------------------------------------------------------------
typedef int PakHandle_t;
typedef uint64_t PakGuid_t;

//-----------------------------------------------------------------------------
// Page handle types
//-----------------------------------------------------------------------------
struct PakPageHeader_s
{
	uint32_t segmentIdx;
	uint32_t pageAlignment;
	uint32_t dataSize;
};

// ptr.index  != UINT32_MAX
// ptr.offset != UINT32_MAX
// ptr.index   < pak->GetPageCount()
// ptr.offset <= pak->GetPageSize(ptr.index)
#define IS_PAKPTR_VALID(pak, ptr) ((ptr)->index != UINT32_MAX && (ptr)->offset != UINT32_MAX && (ptr)->index < (pak)->GetPageCount() && (ptr)->offset <= (pak)->GetPageSize((ptr)->index))
#define ASSERT_PAKPTR_VALID(pak, ptr) Assert(IS_PAKPTR_VALID(pak, ptr), "Invalid pak page pointer")

union PakPage_u
{
	struct
	{
		uint32_t index;
		uint32_t offset;
	};
	void* ptr;
};

//-----------------------------------------------------------------------------
// Enumerations
//-----------------------------------------------------------------------------
enum PakDecodeMode_e
{
	MODE_DISABLED = -1,

	// the default decoder
	MODE_RTECH,
	MODE_ZSTD
};

enum PakStreamSet_e
{
	STREAMING_SET_MANDATORY = 0,
	STREAMING_SET_OPTIONAL,

	// number of streaming sets
	STREAMING_SET_COUNT
};

enum PakStatus_e
{
	PAK_STATUS_FREED = 0,
	PAK_STATUS_LOAD_PENDING = 1,
	PAK_STATUS_REPAK_RUNNING = 2,
	PAK_STATUS_REPAK_DONE = 3,
	PAK_STATUS_LOAD_STARTING = 4,
	PAK_STATUS_LOAD_PAKHDR = 5,
	PAK_STATUS_LOAD_PATCH_INIT = 6,
	PAK_STATUS_LOAD_PATCH_EDIT_STREAM = 7,
	PAK_STATUS_LOAD_ASSETS = 8,
	PAK_STATUS_LOADED = 9,
	PAK_STATUS_UNLOAD_PENDING = 10,
	PAK_STATUS_FREE_PENDING = 11,
	PAK_STATUS_CANCELING = 12,
	PAK_STATUS_ERROR = 13,
	PAK_STATUS_INVALID_PAKHANDLE = 14,
	PAK_STATUS_BUSY = 15
};

struct PakAssetBinding_s
{
	enum EType
	{
		NONE = 0,

		// not registered, assets with type 'NONE'
		// will be stubbed with this value!
		STUB,

		// explicitly registered by pak or code
		REGISTERED
	};

	// for example '0x6C74616D' for the material asset.
	uint32_t extension;
	uint32_t version;

	// description or name of asset.
	const char* description;

	// asset specific load callbacks
	void* loadAssetFunc;
	void* unloadAssetFunc;
	void* replaceAssetFunc;

	CAlignedMemAlloc* allocator;

	unsigned int headerSize;
	unsigned int nativeClassSize; // Native class size, for 'material' it would be CMaterialGlue full size.
	unsigned int headerAlignment;

	// the type of this asset bind
	// NOTE: the asset bind will be stubbed if its 'NONE' in runtime!
	EType type;

	// [ PIXIE ]: Should be the full size across Season 0-3.
};

struct PakAsset_s
{
	// the guid of this asset, which will be used to index into, and retrieve
	// this asset from the hash table
	PakGuid_t guid;
	uint64_t padding; // Unknown.

	PakPage_u headPtr;
	PakPage_u dataPtr;

	// offset to the streaming data in the streaming set
	uint64_t streamingDataFileOffset[STREAMING_SET_COUNT];
	uint16_t pageEnd;

	// the number of remaining dependencies that are yet to be resolved
	uint16_t numRemainingDependencies;
	uint32_t dependentsIndex;
	uint32_t dependenciesIndex;
	uint32_t dependentsCount;
	uint32_t dependenciesCount;

	// size of the asset's header
	uint32_t headerSize;

	// versions of the asset
	uint32_t version;
	uint32_t magic;

	FORCEINLINE uint8_t HashTableIndexForAssetType() const
	{
		return (((0x1020601 * magic) >> 24) & PAK_MAX_TRACKED_TYPES_MASK);
	}
};

struct PakAssetShort_s
{
	PakGuid_t guid;
	uint32_t trackerIndex;
	uint32_t unk_C;
	void* head;
	void* cpu;
};

struct PakAssetTracker_s
{
	void* memPage;
	int trackerIndex;
	int loadedPakIndex;
	uint8_t assetTypeHashIdx;
	char unk_11[3];
	char unk_10[4];
};

struct PakTracker_s
{
	uint32_t numPaksTracked;
	int unk_4;
	int unk_8;
	char gap_C[644100];
	int loadedAssetIndices[PAK_MAX_LOADED_PAKS];
	char gap_9DC04[522240];
};

class PakLoadedInfo_s
{
public:
	struct StreamingInfo_t
	{
		inline void Reset()
		{
			embeddedStarpakName = nullptr;
			streamFileNumber[0] = FS_ASYNC_FILE_INVALID;
			streamFileNumber[1] = FS_ASYNC_FILE_INVALID;
			streamFileNumber[2] = FS_ASYNC_FILE_INVALID;
			streamFileNumber[3] = FS_ASYNC_FILE_INVALID;
			streamFileCount = NULL;
			streamingDisabled = true;
		}

		char* embeddedStarpakName;
		int streamFileNumber[PAK_MAX_STREAMING_FILE_HANDLES_PER_SET];
		int streamFileCount;
		bool streamingDisabled;
		char padding_maybe[3];
	};

	PakHandle_t handle;
	PakStatus_e status;
	JobID_t loadJobId;
	uint32_t padding_maybe;

	// the log channel of the pak, see [r5apex.exe+43E873]; this is used
	// for errors if a pak failed to load. the value is assigned when the
	// pak is loaded through Pak_LoadAsync or Pak_LoadAsyncAndWaitOrHelp.
	int logChannel;

	uint32_t assetCount;
	const char* fileName;
	CAlignedMemAlloc* allocator;
	PakGuid_t* assetGuids; //size of the array is m_nAssetCount
	void* segmentBuffers[PAK_SEGMENT_BUFFER_TYPES];
	void* guidDestriptors;
	FILETIME fileTime;
	PakFile_s* pakFile;
	StreamingInfo_t streamInfo[STREAMING_SET_COUNT];
	uint32_t fileHandle;
	uint8_t m_nUnk5;
	HMODULE hModule;

}; //Size: 0x00B8/0x00E8

struct PakGlobalState_s
{
	// [ PIXIE ]: Max possible registered assets on Season 3, 0-2 I did not check yet.
	PakAssetBinding_s assetBindings[PAK_MAX_TRACKED_TYPES];
	PakAssetShort_s loadedAssets[PAK_MAX_LOADED_ASSETS];

	// assets that are tracked across all asset types
	PakAssetTracker_s trackedAssets[PAK_MAX_TRACKED_ASSETS];

	RHashMap trackedAssetMap; // links to 'trackedAssets'
	RHashMap loadedPakMap;    // links to 'loadedPaks'

	// all currently loaded pak handles
	PakLoadedInfo_s loadedPaks[PAK_MAX_LOADED_PAKS];

	RMultiHashMap unkMap2; // links to 'unkIntArray' and 'unkIntArray2'
	int unkIntArray[PAK_MAX_TRACKED_ASSETS];
	int unkIntArray2[PAK_MAX_LOADED_ASSETS];

	// whether asset streaming (mandatory & optional) is enabled
	b64 useStreamingSystem;

	// whether we should emulate (fake) our streaming install for debugging
	b64 emulateStreamingInstallInit;
	b64 emulateStreamingInstall;

	// mounted # streamable assets (globally across all paks)
	int64_t numStreamableAssets;
	b64 hasPendingUnloadJobs;

	// paks that contain tracked assets
	PakTracker_s* pakTracker;

	// called when threads have to be synced (e.g. syncing the render thread
	// when we execute the unloading of paks and assets)
	void* threadSyncFunc;

	// the index to the asset in the trackedAssets array of the last asset we
	// tracked
	int lastAssetTrackerIndex;
	bool updateSplitScreenAnims;

	// the current # pending asset loading jobs
	int16_t numAssetLoadJobs;
	JobFifoLock_s fifoLock;
	JobID_t pakLoadJobId;

	int16_t loadedPakCount;
	int16_t requestedPakCount;

	PakHandle_t loadedPakHandles[PAK_MAX_LOADED_PAKS];

	JobTypeID_t assetBindJobTypes[PAK_MAX_TRACKED_TYPES];
	JobTypeID_t jobTypeSlots_Unused[PAK_MAX_TRACKED_TYPES];

	JobTypeID_t dispatchLoadJobTypes[PAK_MAX_DISPATCH_LOAD_JOBS];
	uint8_t dispathLoadJobPriorities[PAK_MAX_DISPATCH_LOAD_JOBS]; // promoted to JobPriority_e
	uint32_t dispatchLoadJobAffinities[PAK_MAX_DISPATCH_LOAD_JOBS];

	RTL_SRWLOCK cpuDataLock;
	char unknown_or_unused[32];
	void* addToMapFunc;
	void* removeFromMapFunc;
	__int64 qword_167ED8540;
	int dword_167ED8548;
	int dword_167ED854C;
	__int64 qword_167ED8550;
	int dword_167ED8558;
	int unknown_dword_or_nothing;
	int dword_167ED8560;
	int numPatchedPaks;
	const char** patchedPakFiles;
	uint8_t* patchNumbers;
};

struct PakPatchFileHeader_s
{
	uint64_t compressedSize;
	uint64_t decompressedSize;
};

struct PakPatchDataHeader_s
{
	uint32_t editStreamSize;
	uint32_t pageCount;
};

struct PakFileHeader_s
{
	inline uint32_t GetTotalStreamingNamesBufferSize() const
	{
		return(
			streamingFilesBufSize[STREAMING_SET_MANDATORY] +
			streamingFilesBufSize[STREAMING_SET_OPTIONAL]);
	}

	inline uint64_t GetTotalEmbeddedStreamingDataSize() const
	{
		return (
			embeddedStreamingDataSize[STREAMING_SET_MANDATORY] + 
			embeddedStreamingDataSize[STREAMING_SET_OPTIONAL]);
	}

	inline uint64_t GetTotalHeaderSize() const
	{
		uint64_t headerSize = sizeof(PakFileHeader_s);

		// if we have patches, we should include the patch header as well
		if (patchIndex > 0)
			headerSize += sizeof(PakPatchDataHeader_s);

		// the streaming file paths belong to the header as well
		headerSize += GetTotalStreamingNamesBufferSize();
		return headerSize;
	}

	inline PakDecodeMode_e GetCompressionMode() const
	{
		if (flags & PAK_HEADER_FLAGS_ZSTREAM_ENCODED)
			return PakDecodeMode_e::MODE_ZSTD;

		// NOTE: this should be the first check once we rebuilt function
		// 14043F300 and alloc ring buffer for the flags individually instead
		// instead of having to define the main compress flag (which really
		// just means that the pak is using the RTech decoder)
		else if (flags & PAK_HEADER_FLAGS_COMPRESSED)
			return PakDecodeMode_e::MODE_RTECH;

		return PakDecodeMode_e::MODE_DISABLED;
	}

	// file versions
	uint32_t magic;       // 'RPak'
	uint16_t version;     // R2 = '7' R5 = '8'

	// pak file flags
	uint16_t flags;

	// when this pak file was built
	FILETIME fileTime;
	uint64_t checksum;

	// compressed size of the pak file, this includes the header
	uint64_t compressedSize;

	// offset to the embedded mandatory and optional streaming data
	// NOTE: this should be NULL if external streaming sets are used
	uint64_t embeddedStreamingDataOffset[STREAMING_SET_COUNT];

	// decompressed size of  this pak, this includes the header
	// NOTE: if the pak is uncompressed, this will equal compressedSize
	uint64_t decompressedSize;

	// size of the embedded mandatory and optional streaming data
	// NOTE: this should be NULL if external streaming sets are used
	uint64_t embeddedStreamingDataSize[STREAMING_SET_COUNT];

	// size of the string array containing paths to external streaming files
	uint16_t streamingFilesBufSize[STREAMING_SET_COUNT];

	// number of segments in this pak; absolute max = PAK_MAX_SEGMENTS
	uint16_t virtualSegmentCount;

	// number of memory pages to allocate for this pak
	uint16_t memPageCount;

	uint16_t patchIndex;

	uint32_t descriptorCount;

	// number of assets in this pak
	uint32_t assetCount;
	uint32_t guidDescriptorCount;
	uint32_t relationsCounts;

	uint8_t  unk2[0x10];

	// size not verified. offsets every page by x amount, if not 0 start of first page has data corresponding for 'patching some page'
	uint32_t memPageOffset;

	uint8_t  unk3[0x8];
}; static_assert(sizeof(PakFileHeader_s) == 0x80);

// segment flags
#define SF_HEAD (0)
#define SF_TEMP (1 << 0) // 0x1
#define SF_CPU  (1 << 1) // 0x2
#define SF_DEV  (1 << 8) // 0x80

struct PakSegmentHeader_s
{
	int typeFlags;
	int dataAlignment;
	size_t dataSize;
};

struct PakSegmentDescriptor_s
{
	size_t assetTypeCount[PAK_MAX_TRACKED_TYPES];
	int64_t segmentSizes[PAK_MAX_SEGMENTS];

	size_t segmentSizeForType[PAK_SEGMENT_BUFFER_TYPES];
	int segmentAlignmentForType[PAK_SEGMENT_BUFFER_TYPES];
};

struct PakDecoder_s
{
	const uint8_t* inputBuf;
	uint8_t* outputBuf;

	uint64_t inputMask;
	uint64_t outputMask;

	size_t fileSize;
	size_t decompSize;

	uint64_t inputInvMask;
	uint64_t outputInvMask;

	uint32_t headerOffset;

	// this field was unused, it now contains the decoder mode
	PakDecodeMode_e decodeMode;

	// NOTE: unless you are in the RTech decoder, use the getter if you need to
	// get the current pos!!!
	uint64_t inBufBytePos;
	// NOTE: unless you are in the RTech decoder, use the getter if you need to
	// get the current pos!!!
	uint64_t outBufBytePos;

	size_t bufferSizeNeeded;

	// current byte and current bit of byte
	uint64_t currentByte;
	uint32_t currentBit;

	uint32_t dword6C;
	uint64_t qword70;

	union
	{
		size_t compressedStreamSize;

		// compressedStreamSize isn't used on ZStd paks, instead, we need to
		// store the frame header size
		size_t frameHeaderSize;
	};

	union
	{
		size_t decompressedStreamSize;

		// decompressedStreamSize isn't used on ZStd paks; use this space for
		// the decoder
		ZSTD_DStream* zstreamContext;
	};
};

struct PakRingBufferFrame_s
{
	size_t bufIndex;
	size_t frameLen;
};

struct PakFileStream_s
{
	struct Descriptor
	{
		size_t dataOffset;
		size_t compressedSize;
		size_t decompressedSize;

		// NOTE: if this is set, the game sets 'PakMemoryData_t::processedPatchedDataSize'
		// to 'dataOffset'; else its getting set to 'sizeof(PakFileHeader_t)'.
		PakDecodeMode_e compressionMode;
	};

	size_t readOffset;
	size_t fileSize;
	int fileHandle;
	int asyncRequestHandles[PAK_MAX_DATA_CHUNKS_PER_STREAM];
	uint8_t dataChunkStatuses[PAK_MAX_DATA_CHUNKS_PER_STREAM];
	uint32_t numDataChunksProcessed;
	uint32_t numDataChunks;
	uint8_t fileReadStatus;
	bool finishedLoadingPatches;
	_BYTE gapBE;
	uint8_t numLoadedFiles;
	Descriptor descriptors[PAK_MAX_ASYNC_STREAMED_LOAD_REQUESTS];
	uint8_t* buffer;
	uint64_t bufferMask;
	uint64_t bytesStreamed;
};

struct PakPatchFuncs_s
{
	typedef bool (*PatchFunc_t)(PakFile_s* const pak, size_t* const numAvailableBytes);

	enum PatchCommands_e
	{
		PATCH_CMD0,
		PATCH_CMD1,
		PATCH_CMD2,
		PATCH_CMD3,
		PATCH_CMD4,
		PATCH_CMD5, // Same as cmd4.
		PATCH_CMD6,

		// !!! NOT A CMD !!!
		PATCH_CMD_COUNT
	};

	inline PatchFunc_t operator[](ssize_t i) const
	{
		Assert((i >= 0) && (i < SDK_ARRAYSIZE(patchFuncs)));
		return patchFuncs[i];
	}

	PatchFunc_t patchFuncs[PATCH_CMD_COUNT];

};

struct PakMemoryData_s
{
	uint64_t processedPatchedDataSize;
	char* patchData; // pointer to patch stream data

	char* patchDataPtr;
	RBitRead bitBuf;
	uint32_t patchDataOffset;

	_BYTE patchCommands[64];

	_BYTE PATCH_field_68[64];
	_BYTE PATCH_unk2[256];
	_BYTE PATCH_unk3[256];

	_QWORD field_2A8;

	// number of bytes remaining in the patch stream data
	size_t patchSrcSize;

	// pointer to the location in the pak that a patch command is writing to
	char* patchDstPtr;

	size_t numPatchBytesToProcess;
	PakPatchFuncs_s::PatchFunc_t patchFunc;

	// the total size of the pak file, returned by FS_OpenAsyncFile
	size_t fileSize;

	PakHandle_t pakId;
	JobID_t assetLoadJobId;
	int* loadedAssetIndices;
	uint8_t** memPageBuffers;

	PakPatchFileHeader_s* patchHeaders;
	unsigned short* patchIndices;

	char* streamingFilePaths[STREAMING_SET_COUNT];

	PakSegmentHeader_s* segmentHeaders;
	PakPageHeader_s* pageHeaders;

	PakPage_u* virtualPointers;
	PakAsset_s* assetEntries;

	PakPage_u* guidDescriptors;
	uint32_t* fileRelations;

	char gap5E0[32];

	PakPatchDataHeader_s* patchDataHeader;
	PakAsset_s** ppAssetEntries;

	int someAssetCount;
	int numShiftedPointers;

	// array of sizes/offsets in the SF_HEAD segment buffer
	__int64 unkAssetTypeBindingSizes[PAK_MAX_TRACKED_TYPES];

	const char* fileName;
	PakFileHeader_s pakHeader;
	PakPatchFileHeader_s patchHeader;
};

struct PakFile_s
{
	int numProcessedPointers;
	uint32_t processedAssetCount;
	int processedPageCount;
	int firstPageIdx;

	uint32_t patchCount;
	uint32_t dword14;

	PakFileStream_s fileStream;
	uint64_t inputBytePos;
	
	// the number of streamed files we've processed that are held
	// in the async file stream. if everything is finished, this
	// value will match PakFileStream_s::numLoadedFiles
	uint8_t processedStreamCount;

	char gap1F9[4];

	bool resetInBytePos;
	bool updateBytePosPostProcess;

	bool isCompressed;
	PakDecoder_s pakDecoder;
	uint8_t* decompBuffer;
	size_t maxCopySize;
	size_t headerSize; // Always sizeof(PakFileHeader_s)

	PakMemoryData_s memoryData;

	inline const char* GetName() const
	{
		return memoryData.fileName;
	}

	inline const PakFileHeader_s& GetHeader() const
	{
		return memoryData.pakHeader;
	}

	inline uint32_t GetAssetCount() const
	{
		return GetHeader().assetCount;
	}

	inline uint32_t GetPointerCount() const
	{
		return GetHeader().descriptorCount;
	}

	// --- pages ---
	inline uint16_t GetPageCount() const
	{
		return GetHeader().memPageCount;
	}

	inline bool IsPageOffsetValid(uint32_t index, uint32_t offset) const
	{
		// validate page index
		if (index == UINT32_MAX || index > GetPageCount())
			return false;

		// !TODO! find some other way of validating offset within page
		// commented because otherwise we incorrectly return false a lot.
		// because of pointer shifting, the offset can't be easily validated like this
		// 
		//if (offset == UINT32_MAX || && offset > GetPageSize(index))
		//	return false;

		return true;
	}

	inline const PakPageHeader_s* GetPageHeader(const uint32_t i) const
	{
		assert(i != UINT32_MAX && i < GetPageCount());

		return &memoryData.pageHeaders[i];
	}

	inline uint32_t GetPageSize(const uint32_t i) const
	{
		assert(i != UINT32_MAX && i < GetPageCount());
		return memoryData.pageHeaders[i].dataSize;
	}

	inline void* GetPointerForPageOffset(const uint32_t index, const uint32_t offset) const
	{
		assert(IsPageOffsetValid(index, offset));

		return memoryData.memPageBuffers[index] + offset;
	}

	inline void* GetPointerForPageOffset(const PakPage_u& ptr) const
	{
		assert(IsPageOffsetValid(ptr.index, ptr.offset));

		return memoryData.memPageBuffers[ptr.index] + ptr.offset;
	}

	inline void* GetPointerForPageOffset(const PakPage_u* ptr) const
	{
		assert(IsPageOffsetValid(ptr->index, ptr->offset));

		return memoryData.memPageBuffers[ptr->index] + ptr->offset;
	}

	// --- segments ---
	inline uint16_t GetSegmentCount() const
	{
		return GetHeader().virtualSegmentCount;
	}

	inline const PakSegmentHeader_s* GetSegmentHeader(const uint32_t i) const
	{
		assert(i < GetSegmentCount());

		return &memoryData.segmentHeaders[i];
	}
};


static_assert(sizeof(PakTracker_s) == 0x11D410);
static_assert(sizeof(PakFile_s) == 2224); // S3+
static_assert(sizeof(PakLoadedInfo_s) == 184);
static_assert(sizeof(PakDecoder_s) == 136);
static_assert(sizeof(PakPatchFileHeader_s) == 16);
static_assert(sizeof(PakPatchDataHeader_s) == 8);
static_assert(sizeof(PakGlobalState_s) == 0xC98A48);

#endif // RTECH_IPACKFILE_H
