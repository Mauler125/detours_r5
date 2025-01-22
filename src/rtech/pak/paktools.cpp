//=============================================================================//
//
// Purpose: general pak tools
//
//=============================================================================//
#include "tier0/binstream.h"
#include "rtech/ipakfile.h"

#include "pakstate.h"
#include "paktools.h"

//----------------------------------------------------------------------------------
// checks if the pak base path exists
//----------------------------------------------------------------------------------
bool Pak_BasePathExists()
{
	return IsDirectory(PAK_PLATFORM_PATH);
}

//----------------------------------------------------------------------------------
// creates the pak base path if it doesn't exists
//----------------------------------------------------------------------------------
bool Pak_CreateBasePath()
{
	// already exists
	if (Pak_BasePathExists())
		return true;

	return CreateDirHierarchy(PAK_PLATFORM_PATH) == 0;
}

//----------------------------------------------------------------------------------
// checks if the pak override path exists
//----------------------------------------------------------------------------------
bool Pak_OverridePathExists()
{
	return IsDirectory(PAK_PLATFORM_OVERRIDE_PATH);
}

//----------------------------------------------------------------------------------
// creates the pak override path if it doesn't exists
//----------------------------------------------------------------------------------
bool Pak_CreateOverridePath()
{
	// already exists
	if (Pak_OverridePathExists())
		return true;

	return CreateDirHierarchy(PAK_PLATFORM_OVERRIDE_PATH) == 0;
}

//----------------------------------------------------------------------------------
// checks if an override file exists and returns whether it should be loaded instead
//----------------------------------------------------------------------------------
bool Pak_FileOverrideExists(const char* const pakFilePath, char* const outPath, const size_t outBufLen)
{
    // check the overrides path
    snprintf(outPath, outBufLen, PAK_PLATFORM_OVERRIDE_PATH"%s", V_UnqualifiedFileName(pakFilePath));
    return FileExists(outPath);
}

//----------------------------------------------------------------------------------
// checks if a pak file exists
//----------------------------------------------------------------------------------
int Pak_FileExists(const char* const pakFilePath)
{
    char fullPath[1024];
    if (Pak_FileOverrideExists(pakFilePath, fullPath, sizeof(fullPath)))
        return true;

    // check the platform's default path
    snprintf(fullPath, sizeof(fullPath), PAK_PLATFORM_PATH"%s", pakFilePath);

    return FileExists(fullPath);
}

//-----------------------------------------------------------------------------
// returns pak status as string
//-----------------------------------------------------------------------------
const char* Pak_StatusToString(const PakStatus_e status)
{
	switch (status)
	{
	case PakStatus_e::PAK_STATUS_FREED:                  return "PAK_STATUS_FREED";
	case PakStatus_e::PAK_STATUS_LOAD_PENDING:           return "PAK_STATUS_LOAD_PENDING";
	case PakStatus_e::PAK_STATUS_REPAK_RUNNING:          return "PAK_STATUS_REPAK_RUNNING";
	case PakStatus_e::PAK_STATUS_REPAK_DONE:             return "PAK_STATUS_REPAK_DONE";
	case PakStatus_e::PAK_STATUS_LOAD_STARTING:          return "PAK_STATUS_LOAD_STARTING";
	case PakStatus_e::PAK_STATUS_LOAD_PAKHDR:            return "PAK_STATUS_LOAD_PAKHDR";
	case PakStatus_e::PAK_STATUS_LOAD_PATCH_INIT:        return "PAK_STATUS_LOAD_PATCH_INIT";
	case PakStatus_e::PAK_STATUS_LOAD_PATCH_EDIT_STREAM: return "PAK_STATUS_LOAD_PATCH_EDIT_STREAM";
	case PakStatus_e::PAK_STATUS_LOAD_ASSETS:            return "PAK_STATUS_LOAD_ASSETS";
	case PakStatus_e::PAK_STATUS_LOADED:                 return "PAK_STATUS_LOADED";
	case PakStatus_e::PAK_STATUS_UNLOAD_PENDING:         return "PAK_STATUS_UNLOAD_PENDING";
	case PakStatus_e::PAK_STATUS_FREE_PENDING:           return "PAK_STATUS_FREE_PENDING";
	case PakStatus_e::PAK_STATUS_CANCELING:              return "PAK_STATUS_CANCELING";
	case PakStatus_e::PAK_STATUS_ERROR:                  return "PAK_STATUS_ERROR";
	case PakStatus_e::PAK_STATUS_INVALID_PAKHANDLE:      return "PAK_STATUS_INVALID_PAKHANDLE";
	case PakStatus_e::PAK_STATUS_BUSY:                   return "PAK_STATUS_BUSY";
	default:                                            return "PAK_STATUS_UNKNOWN";
	}
}

//-----------------------------------------------------------------------------
// returns pak decoder as string
//-----------------------------------------------------------------------------
const char* Pak_DecoderToString(const PakDecodeMode_e mode)
{
	switch (mode)
	{
	case PakDecodeMode_e::MODE_RTECH: return "RTech";
	case PakDecodeMode_e::MODE_ZSTD: return "ZStd";
	}

	UNREACHABLE();
}

//-----------------------------------------------------------------------------
// compute a guid from input string data that resides in a memory address that
// is aligned to a 4 byte boundary
//-----------------------------------------------------------------------------
static PakGuid_t Pak_StringToGuidAligned(const char* string)
{
	uint64_t         v1; // r9
	int               i; // r11d
	uint32_t         v4; // edi
	int              v5; // ebp
	int              v6; // r10d
	uint32_t         v7; // ecx
	uint32_t         v8; // edx
	uint32_t         v9; // eax
	uint32_t        v10; // r8d
	int64_t         v11; // r10
	uint64_t        v12; // r8
	int             v13; // eax
	int             v15; // ecx

	v1 = 0ull;
	for (i = 0; ; i += 4)
	{
		v4 = ~*(_DWORD*)string & (*(_DWORD*)string - 0x1010101) & 0x80808080;
		v5 = v4 ^ (v4 - 1);
		v6 = v5 & *(_DWORD*)string ^ 0x5C5C5C5C;
		v7 = ~v6 & (v6 - 0x1010101) & 0x80808080;
		v8 = v7 & -(int32_t)v7;
		if (v7 != v8)
		{
			v9 = 0xFF000000;
			do
			{
				v10 = v9;
				if ((v9 & v6) == 0)
					v8 |= v9 & 0x80808080;
				v9 >>= 8;
			} while (v10 >= 0x100);
		}
		v11 = 0x633D5F1 * v1;
		v12 = (0xFB8C4D96501i64 * (uint64_t)(((v5 & *(_DWORD*)string) - 45 * (v8 >> 7)) & 0xDFDFDFDF)) >> 24;
		if (v4)
			break;
		string += 4;
		v1 = ((v11 + v12) >> 61) ^ (v11 + v12);
	}
	v13 = -1;
	if (_BitScanReverse((unsigned long*)&v15, v5))
		v13 = v15;
	return v12 + v11 - 0xAE502812AA7333i64 * (uint32_t)(i + v13 / 8);
}

//-----------------------------------------------------------------------------
// compute a guid from input string data that resides in a memory address that
// isn't aligned to a 4 byte boundary
//-----------------------------------------------------------------------------
static PakGuid_t Pak_StringToGuidUnaligned(const char* string)
{
	uint64_t        v1; // rbx
	uint64_t        v2; // r10
	int              i; // esi
	int             v4; // edx
	uint32_t        v5; // edi
	int             v6; // ebp
	int             v7; // edx
	uint32_t        v8; // ecx
	uint32_t        v9; // r8d
	uint32_t       v10; // eax
	uint32_t       v11; // r9d
	int64_t        v12; // r9
	uint64_t       v13; // r8
	int            v14; // eax
	int            v16; // ecx

	v1 = 0ull;
	v2 = (uint64_t)(string + 3);
	for (i = 0; ; i += 4)
	{
		if ((v2 ^ (v2 - 3)) >= 0x1000)
		{
			v4 = *(uint8_t*)(v2 - 3);
			if ((_BYTE)v4)
			{
				v4 = *(uint16_t*)(v2 - 3);
				if (*(_BYTE*)(v2 - 2))
				{
					v4 |= *(uint8_t*)(v2 - 1) << 16;
					if (*(_BYTE*)(v2 - 1))
						v4 |= *(uint8_t*)v2 << 24;
				}
			}
		}
		else
		{
			v4 = *(_DWORD*)(v2 - 3);
		}
		v5 = ~v4 & (v4 - 0x1010101) & 0x80808080;
		v6 = v5 ^ (v5 - 1);
		v7 = v6 & v4;
		v8 = ~(v7 ^ 0x5C5C5C5C) & ((v7 ^ 0x5C5C5C5C) - 0x1010101) & 0x80808080;
		v9 = v8 & -(int32_t)v8;
		if (v8 != v9)
		{
			v10 = 0xFF000000;
			do
			{
				v11 = v10;
				if ((v10 & (v7 ^ 0x5C5C5C5C)) == 0)
					v9 |= v10 & 0x80808080;
				v10 >>= 8;
			} while (v11 >= 0x100);
		}
		v12 = 0x633D5F1 * v1;
		v13 = (0xFB8C4D96501i64 * (uint64_t)((v7 - 45 * (v9 >> 7)) & 0xDFDFDFDF)) >> 24;
		if (v5)
			break;
		v2 += 4i64;
		v1 = ((v12 + v13) >> 61) ^ (v12 + v13);
	}
	v14 = -1;
	if (_BitScanReverse((unsigned long*)&v16, v6))
		v14 = v16;
	return v13 + v12 - 0xAE502812AA7333i64 * (uint32_t)(i + v14 / 8);
}

//-----------------------------------------------------------------------------
// compute a guid from input string data
//-----------------------------------------------------------------------------
PakGuid_t Pak_StringToGuid(const char* const string)
{
	return ((uintptr_t)string & 3)
		? Pak_StringToGuidUnaligned(string)
		: Pak_StringToGuidAligned(string);
}

//-----------------------------------------------------------------------------
// gets information about loaded pak file via pak id
//-----------------------------------------------------------------------------
PakLoadedInfo_s* Pak_GetPakInfo(const PakHandle_t pakId)
{
	return &g_pakGlobals->loadedPaks[pakId & PAK_MAX_LOADED_PAKS_MASK];
}

//-----------------------------------------------------------------------------
// gets information about loaded pak file via pak name
//-----------------------------------------------------------------------------
const PakLoadedInfo_s* Pak_GetPakInfo(const char* const pakName)
{
	// a pak under the same name can be loaded more than once, the hotswap
	// system just switches the asset pointers to the new 'live' pak if
	// there are assets that overlap the ones in the previous pak. we want
	// to find the most recently loaded pak under the provided name and
	// return that to the caller. the higher the handle, the newer it is.
	PakHandle_t highestHandle = PAK_INVALID_HANDLE;

	for (uint16_t i = 0; i < PAK_MAX_LOADED_PAKS; ++i)
	{
		const PakLoadedInfo_s* const info = &g_pakGlobals->loadedPaks[i];
		if (!info)
			continue;

		if (!info->fileName || !*info->fileName)
			continue;

		if (strcmp(pakName, info->fileName) != 0)
			continue;

		if (info->handle > highestHandle)
			highestHandle = info->handle;
	}

	if (highestHandle == PAK_INVALID_HANDLE)
	{
		DevWarning(eDLL_T::RTECH, "%s - Failed to retrieve pak info for name '%s'\n", __FUNCTION__, pakName);
		return nullptr;
	}

	return Pak_GetPakInfo(highestHandle);
}

//-----------------------------------------------------------------------------
// returns a pointer to the patch data header
//-----------------------------------------------------------------------------
PakPatchDataHeader_s* Pak_GetPatchDataHeader(PakFileHeader_s* const pakHeader)
{
	// shouldn't be called if the pak doesn't have patches!
	assert(pakHeader->patchIndex > 0);
	return reinterpret_cast<PakPatchDataHeader_s*>(reinterpret_cast<uint8_t* const>(pakHeader) + sizeof(PakFileHeader_s));
}

//-----------------------------------------------------------------------------
// returns a pointer to the patch file header
//-----------------------------------------------------------------------------
PakPatchFileHeader_s* Pak_GetPatchFileHeader(PakFileHeader_s* const pakHeader, const int index)
{
	assert(pakHeader->patchIndex > 0 && index < pakHeader->patchIndex);
	uint8_t* address = reinterpret_cast<uint8_t* const>(pakHeader);

	// skip the file header
	address += sizeof(PakFileHeader_s);

	// skip the patch data header, the patch file headers start from there
	address += sizeof(PakPatchDataHeader_s);
	PakPatchFileHeader_s* const patchHeaders = reinterpret_cast<PakPatchFileHeader_s* const>(address);

	return &patchHeaders[index];
}

//-----------------------------------------------------------------------------
// returns the patch number belonging to the index provided
//-----------------------------------------------------------------------------
short Pak_GetPatchNumberForIndex(PakFileHeader_s* const pakHeader, const int index)
{
	assert(pakHeader->patchIndex > 0 && index < pakHeader->patchIndex);
	const uint8_t* patchHeader = reinterpret_cast<const uint8_t*>(Pak_GetPatchFileHeader(pakHeader, pakHeader->patchIndex - 1));

	// skip the last patch file header, the patch number start from there
	patchHeader += sizeof(PakPatchFileHeader_s);
	const short* patchNumber = reinterpret_cast<const short*>(patchHeader);

	return patchNumber[index];
}

//-----------------------------------------------------------------------------
// searches for pak patches and updates all referenced files in patch headers
//-----------------------------------------------------------------------------
bool Pak_UpdatePatchHeaders(uint8_t* const inBuf, const char* const outPakFile)
{
	// file name without extension and patch number ID
	char baseFilePath[MAX_PATH];
	snprintf(baseFilePath, sizeof(baseFilePath), "%s", outPakFile);

	const char* const fileNameUnqualified = V_UnqualifiedFileName(baseFilePath);

	V_StripExtension(baseFilePath, baseFilePath, sizeof(baseFilePath));

	// strip the patch id, but make sure we only do this on the file name
	// and not the path
	char* const patchIdentifier = strrchr(baseFilePath, '(');

	if (patchIdentifier && patchIdentifier > fileNameUnqualified)
		*patchIdentifier = '\0';

	// NOTE: we modify the in buffer as the patch headers belong to the
	// compressed section
	PakFileHeader_s* const inHeader = reinterpret_cast<PakFileHeader_s* const>(inBuf);

	// update each patch header
	for (uint16_t i = 0; i < inHeader->patchIndex; i++)
	{
		short patchNumber = Pak_GetPatchNumberForIndex(inHeader, i);
		char patchFile[MAX_PATH];

		// the first patch number does not have an identifier in its name
		if (patchNumber == 0)
			snprintf(patchFile, sizeof(patchFile), "%s.rpak", baseFilePath);
		else
			snprintf(patchFile, sizeof(patchFile), "%s(%02u).rpak", baseFilePath, patchNumber);

		CIOStream inPatch;

		// unable to open patch while there should be one, we must calculate
		// new file sizes here, or else the runtime would fail to load them
		if (!inPatch.Open(patchFile, CIOStream::Mode_e::Read))
			return false;

		const size_t fileSize = inPatch.GetSize();

		// pak appears truncated
		if (fileSize <= sizeof(PakFileHeader_s))
			return false;

		DevMsg(eDLL_T::RTECH, "%s: updating patch header for pak '%s', new size = %zu\n",
			__FUNCTION__, patchFile, fileSize);

		PakPatchFileHeader_s* const patchHeader = Pak_GetPatchFileHeader(inHeader, i);
		patchHeader->compressedSize = fileSize;
	}

	return true;
}

//-----------------------------------------------------------------------------
// prints the pak header details to the console
//-----------------------------------------------------------------------------
void Pak_ShowHeaderDetails(const PakFileHeader_s* const pakHeader)
{
	SYSTEMTIME systemTime;
	FileTimeToSystemTime(&pakHeader->fileTime, &systemTime);

	Msg(eDLL_T::RTECH, "______________________________________________________________\n");
	Msg(eDLL_T::RTECH, "-+ Pak header details ----------------------------------------\n");
	Msg(eDLL_T::RTECH, " |-- Magic    : '0x%08X'\n", pakHeader->magic);
	Msg(eDLL_T::RTECH, " |-- Version  : '%hu'\n", pakHeader->version);
	Msg(eDLL_T::RTECH, " |-- Flags    : '0x%04hX'\n", pakHeader->flags);
	Msg(eDLL_T::RTECH, " |-- Time     : '%hu-%hu-%hu/%hu %hu:%hu:%hu.%hu'\n",
		systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wDayOfWeek,
		systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
	Msg(eDLL_T::RTECH, " |-- Checksum : '0x%08llX'\n", pakHeader->checksum);
	Msg(eDLL_T::RTECH, " |-- Assets   : '%u'\n", pakHeader->assetCount);
	Msg(eDLL_T::RTECH, " |-+ Compression ---------------------------------------------\n");
	Msg(eDLL_T::RTECH, " | |-- Size comp: '%zu'\n", pakHeader->compressedSize);
	Msg(eDLL_T::RTECH, " | |-- Size decp: '%zu'\n", pakHeader->decompressedSize);
	Msg(eDLL_T::RTECH, " | |-- Ratio    : '%.02f'\n", (pakHeader->compressedSize * 100.f) / pakHeader->decompressedSize);
	Msg(eDLL_T::RTECH, "--------------------------------------------------------------\n");
}
