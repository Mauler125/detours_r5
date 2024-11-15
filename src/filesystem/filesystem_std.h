#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include <tier1/keyvalues.h>
#include "ifilesystem.h"

class CBaseFileSystem : public CTier1AppSystem<IFileSystem>
{
public:
	//--------------------------------------------------------
	virtual bool			IsSteam() const { return false; };
	virtual	FilesystemMountRetval_t MountSteamContent(int nExtraAppId = -1) { return FilesystemMountRetval_t::FILESYSTEM_MOUNT_FAILED; };

	virtual bool InitFeatureFlags() { return false; };
	virtual bool InitFeatureFlags(const char* pszFlagSetFile) { return false; };

	virtual void AddSearchPath(const char* pPath, const char* pPathID, SearchPathAdd_t addType) {};
	virtual bool RemoveSearchPath(const char* pPath, const char* pPathID) { return false; };
	virtual void			RemoveAllSearchPaths(void) {};
	virtual void			RemoveSearchPaths(const char* szPathID) {};
	virtual void			MarkPathIDByRequestOnly(const char* pPathID, bool bRequestOnly) {}
	virtual const char* RelativePathToFullPath(const char* pFileName, const char* pPathID, char* pLocalPath, ssize_t localPathBufferSize, PathTypeFilter_t pathFilter = FILTER_NONE, PathTypeQuery_t* pPathType = NULL) { return nullptr; };
#if IsGameConsole()
	virtual bool            GetPackFileInfoFromRelativePath(const char* pFileName, const char* pPathID, char* pPackPath, ssize_t nPackPathBufferSize, ptrdiff_t& nPosition, ssize_t& nLength) { return false; };
#endif
	virtual ssize_t			GetSearchPath(const char* pathID, bool bGetPackFiles, char* pPath, ssize_t nMaxLen) { return NULL; };
	virtual bool			AddPackFile(const char* fullpath, const char* pathID) { return false; };

	//--------------------------------------------------------
	// File manipulation operations
	//--------------------------------------------------------
	virtual void			RemoveFile(char const* pRelativePath, const char* pathID = 0) {};                  // Deletes a file (on the WritePath)
	virtual bool			RenameFile(char const* pOldPath, char const* pNewPath, const char* pathID = 0) { return false; }; // Renames a file (on the WritePath)
	virtual int				CreateDirHierarchy(const char* path, const char* pathID = 0);                   // create a local directory structure
	virtual bool			IsDirectory(const char* pFileName, const char* pathID = 0);                     // File I/O and info
	virtual ssize_t			FileTimeToString(char* pStrip, ssize_t maxCharsIncludingTerminator, long fileTime) { return NULL; }; // Returns the string size

	//--------------------------------------------------------
	// Open file operations
	//--------------------------------------------------------

	virtual void			SetBufferSize(FileHandle_t file/*, ssize_t nBytes*/) {};
	virtual bool			IsOk(FileHandle_t file) { return false; };
	virtual bool			EndOfFile(FileHandle_t file) { return false; };
	virtual char* ReadLine(char* pOutput, size_t maxChars, FileHandle_t file) { return nullptr; };
#if ! defined(SWIG)
	// Don't let SWIG see the PRINTF_FORMAT_STRING attribute or it will complain.
	virtual ssize_t			FPrintf(FileHandle_t file, PRINTF_FORMAT_STRING const char* pFormat, ...) FMTFUNCTION(3, 4) { return NULL; };
#else
	virtual ssize_t			FPrintf(FileHandle_t file, const char* pFormat, ...) FMTFUNCTION(3, 4) { return NULL; };
#endif

	//--------------------------------------------------------
	// Dynamic library operations
	//--------------------------------------------------------
	// load/unload modules
	virtual CSysModule* LoadModule(const char* pFileName, const char* pPathID = 0, bool bValidatedDllOnly = true) { return nullptr; };
	virtual void        UnloadModule(CSysModule* pModule) {};

	//--------------------------------------------------------
	// File searching operations
	//--------------------------------------------------------
	// FindFirst/FindNext. Also see FindFirstEx.
	virtual const char* FindFirst(const char* pWildCard, FileFindHandle_t* pHandle) { return nullptr; };
	virtual const char* FindNext(FileFindHandle_t handle) { return nullptr; };
	virtual bool        FindIsDirectory(FileFindHandle_t handle) { return false; };
	virtual void        FindClose(FileFindHandle_t handle) {};

	// Same as FindFirst, but you can filter by path ID, which can make it faster.
	virtual const char* FindFirstEx(
		const char* pWildCard,
		const char* pPathID,
		FileFindHandle_t* pHandle
	) { return nullptr; };

	// Searches for a file in all paths and results absolute path names for the file, works in pack files (zip and vpk) too
	// Lets you search for something like sound/sound.cache and get a list of every sound cache
	virtual void			FindFileAbsoluteList(CUtlVector<CUtlString>& outAbsolutePathNames, const char* pWildCard, const char* pPathID) {};

	//--------------------------------------------------------
	// File name and directory operations
	//--------------------------------------------------------

	// FIXME: This method is obsolete! Use RelativePathToFullPath instead!
	// converts a partial path into a full path
	virtual const char* GetLocalPath(const char* pFileName, char* pLocalPath, ssize_t localPathBufferSize) { return nullptr; };

	// Returns true on success ( based on current list of search paths, otherwise false if 
	//  it can't be resolved )
	virtual bool			FullPathToRelativePath(const char* pFullpath, char* pRelative, ssize_t maxlen) { return false; };

	// Gets the current working directory
	virtual bool			GetCurrentDirectory(char* pDirectory, unsigned int maxlen) { return false; }; // Last parameter is a DWORD passed to 'GetCurrentDirectoryA()' internally.

	//--------------------------------------------------------
	// Filename dictionary operations
	//--------------------------------------------------------

	virtual FileNameHandle_t	FindOrAddFileName(char const* pFileName) { return nullptr; };
	virtual bool				String(const FileNameHandle_t& handle, char* buf, ssize_t buflen) { return NULL; };

	//--------------------------------------------------------
	// Asynchronous file operations
	//--------------------------------------------------------

//--------------- [ !!! AMOS: !!! ALL ASYNC METHODS ARE UNIMPLEMENTED !!! PURECALL !!! ] ---------------//
	//------------------------------------
	// Global operations
	//------------------------------------
	virtual void PureCall0() {};
	virtual void PureCall1() {};
	virtual void PureCall2() {};
	virtual void PureCall3() {};
	virtual void PureCall4() {};
	virtual void PureCall5() {};

	//--------------------------------------------------------
	// Debugging operations
	//--------------------------------------------------------

	// Dump to printf/OutputDebugString the list of files that have not been closed
	virtual void			PrintOpenedFiles(void) {};
	virtual void			PrintSearchPaths(void) {};

	// output
	virtual void			SetWarningFunc(void (*pfnWarning)(const char* fmt, ...)) {};
	virtual void			SetWarningLevel(FileWarningLevel_t level) {};
	virtual void			AddLoggingFunc(void (*pfnLogFunc)(const char* fileName, const char* accessType)) {};
	virtual void			RemoveLoggingFunc(FileSystemLoggingFunc_t logFunc) {};

	virtual __int64 __fastcall sub_14038C240(__int64 a1) { return NULL; };
	virtual __int64 __fastcall sub_14038C380(__int64 a1) { return NULL; };
	virtual __int64 __fastcall sub_14038C400(__int64 a1, __int64 a2) { return NULL; };

	// Returns the file system statistics retrieved by the implementation.  Returns NULL if not supported.
	virtual const FileSystemStatistics* GetFilesystemStatistics() { return nullptr; };

	//--------------------------------------------------------
	// Start of new functions after Lost Coast release (7/05)
	//--------------------------------------------------------

	virtual FileHandle_t	OpenEx(const char* pFileName, const char* pOptions, unsigned flags = 0, const char* pathID = 0/*, char** ppszResolvedFilename = NULL*/) { return nullptr; };

	// Extended version of read provides more context to allow for more optimal reading
	virtual ssize_t			ReadEx(void* pOutput, ssize_t sizeDest, ssize_t size, FileHandle_t file);
	virtual ssize_t			ReadFileEx(const char* pFileName, const char* pPath, void** ppBuf, bool bNullTerminate = false, bool bOptimalAlloc = false, ssize_t nMaxBytes = 0, ptrdiff_t nStartingByte = 0, FSAllocFunc_t pfnAlloc = NULL) { return NULL; };

	virtual FileNameHandle_t	FindFileName(char const* pFileName) { return nullptr; };

#if defined( TRACK_BLOCKING_IO )
	virtual void			EnableBlockingFileAccessTracking(bool state) {};
	virtual bool			IsBlockingFileAccessEnabled() { return false; };

	virtual IBlockingFileItemList* RetrieveBlockingFileAccessInfo() { return nullptr; };
#endif

	virtual void SetupPreloadData() {};
	virtual void DiscardPreloadData() {};

	// If the "PreloadedData" hasn't been purged, then this'll try and instance the KeyValues using the fast path of compiled keyvalues loaded during startup.
	// Otherwise, it'll just fall through to the regular KeyValues loading routines
	virtual KeyValues* LoadKeyValues(KeyValuesPreloadType_t type, char const* filename, char const* pPathID = 0) { return nullptr; };
	virtual bool		LoadKeyValues(KeyValues& head, KeyValuesPreloadType_t type, char const* filename, char const* pPathID = 0) { return false; };

	virtual bool		GetFileTypeForFullPath(char const* pFullPath, wchar_t* buf, size_t bufSizeInBytes) { return false; };

	//--------------------------------------------------------
	//--------------------------------------------------------
	virtual bool		ReadToBuffer(FileHandle_t hFile, CUtlBuffer& buf, ssize_t nMaxBytes = 0, FSAllocFunc_t pfnAlloc = NULL);

	//--------------------------------------------------------
	// Optimal IO operations
	//--------------------------------------------------------
	virtual bool GetOptimalIOConstraints(FileHandle_t hFile, uint64_t* pOffsetAlign, uint64_t* pSizeAlign, uint64_t* pBufferAlign);
	virtual void* AllocOptimalReadBuffer(ptrdiff_t nOffset = 0/*!!! UNUSED !!!*/, ssize_t nSize = 0) { return nullptr; };
	virtual void FreeOptimalReadBuffer(void*) {};


	virtual bool __fastcall sub_140383E00(__int64 a2) { return false; };
	virtual bool sub_1403836A0() { return false; };
	virtual __int64 __fastcall sub_140384310(int a1) { return NULL; };
	virtual __int64 __fastcall CheckVPKMode(int nMode) { return NULL; }; // Checks if the VPK mode equals the mode input.


	//--------------------------------------------------------
	// Cache/VPK operations
	//--------------------------------------------------------
	virtual bool ReadFromCache(const char* pPath, FileSystemCache* pCache) { return false; };

	virtual bool __fastcall sub_14037FFA0(__int64 a1, unsigned int a2, __int64 a3) { return false; };

	virtual void SetVPKCacheModeClient() {}; // g_nVPKCacheMode = 1;
	virtual void SetVPKCacheModeServer() {}; // g_nVPKCacheMode = 2;
	virtual bool IsVPKCacheEnabled() { return false; };     // g_nVPKCacheMode != 0;

	virtual __int64 __fastcall PrecacheTaskItem(__int64 a1) { return NULL; };

	virtual void ResetItemCacheSize(int edx) {};
	virtual void __fastcall sub_140380100(__int64 a1) {};
	virtual void __fastcall sub_140380230(char a2) {};
	virtual void* __fastcall sub_1403801F0(const void* a1, unsigned int a2) { return nullptr; };
	virtual void __fastcall sub_140380220(__int64 a1) {};
	virtual bool ResetItemCache() { return false; };
	virtual char __fastcall sub_1403836D0(int a1, char* a2, unsigned int a3) { return false; };
	virtual __int64 __fastcall sub_140383840(unsigned int a1, __int64 a2, char* a3, unsigned int BufferCount) { return NULL; };
	virtual const char** __fastcall sub_140383760(unsigned int a1) { return nullptr; };
	virtual __int64 __fastcall sub_140383A20(const char* a1) { return NULL; };

	virtual VPKData_t* MountVPKFile(const char* pVpkPath) { return nullptr; };
	virtual const char* UnmountVPKFile(const char* pBasename) { return nullptr; };

	virtual void __fastcall sub_140383370() {};
	virtual void __fastcall sub_140383560() {};

	virtual unsigned __int64 __fastcall PnpCtxRegQueryInfoKey(__int64 a1, char* a2, unsigned int* a3, unsigned __int64 a4, unsigned __int64 a5, void(__fastcall* a6)(unsigned __int64)) { return NULL; };
	virtual char* sub_1403842B0() { return nullptr; };
	virtual __int64 __fastcall sub_1403842C0(__int64 a1, unsigned int a2, __int64 a3) { return NULL; };

	virtual char __fastcall LoadMainVPK(const char* pszVPKFile) { return NULL; };

	virtual __int64 sub_140380080() { return NULL; };
	virtual char __fastcall sub_14038B530(const char* a1, unsigned __int8* a2, char* a3, __int64 Count) { return NULL; };
	virtual __int64 __fastcall sub_14038C830(unsigned __int16* a1) { return NULL; };
	virtual __int64 __fastcall sub_140388360(char* a1, __int64 a2) { return NULL; };
	virtual __int64 __fastcall sub_140384C60(char* a1, unsigned __int64 a2) { return NULL; };
	virtual void __fastcall sub_140382A80() { };
	virtual __int64 __fastcall sub_14038CC90(int a1, unsigned int a2, __int64 a3, __int64 a4) { return NULL; };
	virtual __int64 __fastcall UserMathErrorFunction() { return NULL; };

	virtual ssize_t			Read(void* pOutput, ssize_t size, FileHandle_t file);
	virtual ssize_t			Write(void const* pInput, ssize_t size, FileHandle_t file);

	// if pathID is NULL, all paths will be searched for the file
	virtual FileHandle_t	Open(const char* pFileName, const char* pOptions, const char* pPathID = 0, int64_t unknown = 0);
	virtual void			Close(FileHandle_t file);

	virtual void			Seek(FileHandle_t file, ptrdiff_t pos, FileSystemSeek_t seekType);
	virtual ptrdiff_t		Tell(FileHandle_t file);
	virtual ssize_t			FSize(const char* pFileName, const char* pPathID = 0); // Gets optimized away if it isn't named differently or used.
	virtual ssize_t			Size(FileHandle_t file);

	virtual void			Flush(FileHandle_t file);
	virtual bool			Precache(const char* pFileName, const char* pPathID = 0);

	virtual bool			FileExists(const char* pFileName, const char* pPathID = 0);
	virtual bool			IsFileWritable(char const* pFileName, const char* pPathID = 0);
	virtual bool			SetFileWritable(char const* pFileName, bool writable, const char* pPathID = 0);

	virtual long long		GetFileTime(const char* pFileName, const char* pPathID = 0);

	virtual bool			ReadFile(const char* pFileName, const char* pPath, CUtlBuffer& buf, ssize_t nMaxBytes = 0, ptrdiff_t nStartingByte = 0, FSAllocFunc_t pfnAlloc = NULL);
	virtual bool			WriteFile(const char* pFileName, const char* pPath, CUtlBuffer& buf);
	virtual bool			UnzipFile(const char* pFileName, const char* pPath, const char* pDestination) { return false; };

	char* ReadLine(char* maxChars, ssize_t maxOutputLength, FileHandle_t file);
	CUtlString ReadString(FileHandle_t pFile);
};

class CFileSystem_Stdio : public CBaseFileSystem
{
protected:
	// implementation of CBaseFileSystem virtual functions
	virtual FILE* FS_fopen(const char* filename, const char* options, unsigned flags, int64* size) { return nullptr; };
	virtual void FS_setbufsize(FILE* fp, unsigned nBytes) {};
	virtual void FS_fclose(FILE* fp) {};
	virtual void FS_fseek(FILE* fp, int64 pos, int seekType) {};
	virtual long FS_ftell(FILE* fp) { return NULL; };
	virtual int FS_feof(FILE* fp) { return NULL; };
	virtual size_t FS_fread(void* dest, size_t destSize, size_t size, FILE* fp) { return NULL; };
	virtual size_t FS_fwrite(const void* src, size_t size, FILE* fp) { return NULL; };
	virtual bool FS_setmode(FILE* fp, FileMode_t mode) { return false; };
	virtual size_t FS_vfprintf(FILE* fp, const char* fmt, va_list list) { return NULL; };
	virtual int FS_ferror(FILE* fp) { return NULL; };
	virtual int FS_fflush(FILE* fp) { return NULL; };
	virtual char* FS_fgets(char* dest, unsigned int destSize) { return nullptr; };
	virtual int FS_stat(const char* path, struct _stat* buf, bool* pbLoadedFromSteamCache = NULL) { return NULL; };
	virtual int FS_chmod(const char* path, int pmode) { return NULL; };
	virtual HANDLE FS_FindFirstFile(const char* findname, WIN32_FIND_DATA* dat) { return NULL; };
	virtual bool FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA* dat) { return false; };
	virtual bool FS_FindClose(HANDLE handle) { return false; };
	virtual int FS_GetSectorSize(FILE*) { return NULL; };
};

#endif // FILESYSTEM_H
