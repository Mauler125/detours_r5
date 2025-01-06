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

struct MaterialGlue_t;
struct TextureAsset_s;

struct StreamDBLump_s
{
	uint64_t offset;
	uint64_t count;
};

struct StreamDBHeader_s
{
	uint32_t magic;
	uint16_t majorVersion;
	uint16_t minorVersion;
	char unkPad1[20];
	float unk1;
	float unk2;
	char unkPad2[130];
	StreamDBLump_s lumps[6];
	char unkPad3[128];
};

struct StreamingDBPageState_s
{
	int page;
	int unk;
	char* pageData;
	char gap_10[8];
};

struct ST_ResidentPage
{
	uint64_t dataOffset;
	int dataSize;
	float coverageScale;
	uint16_t minCellX;
	uint16_t minCellY;
	uint16_t maxCellX;
	uint16_t maxCellY;
};

struct ST_Material
{
	int nameOffset;
	char unk[4];
	PakGuid_t materialGUID;
	char unk2[8];
};

#endif // RTECH_ISTREAMDB
