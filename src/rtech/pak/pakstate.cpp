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

	uint32_t numLoaded = 0;

	for (uint16_t i = 0, n = PAK_MAX_LOADED_PAKS; i < n; ++i)
	{
		const PakLoadedInfo_s& info = g_pakGlobals->loadedPaks[i];

		if (info.status == PakStatus_e::PAK_STATUS_FREED)
			continue;

		const char* const pakStatus = Pak_StatusToString(info.status);

		Msg(eDLL_T::RTECH, "| %04i | %-50s | %-36s | %11u |\n", info.handle, info.fileName, pakStatus, info.assetCount);
		numLoaded++;
	}
	Msg(eDLL_T::RTECH, "|------|----------------------------------------------------|--------------------------------------|-------------|\n");
	Msg(eDLL_T::RTECH, "| %18u loaded paks.                                                                                |\n", numLoaded);
	Msg(eDLL_T::RTECH, "|------|----------------------------------------------------|--------------------------------------|-------------|\n");
}

/*
=====================
Pak_ListTypes_f
=====================
*/
static void Pak_ListTypes_f()
{
	Msg(eDLL_T::RTECH, "| ext  | description               | version | alignment | header size | struct size |\n");
	Msg(eDLL_T::RTECH, "|------|---------------------------|---------|-----------|-------------|-------------|\n");

	uint32_t numRegistered = 0;

	for (uint8_t i = 0; i < PAK_MAX_TRACKED_TYPES; ++i)
	{
		const PakAssetBinding_s& type = g_pakGlobals->assetBindings[i];

		if (type.type == PakAssetBinding_s::EType::NONE || type.type == PakAssetBinding_s::EType::STUB)
			continue;

		FourCCString_t assetExtension;
		FourCCToString(assetExtension, type.extension);

		Msg(eDLL_T::RTECH, "| %-4s | %-25s | %7u | %9u | %11u | %11u |\n", 
			assetExtension, type.description, type.version, type.headerAlignment, type.headerSize, type.structSize);

		numRegistered++;
	}
	Msg(eDLL_T::RTECH, "|------|---------------------------|---------|-----------|-------------|-------------|\n");
	Msg(eDLL_T::RTECH, "| %18u registered types.                                               |\n", numRegistered);
	Msg(eDLL_T::RTECH, "|------|---------------------------|---------|-----------|-------------|-------------|\n");
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

	const PakLoadedInfo_s* pakInfo = nullptr;

	if (args.HasOnlyDigits(1))
	{
		const PakHandle_t pakHandle = atoi(args.Arg(1));
		pakInfo = Pak_GetPakInfo(pakHandle);

		if (pakInfo->status != PAK_STATUS_LOADED)
		{
			Warning(eDLL_T::RTECH, "Pak with handle '%d' is currently unavailable; status '%s', cannot unload\n",
				pakHandle, Pak_StatusToString(pakInfo->status));
			return;
		}
	}
	else
	{
		const char* const pakName = args.Arg(1);
		pakInfo = Pak_GetPakInfo(pakName);

		if (!pakInfo)
		{
			Warning(eDLL_T::RTECH, "Pak with name '%s' not loaded, cannot unload\n", pakName);
			return;
		}
		else if (pakInfo->status != PAK_STATUS_LOADED)
		{
			Warning(eDLL_T::RTECH, "Pak with name '%s' is currently unavailable; status '%s', cannot unload\n",
				pakName, Pak_StatusToString(pakInfo->status));
			return;
		}
	}

	Msg(eDLL_T::RTECH, "Requested pak unload for file '%s' with handle '%d'\n", pakInfo->fileName, pakInfo->handle);
	g_pakLoadApi->UnloadAsync(pakInfo->handle);
}

/*
=====================
Pak_RequestLoad_f
=====================
*/
static void Pak_RequestLoad_f(const CCommand& args)
{
	const char* const pakFile = args.Arg(1);

	Msg(eDLL_T::RTECH, "Requested pak load for file '%s'\n", pakFile);
	g_pakLoadApi->LoadAsync(pakFile, AlignedMemAlloc(), 1, 0);
}

/*
=====================
Pak_RequestSwap_f
=====================
*/
static void Pak_RequestSwap_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	const PakLoadedInfo_s* pakInfo = nullptr;

	if (args.HasOnlyDigits(1))
	{
		const PakHandle_t pakHandle = atoi(args.Arg(1));
		pakInfo = Pak_GetPakInfo(pakHandle);

		if (pakInfo->status != PAK_STATUS_LOADED)
		{
			Warning(eDLL_T::RTECH, "Pak with handle '%d' is currently unavailable; status '%s', cannot swap\n",
				pakHandle, Pak_StatusToString(pakInfo->status));
			return;
		}
	}
	else
	{
		const char* const pakName = args.Arg(1);
		pakInfo = Pak_GetPakInfo(pakName);

		if (!pakInfo)
		{
			Warning(eDLL_T::RTECH, "Pak with name '%s' not loaded, cannot swap\n", pakName);
			return;
		}
		else if (pakInfo->status != PAK_STATUS_LOADED)
		{
			Warning(eDLL_T::RTECH, "Pak with name '%s' is currently unavailable; status '%s', cannot swap\n",
				pakName, Pak_StatusToString(pakInfo->status));
			return;
		}
	}

	Msg(eDLL_T::RTECH, "Requested pak swap for file '%s' with handle '%d'\n", pakInfo->fileName, pakInfo->handle);

	g_pakLoadApi->UnloadAsyncAndWait(pakInfo->handle); // Wait till this slot gets free'd.
	g_pakLoadApi->LoadAsync(pakInfo->fileName, AlignedMemAlloc(), pakInfo->logChannel, pakInfo->unkAC);
}

/*
=====================
Pak_StringToGUID_f
=====================
*/
static void Pak_StringToGUID_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	const PakGuid_t guid = Pak_StringToGuid(args.Arg(1));

	Msg(eDLL_T::RTECH, "______________________________________________________________\n");
	Msg(eDLL_T::RTECH, "] RTECH_HASH ]------------------------------------------------\n");
	Msg(eDLL_T::RTECH, "] GUID: '0x%llX'\n", guid);
}

/*
=====================
Pak_Decompress_f

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

	const CFmtStr1024 inPakFile(PAK_PLATFORM_PATH "%s", args.Arg(1));
	const CFmtStr1024 outPakFile(PAK_PLATFORM_OVERRIDE_PATH "%s", args.Arg(1));

	if (!Pak_DecodePakFile(inPakFile.String(), outPakFile.String()))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - decompression failed for '%s'!\n",
			__FUNCTION__, inPakFile.String());
	}
}

/*
=====================
Pak_Compress_f

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

	const CFmtStr1024 inPakFile(PAK_PLATFORM_OVERRIDE_PATH "%s", args.Arg(1));
	const CFmtStr1024 outPakFile(PAK_PLATFORM_PATH "%s", args.Arg(1));

	// NULL means default compress level
	const int compressLevel = args.ArgC() > 2 ? atoi(args.Arg(2)) : NULL;

	if (!Pak_EncodePakFile(inPakFile.String(), outPakFile.String(), compressLevel))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - compression failed for '%s'!\n",
			__FUNCTION__, inPakFile.String());
	}
}

static ConCommand pak_stringtoguid("pak_stringtoguid", Pak_StringToGUID_f, "Compute GUID from input text", FCVAR_DEVELOPMENTONLY);

static ConCommand pak_compress("pak_compress", Pak_Compress_f, "Compresses specified RPAK file", FCVAR_DEVELOPMENTONLY, RTech_PakCompress_f_CompletionFunc);
static ConCommand pak_decompress("pak_decompress", Pak_Decompress_f, "Decompresses specified RPAK file", FCVAR_DEVELOPMENTONLY, RTech_PakDecompress_f_CompletionFunc);

static ConCommand pak_requestload("pak_requestload", Pak_RequestLoad_f, "Requests asynchronous load for specified RPAK file", FCVAR_DEVELOPMENTONLY, RTech_PakLoad_f_CompletionFunc);
static ConCommand pak_requestunload("pak_requestunload", Pak_RequestUnload_f, "Requests asynchronous unload for specified RPAK file or ID", FCVAR_DEVELOPMENTONLY, RTech_PakUnload_f_CompletionFunc);

static ConCommand pak_requestswap("pak_requestswap", Pak_RequestSwap_f, "Requests swap for specified RPAK file or ID", FCVAR_DEVELOPMENTONLY, RTech_PakSwap_f_CompletionFunc);

static ConCommand pak_listpaks("pak_listpaks", Pak_ListPaks_f, "Display a list of loaded RPAK files", FCVAR_RELEASE);
static ConCommand pak_listtypes("pak_listtypes", Pak_ListTypes_f, "Display a list of registered asset types", FCVAR_RELEASE);


// Symbols taken from R2 dll's.
PakLoadFuncs_s* g_pakLoadApi = nullptr;
