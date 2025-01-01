//=============================================================================//
//
// Purpose: buffered pak encoder
//
//=============================================================================//
#include "tier0/binstream.h"
#include "rtech/ipakfile.h"
#include "paktools.h"
#include "pakencode.h"

//-----------------------------------------------------------------------------
// determines whether encoding had failed
//-----------------------------------------------------------------------------
static bool Pak_HasEncodeFailed(const size_t result)
{
	return static_cast<bool>(ZSTD_isError(result));
}

//-----------------------------------------------------------------------------
// returns the encode error as string
//-----------------------------------------------------------------------------
static const char* Pak_GetEncodeError(const size_t result)
{
	return ZSTD_getErrorName(result);
}

//-----------------------------------------------------------------------------
// encodes the pak file from buffer, we can't do streamed compression as we
// need to know the actual decompress size ahead of time, else the runtime will
// fail as we wouldn't be able to parse the decompressed size from the frame
// header
//-----------------------------------------------------------------------------
bool Pak_BufferToBufferEncode(const uint8_t* const inBuf, const uint64_t inLen,
	uint8_t* const outBuf, const uint64_t outLen, const int level)
{
	// offset to the actual pak data, the main file header shouldn't be
	// compressed
	const size_t dataOffset = sizeof(PakFileHeader_s);

	uint8_t* const dstBuf = outBuf + dataOffset;
	const size_t dstLen = outLen - dataOffset;

	const uint8_t* const srcBuf = inBuf + dataOffset;
	const size_t srcLen = inLen - dataOffset;

	size_t compressSize = NULL;

	compressSize = ZSTD_compress(dstBuf, dstLen, srcBuf, srcLen, level);

	if (Pak_HasEncodeFailed(compressSize))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: compression failed! [%s]\n",
			__FUNCTION__, Pak_GetEncodeError(compressSize));

		return false;
	}

	PakFileHeader_s* const outHeader = reinterpret_cast<PakFileHeader_s* const>(outBuf);

	// the compressed size includes the entire buffer, even the data we didn't
	// compress like the file header
	outHeader->compressedSize = compressSize + dataOffset;

	// these flags are required for the game's runtime to decide whether or not to
	// decompress the pak, and how; see Pak_ProcessPakFile() for more details
	outHeader->flags |= PAK_HEADER_FLAGS_COMPRESSED;
	outHeader->flags |= PAK_HEADER_FLAGS_ZSTREAM_ENCODED;

	return true;
}

//-----------------------------------------------------------------------------
// encodes the pak file from file name
//-----------------------------------------------------------------------------
bool Pak_EncodePakFile(const char* const inPakFile, const char* const outPakFile, const int level)
{
	if (!Pak_CreateBasePath())
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: failed to create output path for pak file '%s'!\n",
			__FUNCTION__, outPakFile);

		return false;
	}

	CIOStream inPakStream;

	if (!inPakStream.Open(inPakFile, CIOStream::Mode_e::Read))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: failed to open pak file '%s' for read!\n",
			__FUNCTION__, inPakFile);

		return false;
	}

	CIOStream outPakStream;

	if (!outPakStream.Open(outPakFile, CIOStream::Mode_e::Write))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: failed to open pak file '%s' for write!\n",
			__FUNCTION__, outPakFile);

		return false;
	}

	const size_t fileSize = inPakStream.GetSize();

	// file appears truncated
	if (fileSize <= sizeof(PakFileHeader_s))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: pak '%s' appears truncated!\n",
			__FUNCTION__, inPakFile);

		return false;
	}

	std::unique_ptr<uint8_t[]> inPakBufContainer(new uint8_t[fileSize]);
	uint8_t* const inPakBuf = inPakBufContainer.get();

	inPakStream.Read(inPakBuf, fileSize);
	inPakStream.Close();

	const PakFileHeader_s* const inHeader = reinterpret_cast<PakFileHeader_s* const>(inPakBuf);

	if (inHeader->magic != PAK_HEADER_MAGIC || inHeader->version != PAK_HEADER_VERSION)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: pak '%s' has incompatible or invalid header!\n",
			__FUNCTION__, inPakFile);

		return false;
	}

	if (inHeader->GetCompressionMode() != PakDecodeMode_e::MODE_DISABLED)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: pak '%s' is already compressed!\n",
			__FUNCTION__, inPakFile);

		return false;
	}

	if (inHeader->compressedSize != fileSize)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: pak '%s' appears truncated or corrupt; compressed size: '%zu' expected: '%zu'!\n",
			__FUNCTION__, inPakFile, fileSize, inHeader->compressedSize);

		return false;
	}

	// NOTE: if the paks this particular pak patches have different sizes than
	// current sizes in the patch header, the runtime will crash!
	if (inHeader->patchIndex && !Pak_UpdatePatchHeaders(inPakBuf, outPakFile))
	{
		Warning(eDLL_T::RTECH, "%s: pak '%s' is a patch pak, but the pak(s) it patches weren't found; patch headers not updated!\n",
			__FUNCTION__, inPakFile);
	}

	const size_t outBufSize = inHeader->decompressedSize;

	std::unique_ptr<uint8_t[]> outPakBufContainer(new uint8_t[outBufSize]);
	uint8_t* const outPakBuf = outPakBufContainer.get();

	PakFileHeader_s* const outHeader = reinterpret_cast<PakFileHeader_s* const>(outPakBuf);

	// copy the header over
	*outHeader = *inHeader;

	// encoding failed
	if (!Pak_BufferToBufferEncode(inPakBuf, fileSize, outPakBuf, outBufSize, level))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: failed to compress pak file '%s'!\n",
			__FUNCTION__, inPakFile);

		return false;
	}

	const PakFileHeader_s* const outPakHeader = reinterpret_cast<PakFileHeader_s* const>(outPakBuf);

	Pak_ShowHeaderDetails(outPakHeader);

	// this will be true if the entire buffer has been written
	outPakStream.Write(outPakBuf, outPakHeader->compressedSize);

	Msg(eDLL_T::RTECH, "Compressed pak file to: '%s'\n", outPakFile);
	return true;
}
