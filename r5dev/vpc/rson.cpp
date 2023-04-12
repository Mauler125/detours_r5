#include "core/stdafx.h"
#include <tier0/memstd.h>
#include "tier1/utlbuffer.h"
#include <filesystem/filesystem.h>
#include "vpc/rson.h"

RSON::Node_t* RSON::LoadFromBuffer(const char* pszBufferName, char* pBuffer, RSON::eFieldType rootType)
{
	return RSON_LoadFromBuffer(pszBufferName, pBuffer, rootType, 0, NULL);
}

RSON::Node_t* RSON::LoadFromFile(const char* pszFilePath)
{
	if (FileSystem()->FileExists(pszFilePath, "GAME"))
	{
		FileHandle_t file = FileSystem()->Open(pszFilePath, "rt");

		if (!file)
			return NULL;

		uint32_t nFileSize = FileSystem()->Size(file);

		char* fileBuf = MemAllocSingleton()->Alloc<char>(nFileSize + 1);

		int nRead = FileSystem()->Read(fileBuf, nFileSize, file);
		FileSystem()->Close(file);

		fileBuf[nRead] = '\0';

		RSON::Node_t* node = RSON::LoadFromBuffer(pszFilePath, fileBuf, eFieldType::RSON_OBJECT);

		MemAllocSingleton()->Free(fileBuf);

		if (node)
			return node;
		else
		{
			// [rexx]: not sure if this should be fatal or not. ideally this should be handled appropriately
			// in the calling function
			Error(eDLL_T::ENGINE, NO_ERROR, "Error loading file '%s'\n", pszFilePath);
			return NULL;
		}
	}

	return NULL;
}