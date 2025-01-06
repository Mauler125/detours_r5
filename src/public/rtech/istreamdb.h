//=============================================================================//
//
// Purpose: stream database constants and types
//
//=============================================================================//
#ifndef RTECH_ISTREAMDB
#define RTECH_ISTREAMDB
#include "ipakfile.h"

#define STBSP_FILE_EXTENSION "stbsp"
#define STBSP_NOMINAL_TEX_RES 4096

struct StreamDB_Lump_s
{
	uint64 offset;
	uint64 count;
};

struct StreamDB_Material_s
{
	int nameOffset;
	char unk[4];
	PakGuid_t materialGUID;
	char unk2[8];
};

struct StreamDB_Header_s
{
	uint32 magic;
	uint16 majorVersion;
	uint16 minorVersion;
	char unkPad1[20];
	float unk1;
	float unk2;
	char unkPad2[130];
	StreamDB_Lump_s lumps[6];
	char unkPad3[128];
};

struct StreamDB_PageState_s
{
	int page;
	int unk;
	char* pageData;
	char gap_10[8];
};

struct StreamDB_ResidentPage_s
{
	uint64 dataOffset;
	int dataSize;
	float coverageScale;
	uint16 minCellX;
	uint16 minCellY;
	uint16 maxCellX;
	uint16 maxCellY;
};

#endif // RTECH_ISTREAMDB
