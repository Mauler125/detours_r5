#include "core/stdafx.h"
#include <tier0/memstd.h>
#include "tier1/utlbuffer.h"
#include <filesystem/filesystem.h>
#include "rtech/rson.h"

//-----------------------------------------------------------------------------
// Purpose: loads an RSON from a buffer
// Input  : *pszBufferName - 
//          *pBuffer       - 
//          rootType       - 
// Output : pointer to RSON object on success, nullptr otherwise
//-----------------------------------------------------------------------------
RSON::Node_t* RSON::LoadFromBuffer(const char* pszBufferName, char* pBuffer, RSON::eFieldType rootType)
{
	return RSON_LoadFromBuffer(pszBufferName, pBuffer, rootType, 0, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: loads an RSON from a file
// Input  : *pszFilePath - 
//          *pPathID     - 
// Output : pointer to RSON object on success, nullptr otherwise
//-----------------------------------------------------------------------------
RSON::Node_t* RSON::LoadFromFile(const char* pszFilePath, const char* pPathID, bool* parseFailure)
{
	FileHandle_t file = FileSystem()->Open(pszFilePath, "rt", pPathID);

	if (!file)
		return NULL;

	const ssize_t nFileSize = FileSystem()->Size(file);
	std::unique_ptr<char[]> fileBuf(new char[nFileSize + 1]);

	const ssize_t nRead = FileSystem()->Read(fileBuf.get(), nFileSize, file);
	FileSystem()->Close(file);

	fileBuf[nRead] = '\0';
	RSON::Node_t* node = RSON::LoadFromBuffer(pszFilePath, fileBuf.get(), eFieldType::RSON_OBJECT);
	
	if (!node && parseFailure)
		*parseFailure = true;

	return node;
}