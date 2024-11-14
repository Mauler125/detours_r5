#ifndef IDATACACHE_H
#define IDATACACHE_H

//---------------------------------------------------------
// Cache-defined handle for a cache item
//---------------------------------------------------------
FORWARD_DECLARE_HANDLE(memhandle_t);
typedef memhandle_t DataCacheHandle_t;

#define DC_INVALID_HANDLE ((void*)0xDEADFEEDDEADFEED)

//---------------------------------------------------------
// Check whether the data cache handle is valid
//---------------------------------------------------------
inline bool IsValidDataCacheHandle(const void* const handle)
{
	if (!handle)
		return false;

	if (IsDebug())
	{
		if (handle == DC_INVALID_HANDLE)
			return false;
	}

	return true;
}

#endif // IDATACACHE_H
