//=============================================================================//
//
// Purpose: pak runtime memory and management
//
//=============================================================================//
#include "tier1/fmtstr.h"
#include "common/completion.h"
#include "rtech/ipakfile.h"
#include "pakencode.h"
#include "pakdecode.h"
#include "paktools.h"
#include "pakstate.h"
/*
=====================
Pak_ListPaks_f
=====================
*/
static void Pak_ListPaks_f()
{
	Msg(eDLL_T::RTECH, "| id   | name                                               | status                               | asset count |\n");
	Msg(eDLL_T::RTECH, "|------|----------------------------------------------------|--------------------------------------|-------------|\n");

	uint32_t nTotalLoaded = 0;

	for (int16_t i = 0, n = g_pakGlobals->loadedPakCount; i < n; ++i)
	{
		const PakLoadedInfo_s& info = g_pakGlobals->loadedPaks[i];

		if (info.status == PakStatus_e::PAK_STATUS_FREED)
			continue;

		const char* szRpakStatus = Pak_StatusToString(info.status);

		// todo: make status into a string from an array/vector
		Msg(eDLL_T::RTECH, "| %04i | %-50s | %-36s | %11i |\n", info.handle, info.fileName, szRpakStatus, info.assetCount);
		nTotalLoaded++;
	}
	Msg(eDLL_T::RTECH, "|------|----------------------------------------------------|--------------------------------------|-------------|\n");
	Msg(eDLL_T::RTECH, "| %18i loaded paks.                                                                                |\n", nTotalLoaded);
	Msg(eDLL_T::RTECH, "|------|----------------------------------------------------|--------------------------------------|-------------|\n");
}

/*
=====================
Pak_ListTypes_f
=====================
*/
static void Pak_ListTypes_f()
{
	Msg(eDLL_T::RTECH, "| ext  | description               | version | header size | native size |\n");
	Msg(eDLL_T::RTECH, "|------|---------------------------|---------|-------------|-------------|\n");

	uint32_t nRegistered = 0;

	for (int8_t i = 0; i < PAK_MAX_TRACKED_TYPES; ++i)
	{
		PakAssetBinding_s* type = &g_pakGlobals->assetBindings[i];

		if (!type->description)
			continue;

		FourCCString_t assetExtension;
		FourCCToString(assetExtension, type->extension);

		Msg(eDLL_T::RTECH, "| %-4s | %-25s | %7i | %11i | %11i |\n", assetExtension, type->description, type->version, type->headerSize, type->nativeClassSize);
		nRegistered++;
	}
	Msg(eDLL_T::RTECH, "|------|---------------------------|---------|-------------|-------------|\n");
	Msg(eDLL_T::RTECH, "| %18i registered types.                                   |\n", nRegistered);
	Msg(eDLL_T::RTECH, "|------|---------------------------|---------|-------------|-------------|\n");
}

/*
=====================
Pak_RequestUnload_f
=====================
*/
static void Pak_RequestUnload_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	if (args.HasOnlyDigits(1))
	{
		const PakHandle_t pakHandle = atoi(args.Arg(1));
		const PakLoadedInfo_s* const pakInfo = Pak_GetPakInfo(pakHandle);

		if (!pakInfo)
		{
			Warning(eDLL_T::RTECH, "Found no pak entry for specified handle.\n");
			return;
		}

		Msg(eDLL_T::RTECH, "Requested pak unload for handle '%d'\n", pakHandle);
		g_pakLoadApi->UnloadAsync(pakHandle);
	}
	else
	{
		const PakLoadedInfo_s* const pakInfo = Pak_GetPakInfo(args.Arg(1));
		if (!pakInfo)
		{
			Warning(eDLL_T::RTECH, "Found no pak entry for specified name.\n");
			return;
		}

		Msg(eDLL_T::RTECH, "Requested pak unload for file '%s'\n", args.Arg(1));
		g_pakLoadApi->UnloadAsync(pakInfo->handle);
	}
}

/*
=====================
Pak_RequestLoad_f
=====================
*/
static void Pak_RequestLoad_f(const CCommand& args)
{
	g_pakLoadApi->LoadAsync(args.Arg(1), AlignedMemAlloc(), 1, 0);
}


/*
=====================
Pak_Swap_f
=====================
*/
static void Pak_Swap_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	const char* pakName = nullptr;

	PakHandle_t pakHandle = PAK_INVALID_HANDLE;
	const PakLoadedInfo_s* pakInfo = nullptr;

	if (args.HasOnlyDigits(1))
	{
		pakHandle = atoi(args.Arg(1));
		pakInfo = Pak_GetPakInfo(pakHandle);

		if (!pakInfo)
		{
			Warning(eDLL_T::RTECH, "Found no pak entry for specified handle.\n");
			return;
		}

		pakName = pakInfo->fileName;
	}
	else
	{
		pakName = args.Arg(1);
		pakInfo = Pak_GetPakInfo(pakName);

		if (!pakInfo)
		{
			Warning(eDLL_T::RTECH, "Found no pak entry for specified name.\n");
			return;
		}

		pakHandle = pakInfo->handle;
	}

	Msg(eDLL_T::RTECH, "Requested pak swap for handle '%d'\n", pakHandle);
	g_pakLoadApi->UnloadAsyncAndWait(pakHandle); // Wait till this slot gets free'd.

	g_pakLoadApi->LoadAsync(pakName, AlignedMemAlloc(), pakInfo->logChannel, 0);
}

/*
=====================
RTech_StringToGUID_f
=====================
*/
static void Pak_StringToGUID_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	unsigned long long guid = Pak_StringToGuid(args.Arg(1));

	Msg(eDLL_T::RTECH, "______________________________________________________________\n");
	Msg(eDLL_T::RTECH, "] RTECH_HASH ]------------------------------------------------\n");
	Msg(eDLL_T::RTECH, "] GUID: '0x%llX'\n", guid);
}

/*
=====================
RTech_Decompress_f

  Decompresses input RPak file and
  dumps results to override path
=====================
*/
static void Pak_Decompress_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	CFmtStr1024 inPakFile(PAK_PLATFORM_PATH "%s", args.Arg(1));
	CFmtStr1024 outPakFile(PAK_PLATFORM_OVERRIDE_PATH "%s", args.Arg(1));

	if (!Pak_DecodePakFile(inPakFile.String(), outPakFile.String()))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - decompression failed for '%s'!\n",
			__FUNCTION__, inPakFile.String());
	}
}

/*
=====================
RTech_Compress_f

  Compresses input RPak file and
  dumps results to base path
=====================
*/
static void Pak_Compress_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	CFmtStr1024 inPakFile(PAK_PLATFORM_OVERRIDE_PATH "%s", args.Arg(1));
	CFmtStr1024 outPakFile(PAK_PLATFORM_PATH "%s", args.Arg(1));

	// NULL means default compress level
	const int compressLevel = args.ArgC() > 2 ? atoi(args.Arg(2)) : NULL;

	if (!Pak_EncodePakFile(inPakFile.String(), outPakFile.String(), compressLevel))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - compression failed for '%s'!\n",
			__FUNCTION__, inPakFile.String());
	}
}

static ConCommand pak_stringtoguid("pak_stringtoguid", Pak_StringToGUID_f, "Calculates the GUID from input text", FCVAR_DEVELOPMENTONLY);

static ConCommand pak_compress("pak_compress", Pak_Compress_f, "Compresses specified RPAK file", FCVAR_DEVELOPMENTONLY, RTech_PakCompress_f_CompletionFunc);
static ConCommand pak_decompress("pak_decompress", Pak_Decompress_f, "Decompresses specified RPAK file", FCVAR_DEVELOPMENTONLY, RTech_PakDecompress_f_CompletionFunc);

static ConCommand pak_requestload("pak_requestload", Pak_RequestLoad_f, "Requests asynchronous load for specified RPAK file", FCVAR_DEVELOPMENTONLY, RTech_PakLoad_f_CompletionFunc);
static ConCommand pak_requestunload("pak_requestunload", Pak_RequestUnload_f, "Requests unload for specified RPAK file or ID", FCVAR_DEVELOPMENTONLY, RTech_PakUnload_f_CompletionFunc);

static ConCommand pak_swap("pak_swap", Pak_Swap_f, "Requests swap for specified RPAK file or ID", FCVAR_DEVELOPMENTONLY);

static ConCommand pak_listpaks("pak_listpaks", Pak_ListPaks_f, "Display a list of the loaded Pak files", FCVAR_RELEASE);
static ConCommand pak_listtypes("pak_listtypes", Pak_ListTypes_f, "Display a list of the registered asset types", FCVAR_RELEASE);


// Symbols taken from R2 dll's.
PakLoadFuncs_s* g_pakLoadApi = nullptr;
