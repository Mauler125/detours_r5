#pragma once
#include "download_surface.h"

typedef CUtlMap<CUtlString, CUtlVector<CUtlString>*> DepotChangedList_t;

extern float g_flUpdateCheckRate;

void SDKLauncher_Restart();

bool SDKLauncher_CreateDepotDirectories();
bool SDKLauncher_ClearDepotDirectories();

bool SDKLauncher_ExtractZipFile(const rapidjson::Document& manifest, const CUtlString& filePath, const DepotChangedList_t* const changedList, CProgressPanel* const pProgress);
bool SDKLauncher_BeginInstall(const bool bPreRelease, const bool bOptionalDepots, const bool bFullInstallWhenListEmpty,
	CUtlVector<CUtlString>& zipList, CUtlString* errorMessage, CProgressPanel* const pProgress);

bool SDKLauncher_IsManifestValid(const rapidjson::Document& depotManifest);
bool SDKLauncher_IsDepositoryValid(const rapidjson::Value& depotAssetList);

bool SDKLauncher_DownloadDepotList(const rapidjson::Document& manifest, const CUtlVector<CUtlString>& depotList,
	CUtlVector<CUtlString>& outZipList, CUtlString* const errorMessage, CProgressPanel* const pProgress, const char* const pPath,
	const bool bOptionalDepots);

bool SDKLauncher_InstallDepotList(const rapidjson::Document& manifest, const CUtlVector<CUtlString>& depotList,
	DepotChangedList_t* fileList, CProgressPanel* const pProgress);

bool SDKLauncher_GetRemoteManifest(const char* const url, string& responseMessage, rapidjson::Document& remoteManifest, const bool bPreRelease);
bool SDKLauncher_GetLocalManifest(rapidjson::Document& localManifest);
bool SDKLauncher_WriteLocalManifest(const rapidjson::Document& localManifest, CUtlString* const errorMessage);

bool SDKLauncher_CheckDiskSpace(const int minRequiredSpace, int* const availableSize = nullptr);
bool SDKLauncher_CheckForUpdate(const bool bPreRelease);

bool SDKLauncher_ForceExistingInstanceOnTop();


