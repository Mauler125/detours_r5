#include "sdklauncher_utils.h"
#include "windows/window.h"
#include "tier1/xorstr.h"
#include "tier1/utlmap.h"
#include "tier2/curlutils.h"
#include "zip/src/ZipFile.h"
#include "tier2/jsonutils.h"

bool g_bPartialInstall = false;
//bool g_bExperimentalBuilds = false;
float g_flUpdateCheckRate = 64;

double s_flLastProgressUpdate = 0.0;

#define PROGRESS_CALLBACK_UPDATE_DELTA 0.1

// !TODO: perhaps this should be a core utility shared across
// the entire SDK to allow processes to restart them selfs.
void SDKLauncher_Restart()
{
	char currentPath[MAX_PATH];
	BOOL getResult = GetCurrentDirectoryA(sizeof(currentPath), currentPath);

	if (!getResult)
	{
		// TODO: dialog box and instruct user to manually open the launcher again.
		Error(eDLL_T::COMMON, 0xBADC0DE, __FUNCTION__": Failed to obtain current directory: error code = %08x\n", __FUNCTION__, GetLastError());
	}

	///////////////////////////////////////////////////////////////////////////
	STARTUPINFOA StartupInfo = { 0 };
	PROCESS_INFORMATION ProcInfo = { 0 };

	// Initialize startup info struct.
	StartupInfo.cb = sizeof(STARTUPINFOA);

	DWORD processId = GetProcessId(GetCurrentProcess());

	char commandLine[MAX_PATH];
	sprintf(commandLine, "%lu %s %s", processId, RESTART_DEPOT_DOWNLOAD_DIR, currentPath);

	BOOL createResult = CreateProcessA(
		"bin\\updater.exe",                            // lpApplicationName
		commandLine,                                   // lpCommandLine
		NULL,                                          // lpProcessAttributes
		NULL,                                          // lpThreadAttributes
		FALSE,                                         // bInheritHandles
		CREATE_SUSPENDED,                              // dwCreationFlags
		NULL,                                          // lpEnvironment
		currentPath,                                   // lpCurrentDirectory
		&StartupInfo,                                  // lpStartupInfo
		&ProcInfo                                      // lpProcessInformation
	);

	if (!createResult)
	{
		// TODO: dialog box and instruct user to update again.
		Error(eDLL_T::COMMON, 0xBADC0DE, __FUNCTION__": Failed to create update process: error code = %08x\n", GetLastError());
	}

	///////////////////////////////////////////////////////////////////////////
	// Resume the process.
	ResumeThread(ProcInfo.hThread);

	///////////////////////////////////////////////////////////////////////////
	// Close the process and thread handles.
	CloseHandle(ProcInfo.hProcess);
	CloseHandle(ProcInfo.hThread);

	ExitProcess(EXIT_SUCCESS);
}


//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_CreateDepotDirectories()
{
	if ((!CreateDirectoryA(BASE_PLATFORM_DIR, NULL)          && GetLastError() != ERROR_ALREADY_EXISTS) ||
	    (!CreateDirectoryA(DEFAULT_DEPOT_DOWNLOAD_DIR, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) ||
		(!CreateDirectoryA(RESTART_DEPOT_DOWNLOAD_DIR, NULL) && GetLastError() != ERROR_ALREADY_EXISTS))
	{
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_ClearDepotDirectories()
{
	// Clear all depot files.
	if (!RemoveDirectoryA(RESTART_DEPOT_DOWNLOAD_DIR) ||
		!RemoveDirectoryA(DEFAULT_DEPOT_DOWNLOAD_DIR))
	{
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool GetDepotList(const rapidjson::Document& manifest, const rapidjson::Value*& outDepotList)
{
	rapidjson::Value::ConstMemberIterator depotIt;
	if (JSON_GetIterator(manifest, "depot", JSONFieldType_e::kObject, depotIt))
	{
		if (depotIt->value.MemberCount())
		{
			outDepotList = &depotIt->value;
			return true;
		}
	}
	return false;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool GetDepotEntry(const rapidjson::Document& manifest, const char* const targetDepotName, const rapidjson::Value*& outDepotEntry)
{
	rapidjson::StringBuffer dumpBuff;
	const rapidjson::Value* depotList;

	if (GetDepotList(manifest, depotList))
	{
		for (rapidjson::Value::ConstMemberIterator itr = depotList->MemberBegin(); itr < depotList->MemberEnd(); itr++)
		{
			string name;

			if (!JSON_GetValue(itr->value, "name", JSONFieldType_e::kString, name))
			{
				continue;
			}
			
			if (name.compare(targetDepotName) == NULL)
			{
				outDepotEntry = &itr->value;
				return true;
			}
		}
	}

	JSON_DocumentToBufferDeserialize(manifest, dumpBuff);
	Warning(eDLL_T::COMMON, __FUNCTION__": Failed on target(% s) :\n%s\n", targetDepotName, dumpBuff.GetString());
	return false;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool GetDepotAssetList(const rapidjson::Document& manifest, const char* const targetDepotName, const rapidjson::Value*& outAssetList)
{
	rapidjson::StringBuffer dumpBuff;
	const rapidjson::Value* depotEntry;

	if (GetDepotEntry(manifest, targetDepotName, depotEntry))
	{
		rapidjson::Document::ConstMemberIterator assetListIt;

		if (JSON_GetIterator(*depotEntry, "assets", JSONFieldType_e::kObject, assetListIt))
		{
			if (assetListIt->value.MemberCount())
			{
				outAssetList = &assetListIt->value;
				return true;
			}
		}
	}

	JSON_DocumentToBufferDeserialize(manifest, dumpBuff);
	Warning(eDLL_T::COMMON, __FUNCTION__ ": Failed on target(%s):\n%s\n", targetDepotName, dumpBuff.GetString());
	return false;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_ExtractZipFile(const rapidjson::Document& manifest, const CUtlString& depotFilePath, const DepotChangedList_t* const changedList, CProgressPanel* const pProgress)
{
	ZipArchive::Ptr archive = ZipFile::Open(depotFilePath.Get());
	size_t entries = archive->GetEntriesCount();

	const rapidjson::Value* assetList;
	CUtlString depotFileName = depotFilePath.UnqualifiedFilename();

	const bool assetListRet = GetDepotAssetList(manifest, depotFileName.String(), assetList);
	const CUtlVector<CUtlString>* changedAssetList = nullptr;

	// If a file list is provided, only install what it contains.
	if (changedList)
	{
		unsigned short mapIdx = changedList->Find(depotFileName);

		if (mapIdx != changedList->InvalidIndex())
		{
			const CUtlVector<CUtlString>* const candidate = changedList->Element(mapIdx);

			if (!candidate->IsEmpty())
			{
				changedAssetList = candidate;
			}
		}
	}

	for (size_t i = 0; i < entries; ++i)
	{
		const auto entry = archive->GetEntry(int(i));

		if (entry->IsDirectory())
			continue;

		const CUtlString fullName = entry->GetFullName().c_str();
		const char* const pFullName = fullName.Get();

		// Asset hasn't changed, don't replace it.
		if (changedAssetList && !changedAssetList->HasElement(pFullName))
		{
			Msg(eDLL_T::COMMON, "Asset \"%s\" not in changed list; ignoring...\n", pFullName);
			continue;
		}

		bool installDuringRestart = false;

		// Determine whether or not the asset needs
		// to be installed during a restart.
		rapidjson::Value::ConstMemberIterator assetListIt;
		
		
		if (assetListRet && JSON_GetIterator(*assetList, pFullName, JSONFieldType_e::kObject, assetListIt))
		{
			bool bShouldRestart;
			if (JSON_GetValue(assetListIt->value, "restart", JSONFieldType_e::kBool, bShouldRestart))
			{
				installDuringRestart = bShouldRestart;
			}
		}

		const CUtlString absDirName = fullName.AbsPath();
		const CUtlString dirName = absDirName.DirName();

		CreateDirectories(absDirName.Get());

		if (pProgress)
		{
			pProgress->SetExportLabel(Format("%s (%llu of %llu)", pFullName, i+1, entries).c_str());

			size_t percentage = (i * 100) / entries;
			pProgress->UpdateProgress((uint32_t)percentage, false);
		}

		Msg(eDLL_T::COMMON, "Extracting: %s to %s\n", pFullName, dirName.Get());

		if (installDuringRestart)
		{
			CUtlString tempDir = RESTART_DEPOT_DOWNLOAD_DIR;
			tempDir.Append(pFullName);

			ZipFile::ExtractFile(depotFilePath.Get(), pFullName, tempDir.Get());
		}
		else
		{
			ZipFile::ExtractFile(depotFilePath.Get(), pFullName, pFullName);
		}
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_QueryServer(const char* const url, string& outResponse, string& outMessage, CURLINFO& outStatus)
{
	curl_slist* sList = nullptr;
	CURLParams params;

	params.writeFunction = CURLWriteStringCallback;
	params.timeout = QUERY_TIMEOUT;
	params.verifyPeer = true;
	params.followRedirect = true;
	params.verbose = 0;// IsDebug();

	CURL* curl = CURLInitRequest(url, nullptr, outResponse, sList, params);

	if (!curl)
	{
		return false;
	}

	CURLcode res = CURLSubmitRequest(curl, sList);

	if (!CURLHandleError(curl, res, outMessage, true))
	{
		return false;
	}

	outStatus = CURLRetrieveInfo(curl);
	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_AcquireReleaseManifest(const char* const url, string& responseMessage,
	rapidjson::Document& outManifest, const bool preRelease)
{
	string responseBody;
	CURLINFO status;

	if (!SDKLauncher_QueryServer(url, responseBody, responseMessage, status))
	{
		responseMessage = responseBody;
		return false;
	}

	if (status != 200)
	{
		responseMessage = responseBody;
		return false;
	}

	//rapidjson::Document responseJson;
	outManifest.Parse(responseBody.c_str());

	if (outManifest.HasParseError())
	{
		Warning(eDLL_T::COMMON, "%s: JSON parse error at position %zu: %s\n", __FUNCTION__, outManifest.GetErrorOffset(), rapidjson::GetParseError_En(outManifest.GetParseError()));
		return false;
	}

	if (!outManifest.IsArray())
		return false;

	rapidjson::Value::Array responseJsonArray = outManifest.GetArray();

	for (rapidjson::Value& val : responseJsonArray)
	{
		bool bIsPreRelease;
		if (!JSON_GetValue(val, "prerelease", JSONFieldType_e::kBool, bIsPreRelease))
			return false;

		if (preRelease)
		{
			if (bIsPreRelease)
			{
				outManifest.Swap(val);
				return true;
			}
		}
		else if (!bIsPreRelease)
		{
			outManifest.Swap(val);
			return true;
		}
	}

	outManifest.Swap(responseJsonArray[0]);

	Assert(!outManifest.Empty());
	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
int SDKLauncher_ProgressCallback(CURLProgress* const progessData, double dltotal,
	double dlnow, double ultotal, double ulnow)
{
	CProgressPanel* pDownloadSurface = (CProgressPanel*)progessData->user;

	if (pDownloadSurface->IsCanceled())
	{
		pDownloadSurface->Close();
		return -1;
	}

	// This gets called often, prevent this callback from eating all CPU.
	const double curTime = Plat_FloatTime();
	if ((curTime - s_flLastProgressUpdate) < PROGRESS_CALLBACK_UPDATE_DELTA)
	{
		return 0;
	}
	s_flLastProgressUpdate = curTime;

	double downloaded;
	curl_easy_getinfo(progessData->curl, CURLINFO_SIZE_DOWNLOAD, &downloaded);

	pDownloadSurface->SetExportLabel(Format("%s (%s of %s)", progessData->name,
		FormatBytes((size_t)downloaded).c_str(), FormatBytes(progessData->size).c_str()).c_str());

	size_t percentage = ((size_t)downloaded * 100) / progessData->size;
	pDownloadSurface->UpdateProgress((uint32_t)percentage, false);

	return 0;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_DownloadAsset(const char* const url, const char* const path, const char* const fileName,
	const size_t fileSize, const char* const options, CUtlString* const errorMessage, CProgressPanel* const pProgress)
{
	CURLParams params;

	params.writeFunction = CURLWriteFileCallback;
	params.statusFunction = SDKLauncher_ProgressCallback;
	params.followRedirect = true;

	bool ret = CURLDownloadFile(url, path, fileName, options, fileSize, pProgress, params, errorMessage);

	return ret;
}

bool SDKLauncher_BuildUpdateList(const rapidjson::Document& localManifest,
	const rapidjson::Document& remoteManifest, CUtlVector<CUtlString>& outDepotList, DepotChangedList_t& outFileList)
{
	rapidjson::Document::ConstMemberIterator remoteDepots;
	if (!JSON_GetIterator(remoteManifest, "depot", JSONFieldType_e::kObject, remoteDepots))
	{
		Warning(eDLL_T::COMMON, __FUNCTION__ ": Remote manifest does not contain the \"depot\" field\n");
		return false;
	}

	rapidjson::Document::ConstMemberIterator localDepots;
	if (!JSON_GetIterator(localManifest, "depot", JSONFieldType_e::kObject, localDepots))
	{
		Warning(eDLL_T::COMMON, __FUNCTION__": Local Manifest Invalid: Missing depot object\n");
		return false;
	}

	for (rapidjson::Value::ConstMemberIterator remoteDepot = remoteDepots->value.MemberBegin(); remoteDepot < remoteDepots->value.MemberEnd(); remoteDepot++)
	{
		const char* remoteDepotName = nullptr;
		if (!JSON_GetValue(remoteDepot->value, "name", JSONFieldType_e::kString, remoteDepotName) || !VALID_CHARSTAR(remoteDepotName))
		{
			Warning(eDLL_T::COMMON, __FUNCTION__"Invalid Remote Depot: Depot has no name\n");
			return false;
		}

		bool containsDepot = false;
		bool digestMatch = false;

		for (rapidjson::Value::ConstMemberIterator localDepot = localDepots->value.MemberBegin(); localDepot < localDepots->value.MemberEnd(); localDepot++)
		{
			const char* localDepotName = nullptr;
			
			if (!JSON_GetValue(localDepot->value, "name", JSONFieldType_e::kString, localDepotName) || !VALID_CHARSTAR(localDepotName))
			{
				Warning(eDLL_T::COMMON, __FUNCTION__": Invalid Local Depot: Depot has no name\n");
				return false;
			}

			if (V_strcmp(localDepotName, remoteDepotName) == 0)
			{
				containsDepot = true;

				const char* remoteDepotDigest = nullptr;
				const char* localDepotDigest = nullptr;

				if (JSON_GetValue(remoteDepot->value, "digest", JSONFieldType_e::kString, remoteDepotDigest) &&
					JSON_GetValue(localDepot->value, "digest", JSONFieldType_e::kString, localDepotDigest)
				)
				{
					if (V_strcmp(remoteDepotDigest, localDepotDigest) == 0)
					{
						digestMatch = true;
					}
				}
				
				if(!digestMatch)
				{
					// Check which files have been changed, and only add these to the update list.
					rapidjson::Value::ConstMemberIterator remoteAssets;
					if (!JSON_GetIterator(remoteDepot->value, "assets", JSONFieldType_e::kObject, remoteAssets))
					{
						Warning(eDLL_T::COMMON, __FUNCTION__": Failed to get assets for remote depot %s\n", localDepotName);
						return false;
					}

					rapidjson::Value::ConstMemberIterator localAssets;
					const bool bHasLocalAssets = JSON_GetIterator(localDepot->value, "assets", JSONFieldType_e::kObject, localAssets);

					// Vector containing all changed files.
					CUtlVector<CUtlString>* const changeFileVec = new CUtlVector<CUtlString>();
					outFileList.InsertOrReplace(remoteDepotName, changeFileVec);

					for (rapidjson::Value::ConstMemberIterator remoteAsset = remoteAssets->value.MemberBegin(); remoteAsset != remoteAssets->value.MemberEnd(); ++remoteAsset)
					{
						const char* const assetName = remoteAsset->name.GetString();
						rapidjson::Value::ConstMemberIterator localAsset;
						if (bHasLocalAssets && JSON_GetIterator(localAssets->value, assetName, JSONFieldType_e::kObject, localAsset))
						{
							const char* remoteAssetDigest = nullptr;
							const char* localAssetDigest = nullptr;

							if (!JSON_GetValue(remoteAsset->value, "digest", JSONFieldType_e::kString, remoteAssetDigest) || !remoteAssetDigest)
							{
								Msg(eDLL_T::COMMON, __FUNCTION__ ": Asset %s did not have a remote asset digest\n", assetName);
								return false;
							}

							if (!JSON_GetValue(localAsset->value, "digest", JSONFieldType_e::kString, localAssetDigest) || !localAssetDigest)
							{
								changeFileVec->AddToTail(assetName);
							}
							// Digest mismatch; this file needs to be replaced.
							else if (!V_strcmp(remoteAssetDigest, localAssetDigest))
							{
								Msg(eDLL_T::COMMON, __FUNCTION__": Digest mismatch for asset \"%s\"; added to changed list\n", assetName);
								changeFileVec->AddToTail(assetName);
							}

						}
						else // Newly added file.
						{
							Msg(eDLL_T::COMMON, __FUNCTION__": Local manifest does not contain asset \"%s\"; added to changed list\n", assetName);
							changeFileVec->AddToTail(assetName);
						}							
					}
				}

				break;
			}
		}

		if (containsDepot)
		{
			if (!digestMatch)
			{
				// Digest mismatch, the file has been changed,
				// add it to the list so we are installing it.
				outDepotList.AddToTail(remoteDepotName);
			}
		}
		else
		{
			// Local manifest does not contain the asset,
			// add it to the list so we are installing it.
			outDepotList.AddToTail(remoteDepotName);
		}
	}
	
	return true;
}

//----------------------------------------------------------------------------
// Purpose: start the automatic installation procedure
// Input  : bPreRelease               - 
//			bOptionalDepots           - 
//			bFullInstallWhenListEmpty - if true, installs all depots in the remote manifest when depot list vector is empty
//			&zipList                  - 
//			*errorMessage             - 
//			*pProgress                - 
// Output : true on success, false otherwise
//----------------------------------------------------------------------------
bool SDKLauncher_BeginInstall(const bool bPreRelease, const bool bOptionalDepots, const bool bFullInstallWhenListEmpty,
	CUtlVector<CUtlString>& zipList, CUtlString* const errorMessage, CProgressPanel* const pProgress)
{
	string responseMessage;
	rapidjson::Document remoteManifest;

	if (!SDKLauncher_GetRemoteManifest(XorStr(SDK_DEPOT_VENDOR), responseMessage, remoteManifest, bPreRelease))
	{
		Msg(eDLL_T::COMMON, "%s: Failed! %s\n", "SDKLauncher_GetRemoteManifest", responseMessage.c_str());

		// TODO: make more comprehensive...
		errorMessage->Set("Failed to obtain remote manifest");
		return false;
	}

	CUtlVector<CUtlString> depotList;
	DepotChangedList_t fileList(UtlStringLessFunc);

	rapidjson::Document localManifest;

	if (SDKLauncher_GetLocalManifest(localManifest))
	{
		SDKLauncher_BuildUpdateList(localManifest, remoteManifest, depotList, fileList);
	}
	else
	{
		// Leave the vector empty, this will download everything.
		Assert(depotList.IsEmpty());
	}

	if (depotList.IsEmpty() && !bFullInstallWhenListEmpty)
	{
		if (!SDKLauncher_WriteLocalManifest(remoteManifest, errorMessage))
		{
			return false;
		}
		return true;
	}

	Msg(eDLL_T::COMMON, "Depot download queue:\n");
	FOR_EACH_VEC(depotList, i)
	{
		const CUtlString& depotName = depotList[i];
		Msg(eDLL_T::COMMON, "[ %d ] = %s\n", i, depotName.Get());
	}

	if (!SDKLauncher_DownloadDepotList(remoteManifest, depotList,
		zipList, errorMessage, pProgress, DEFAULT_DEPOT_DOWNLOAD_DIR, bOptionalDepots))
	{
		// Error message is set by previous function.
		return false;
	}

	// Canceling returns true, as the function didn't fail.
	// We should however still delete all the downloaded files.
	if (pProgress->IsCanceled())
		return true;


	if (!SDKLauncher_InstallDepotList(remoteManifest, zipList, &fileList, pProgress))
	{
		return false;
	}

	if (!SDKLauncher_WriteLocalManifest(remoteManifest, errorMessage))
	{
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_DownloadDepotList(const rapidjson::Document& manifest, const CUtlVector<CUtlString>& depotList,
	CUtlVector<CUtlString>& outZipList, CUtlString* const errorMessage, CProgressPanel* const pProgress, const char* const pPath,
	const bool bOptionalDepots)
{
	rapidjson::Value::ConstMemberIterator depotsIt;

	if (!JSON_GetIterator(manifest, "depot", JSONFieldType_e::kObject, depotsIt))
	{
		Assert(0);
		errorMessage->Set("Invalid manifest");
		return false;
	}

	if (!depotsIt->value.MemberCount())
	{
		errorMessage->Set("No depots found in manifest");
		return false;
	}

	int i = 1;

	for (rapidjson::Value::ConstMemberIterator itr = depotsIt->value.MemberBegin(); itr != depotsIt->value.MemberEnd(); itr++)
	{
		if (pProgress->IsCanceled())
			break;

		if (!SDKLauncher_IsDepositoryValid(itr->value))
		{
			Assert(0);
			continue;
		}

		if (!bOptionalDepots && itr->value["optional"].GetBool())
		{
			continue;
		}

		const char* const fileName = itr->value["name"].GetString();
		
		if (!depotList.IsEmpty())
		{
			if (!depotList.HasElement(fileName))
			{
				continue;
			}
		}

		const size_t fileSize = itr->value["size"].GetUint64();
		CUtlString downloadLink = itr->value["vendor"].GetString();
		downloadLink.AppendSlash('/');
		downloadLink.Append(fileName);

		pProgress->SetText(Format("Downloading package %i of %i...", i,
			!depotList.IsEmpty() ? depotList.Count() : depotsIt->value.MemberCount()).c_str());

		Msg(eDLL_T::COMMON, __FUNCTION__": Trying to download asset: %s\n", fileName);

		
		if (!SDKLauncher_DownloadAsset(downloadLink.Get(), pPath, fileName, fileSize, "wb+", errorMessage, pProgress))
		{
			// Error message is set by previous function.
			return false;
		}
		

		// Check if its a zip file, as these are
		// the only files that will be installed.
		if (V_strcmp(V_GetFileExtension(fileName), "zip") == NULL)
		{
			CUtlString filePath;

			filePath = pPath;
			filePath.AppendSlash('/');
			filePath.Append(fileName);

			outZipList.AddToTail(filePath);
		}
		i++;
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_InstallDepotList(const rapidjson::Document& manifest, const CUtlVector<CUtlString>& depotList,
	DepotChangedList_t* fileList, CProgressPanel* const pProgress)
{
	// Install process cannot be canceled.
	pProgress->SetCanCancel(false);

	FOR_EACH_VEC(depotList, i)
	{
		pProgress->SetText(Format("Installing package %i of %i...", i + 1, depotList.Count()).c_str());
		const CUtlString& depotFilePath = depotList[i];

		try
		{
			// TODO[ AMOS ]: make the ZIP lib return error codes instead
			if (!SDKLauncher_ExtractZipFile(manifest, depotFilePath, fileList, pProgress))
				return false;
		}
		catch (const std::exception& ex) {
			Warning(eDLL_T::COMMON, __FUNCTION__": Failed to extract zip %s, err %s", depotFilePath.Get(), ex.what());
			return false;
		}
	}

	return true;
}

//----------------------------------------------------------------------------
// Purpose: checks if client has enough disk space
// Input  : minRequiredSpace - 
//        : *availableSize   - 
// Output : true if space is sufficient, false otherwise
//----------------------------------------------------------------------------
bool SDKLauncher_CheckDiskSpace(const int minRequiredSpace, int* const availableSize)
{
	char currentDir[MAX_PATH]; // Get current dir.
	GetCurrentDirectoryA(sizeof(currentDir), currentDir);

	// Does this disk have enough space?
	ULARGE_INTEGER avaliableSize;
	GetDiskFreeSpaceEx(currentDir, &avaliableSize, nullptr, nullptr);

	const int currentAvailSpace = (int)(avaliableSize.QuadPart / uint64_t(1024 * 1024 * 1024));

	if (availableSize)
	{
		*availableSize = currentAvailSpace;
	}

	if (currentAvailSpace < minRequiredSpace)
	{
		return false;
	}

	return true;
}

bool SDKLauncher_IsManifestValid(const rapidjson::Document& depotManifest)
{
	return (depotManifest.MemberCount() &&
		depotManifest.HasMember("version") && depotManifest["version"].IsString(),
		depotManifest.HasMember("depot") && depotManifest["depot"].IsObject()
	);
}

bool SDKLauncher_IsDepositoryValid(const rapidjson::Value& depot)
{
	return (depot.MemberCount() &&
		depot.HasMember("optional") && depot["optional"].IsBool() &&
		depot.HasMember("vendor") && depot["vendor"].IsString() &&
		depot.HasMember("size") && depot["size"].IsUint64() &&
		depot.HasMember("name") && depot["name"].IsString()
	);
}

bool SDKLauncher_GetRemoteManifest(const char* const url, string& responseMessage, rapidjson::Document& remoteManifest, const bool bPreRelease)
{
	rapidjson::Document responseJson;

	if (!SDKLauncher_AcquireReleaseManifest(url, responseMessage, responseJson, bPreRelease))
	{
		// TODO: Error dialog.

		Warning(eDLL_T::COMMON, "%s: failed!\n", "SDKLauncher_AcquireReleaseManifest\n");
		return false;
	}

	if (!responseJson.HasMember("assets"))
	{
		Warning(eDLL_T::COMMON, "%s: failed!\n", "responseJson.HasMember(\"assets\")\n");
		return false;
	}
	
	const char* depotManifestUrl = nullptr;

	rapidjson::Value::ConstMemberIterator assetListIt;
	if (!JSON_GetIterator(responseJson, "assets", JSONFieldType_e::kArray, assetListIt))
	{
		Warning(eDLL_T::COMMON, __FUNCTION__": Failed to get assets array from remote manifest\n");
		return false;
	}

	for (rapidjson::Value::ConstValueIterator assetItr = assetListIt->value.Begin(); assetItr < assetListIt->value.End(); assetItr++)
	{
		const char* assetName;
		if (JSON_GetValue(*assetItr, "name", JSONFieldType_e::kString, assetName))
		{
			if (V_strcmp(assetName, DEPOT_MANIFEST_FILE) == 0)
			{
				const char* assetDlUrl = nullptr;
				if (!JSON_GetValue(*assetItr, "browser_download_url", JSONFieldType_e::kString, assetDlUrl))
				{
					Warning(eDLL_T::COMMON, __FUNCTION__": Failed to get browser download url for asset %s\n", assetDlUrl);
					return false;
				}

				depotManifestUrl = assetDlUrl;
				break;
			}
		}
	}
	
	if (!VALID_CHARSTAR(depotManifestUrl))
	{
		Warning(eDLL_T::COMMON, "Failed to get depot manifest URL\n");
		return false;
	}

	string responseBody;
	CURLINFO status;

	if (!SDKLauncher_QueryServer(depotManifestUrl, responseBody, responseMessage, status) || 
		status != 200)
	{
		Warning(eDLL_T::COMMON, "%s: failed! %s, status=%d\n", "SDKLauncher_QueryServer", responseMessage.c_str(), status);

		responseMessage = responseBody;
		return false;
	}

	remoteManifest.Parse(responseBody.c_str());	
	if (remoteManifest.HasParseError())
	{
		Warning(eDLL_T::COMMON, __FUNCTION__": JSON parse error at position %zu: %s\n", remoteManifest.GetErrorOffset(), rapidjson::GetParseError_En(remoteManifest.GetParseError()));
		return false;
	}

	Assert(remoteManifest.IsObject());
	return true;
}

bool SDKLauncher_GetLocalManifest(rapidjson::Document& localManifest)
{
	if (!FileSystem()->FileExists(DEPOT_MANIFEST_FILE_PATH))
		return false;

	CUtlBuffer manifestFileBuff(ssize_t(0), 0, CUtlBuffer::TEXT_BUFFER);

	if (!FileSystem()->ReadFile(DEPOT_MANIFEST_FILE_PATH, nullptr, manifestFileBuff))
	{
		Warning(eDLL_T::COMMON, __FUNCTION__": Failed to read local manifest file\n");
		return false;
	}

	localManifest.Parse(static_cast<const char*>(manifestFileBuff.Base()), manifestFileBuff.Size());

	if (localManifest.HasParseError())
	{
		Warning(eDLL_T::COMMON, __FUNCTION__ ": JSON parse error at position %zu: %s\n", localManifest.GetErrorOffset(), rapidjson::GetParseError_En(localManifest.GetParseError()));
		return false;
	}

	if (!SDKLauncher_IsManifestValid(localManifest))
	{
		Warning(eDLL_T::COMMON, __FUNCTION__": Parsed local manifest was not valid\n");
		return false;
	}

	return true;
}

bool SDKLauncher_WriteLocalManifest(const rapidjson::Document& localManifest, CUtlString* const errorMessage)
{
	FileHandle_t hFile = FileSystem()->Open(DEPOT_MANIFEST_FILE_PATH, "wt+");

	if (hFile == INVALID_HANDLE_VALUE)
	{
		errorMessage->Set("Failed to write local manifest file (insufficient rights?)");
		return false;
	}
	
	rapidjson::StringBuffer buff;
	JSON_DocumentToBufferDeserialize(localManifest, buff);	

	FileSystem()->Write(buff.GetString(), buff.GetSize(), hFile);

	FileSystem()->Close(hFile);

	return true;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_CheckForUpdate(const bool bPreRelease)
{
	rapidjson::Document remoteManifest;
	string responseMessage;

	if (!SDKLauncher_AcquireReleaseManifest(XorStr(SDK_DEPOT_VENDOR), responseMessage, remoteManifest, bPreRelease))
	{
		Msg(eDLL_T::COMMON, __FUNCTION__": Failed to obtain remote manifest: %s\n", responseMessage.c_str());
		return false; // Can't determine if there is an update or not; skip...
	}

	rapidjson::Document localManifest;

	if (!SDKLauncher_GetLocalManifest(localManifest))
	{
		// Failed to load a local one; assume an update is required.
		Msg(eDLL_T::COMMON, __FUNCTION__": Failed to obtain local manifest\n");
		return true;
	}

	const char* pszLocalManifestVersion = nullptr;
	if (!JSON_GetValue(localManifest, "version", JSONFieldType_e::kString, pszLocalManifestVersion))
	{
		// No version information; assume an update is required.
		Msg(eDLL_T::COMMON, __FUNCTION__": Local manifest does not contain field '%s'!\n", "version");
		return true;
	}

	const char* pszRemoteManifestTagName = nullptr;
	if (!JSON_GetValue(remoteManifest, "tag_name", JSONFieldType_e::kString, pszRemoteManifestTagName))
	{
		// Don't update if we cant verify the remotes version
		Msg(eDLL_T::COMMON, __FUNCTION__": Remote manifest does not have a tag name\n");
		return false;
	}

	// This evaluates to '0' if the version tags are equal.
	const bool bUpdateAvaliable = V_strcmp(pszLocalManifestVersion, pszRemoteManifestTagName) != 0;

	if (bUpdateAvaliable)
	{
		Msg(eDLL_T::COMMON, __FUNCTION__": Game update avaliable, current latest version is %s installed version is %s\n", pszRemoteManifestTagName,
			pszLocalManifestVersion);
	}

	return bUpdateAvaliable;
}

//----------------------------------------------------------------------------
// Purpose: 
//----------------------------------------------------------------------------
bool SDKLauncher_ForceExistingInstanceOnTop()
{
	HWND existingApp = FindWindowA(FORM_DEFAULT_CLASS_NAME, NULL);

	if (existingApp)
	{
		ForceForegroundWindow(existingApp);
		return true;
	}

	return false;
}
