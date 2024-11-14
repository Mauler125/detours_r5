#pragma once
#include "mathlib/color.h"
#include "tier0/tslist.h"
#include "tier1/utlbuffer.h"
#include "public/ifilesystem.h"
#include "filesystem/filesystem.h"

class RSON
{
public:
	enum eFieldType
	{
		RSON_NULL = 0x1,
		RSON_STRING = 0x2,
		RSON_VALUE = 0x4,
		RSON_OBJECT = 0x8,
		RSON_BOOLEAN = 0x10,
		RSON_INTEGER = 0x20,
		RSON_SIGNED_INTEGER = 0x40,
		RSON_UNSIGNED_INTEGER = 0x80,
		RSON_DOUBLE = 0x100,
		RSON_ARRAY = 0x1000,
	};


	//-------------------------------------------------------------------------
	// 
	//-------------------------------------------------------------------------
	struct Field_t;


	//-------------------------------------------------------------------------
	// 
	//-------------------------------------------------------------------------
	union Value_t
	{
		inline Field_t* GetSubKey() const { return pSubKey; };
		inline Value_t* GetSubValue() const { return pSubValue; };
		inline const char* GetString() const { return pszString; };
		inline int64_t GetInt() const { return integerValue; };

		Field_t* pSubKey;
		Value_t* pSubValue;
		char* pszString;
		int64_t integerValue;
	};

	//-------------------------------------------------------------------------
	// used for the root node of rson tree
	//-------------------------------------------------------------------------
	struct Node_t
	{
		eFieldType m_Type;
		int m_nValueCount;
		Value_t m_Value;

		inline Field_t* GetFirstSubKey() const;
		inline Value_t* GetArrayValue(const int index) const;

		// does not support finding a key in a different level of the tree
		inline Field_t* FindKey(const char* const pszKeyName) const;
	};

	//-------------------------------------------------------------------------
	// used for every other field of the rson tree
	//-------------------------------------------------------------------------
	struct Field_t
	{
		char* m_pszName;
		Node_t m_Node;
		Field_t* m_pNext;
		Field_t* m_pPrev;

		// Inlines
		inline const char* GetString() const { return (m_Node.m_Type == RSON_STRING) ? m_Node.m_Value.GetString() : NULL; };
		inline Field_t* GetNextKey() const { return m_pNext; };
		inline Field_t* GetLastKey() const { return m_pPrev; };

		inline Field_t* GetFirstSubKey() const { return m_Node.GetFirstSubKey(); };
		inline Field_t* FindKey(const char* pszKeyName) const { return m_Node.FindKey(pszKeyName); };
	};

public:
	static Node_t* LoadFromBuffer(const char* pszBufferName, char* pBuffer, eFieldType rootType);
	static Node_t* LoadFromFile(const char* pszFilePath, const char* pPathID = nullptr);
};


///////////////////////////////////////////////////////////////////////////////

RSON::Field_t* RSON::Node_t::GetFirstSubKey() const
{
	if (m_Type & eFieldType::RSON_OBJECT)
		return m_Value.pSubKey;

	return NULL;
};

RSON::Field_t* RSON::Node_t::FindKey(const char* const pszKeyName) const
{
	if ((m_Type & eFieldType::RSON_OBJECT) == 0)
		return NULL;

	for (Field_t* pKey = GetFirstSubKey(); pKey != nullptr; pKey = pKey->GetNextKey())
	{
		if (!_stricmp(pKey->m_pszName, pszKeyName))
			return pKey;
	}

	return NULL;
}

RSON::Value_t* RSON::Node_t::GetArrayValue(const int index) const
{
	assert(m_Type & eFieldType::RSON_ARRAY);
	return m_Value.GetSubValue() + index;
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
inline RSON::Node_t* (*RSON_LoadFromBuffer)(const char* bufName, char* buf, RSON::eFieldType rootType, __int64 a4, void* a5);
inline void (*RSON_Free)(RSON::Node_t* rson, CAlignedMemAlloc* allocator);

///////////////////////////////////////////////////////////////////////////////
class VRSON : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("RSON_LoadFromBuffer", RSON_LoadFromBuffer);
		LogFunAdr("RSON_Free", RSON_Free);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 89 45 60 48 8B D8").FollowNearCallSelf().GetPtr(RSON_LoadFromBuffer);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 83 EF 01 75 E7").FollowNearCallSelf().GetPtr(RSON_Free);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

