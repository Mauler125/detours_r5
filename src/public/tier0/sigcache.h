#ifndef SIGCACHE_H
#define SIGCACHE_H

#include "protoc/sig_map.pb.h"

#define SIGDB_MAGIC	(('p'<<24)+('a'<<16)+('M'<<8)+'S')
#define SIGDB_DICT_SIZE 20

#define SIGDB_MAJOR_VERSION 0x2 // Increment when library changes are made.
#define SIGDB_MINOR_VERSION 0xE // Increment when SDK updates are released.

class CSigCache
{
public:
	CSigCache()
		: m_bInitialized(false)
		, m_bDisabled(false) {};
	~CSigCache() {};

	void SetDisabled(const bool bDisabled);
	void InvalidateMap();

	void AddEntry(const char* szPattern, const uint64_t nRVA);
	bool FindEntry(const char* szPattern, uint64_t& nRVA);

	bool ReadCache(const char* szCacheFile);
	bool WriteCache(const char* szCacheFile) const;

private:
	bool CompressBlob(const size_t nSrcLen, size_t& nDstLen, uint32_t& nAdler32, const uint8_t* pSrcBuf, uint8_t* pDstBuf) const;
	bool DecompressBlob(const size_t nSrcLen, size_t& nDstLen, uint32_t& nAdler32, const uint8_t* pSrcBuf, uint8_t* pDstBuf) const;

	SigMap_Pb m_Cache;
	bool m_bInitialized;
	bool m_bDisabled;
};
extern CSigCache g_SigCache;

#pragma pack(push, 1)
struct SigDBHeader_t
{
	int m_nMagic;
	uint16_t m_nMajorVersion;
	uint16_t m_nMinorVersion;
	uint64_t m_nBlobSizeMem;
	uint64_t m_nBlobSizeDisk;
	uint32_t m_nBlobChecksum;
};
#pragma pack(pop)

#endif // !SIGCACHE_H
