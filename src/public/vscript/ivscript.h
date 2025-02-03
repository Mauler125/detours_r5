#ifndef IVSCRIPT_H
#define IVSCRIPT_H
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "tier1/utlvector.h"
#include "datamap.h"

//---------------------------------------------------------
// Location of the scripts, and its compile list
#define GAME_SCRIPT_PATH "scripts/vscripts/"
#define GAME_SCRIPT_COMPILELIST GAME_SCRIPT_PATH"scripts.rson"

DECLARE_POINTER_HANDLE(HSCRIPT);
#define INVALID_HSCRIPT ((HSCRIPT)-1)

typedef int ScriptDataType_t;
typedef void* ScriptFunctionBindingStorageType_t;

//---------------------------------------------------------

enum ScriptStatus_t
{
	SCRIPT_ERROR = -1,
	SCRIPT_DONE,
	SCRIPT_RUNNING,
};

//---------------------------------------------------------

enum ExtendedFieldType
{
	FIELD_TYPEUNKNOWN = FIELD_TYPECOUNT,
	FIELD_TYPEUNKNOWN1, // Unknown field from R2
	FIELD_CSTRING,
	FIELD_HSCRIPT,
	FIELD_TYPEUNKNOWN2, // Unknown field from R5
	FIELD_VARIANT,
	FIELD_TYPEUNKNOWN3,
	FIELD_TYPEUNKNOWN4,
	FIELD_TYPEUNKNOWN5,
	FIELD_TYPEUNKNOWN6,
	FIELD_ASSET,
	FIELD_OSTRING
};

inline const char* ScriptFieldTypeName(int16 eType)
{
	switch (eType)
	{
	case FIELD_VOID:	return "void";
	case FIELD_FLOAT:	return "float";
	case FIELD_CSTRING:	return "cstring";
	case FIELD_VECTOR:	return "vector";
	case FIELD_INTEGER:	return "integer";
	case FIELD_BOOLEAN:	return "boolean";
	case FIELD_CHARACTER: return "character";
	case FIELD_HSCRIPT:	return "hscript";
	case FIELD_VARIANT:	return "variant";
	case FIELD_ASSET: return "asset";
	case FIELD_OSTRING: return "ostring";
	default:	return "unknown_script_type";
	}
}

//---------------------------------------------------------

struct ScriptFuncDescriptor_t
{
	void Init(const SQChar* scriptName, const SQChar* nativeName,
		const SQChar* description, const SQChar* returnType,
		const SQChar* parameters)
	{
		m_ScriptName = scriptName;
		m_Function = nativeName;
		m_Description = description;
		m_ReturnType = returnType;
		m_Parameters = parameters;
	}

	const SQChar* m_ScriptName;
	const SQChar* m_Function;
	const SQChar* m_Description;
	const SQChar* m_ReturnType;
	const SQChar* m_Parameters;
};

//---------------------------------------------------------

struct ScriptFunctionBinding_t
{
	ScriptFuncDescriptor_t m_Descriptor;
	bool m_bCheckParams;
	bool unk29; // 5th (new) parameter to 'sq_setparamscheck', see [r5apex_ds+10584F2]

	SQInteger m_nDevLevel; // TODO: confirm, this is a guess.
	const SQChar* m_pszCodeHook;
	int unk38; // 4th (new) parameter to 'sq_setparamscheck', see [r5apex_ds+10584F2]

	ScriptDataType_t m_ReturnType;
	CUtlVector<ScriptDataType_t> m_Parameters;
	ScriptFunctionBindingStorageType_t m_pFunction;

	void Init(
		const SQChar* scriptName, const SQChar* nativeName,
		const SQChar* helpString, const SQChar* returnString,
		const SQChar* parameters, const ScriptDataType_t returnType,
		const ScriptFunctionBindingStorageType_t function)
	{
		m_Descriptor.Init(scriptName, nativeName, helpString, returnString, parameters);
		m_bCheckParams = false;
		unk29 = false;

		m_nDevLevel = 0;
		m_pszCodeHook = nullptr;
		unk38 = 0;

		m_ReturnType = returnType;
		m_pFunction = function;
	}
};
static_assert(sizeof(ScriptFunctionBinding_t) == 0x68);

//---------------------------------------------------------

#pragma pack(push, 4)
struct ScriptVariant_t
{
	enum SVFlags_t
	{
		SV_FREE = 0x01,
	};

	~ScriptVariant_t() { Free(); }

	ScriptVariant_t() : m_flags(0), m_type(FIELD_VOID) { m_Vec3D = Vector3D(0, 0, 0); };
	ScriptVariant_t(int val) : m_flags(0), m_type(FIELD_INTEGER) { m_int = val; }
	ScriptVariant_t(float val) : m_flags(0), m_type(FIELD_FLOAT) { m_float = val; }
	ScriptVariant_t(char val) : m_flags(0), m_type(FIELD_CHARACTER) { m_char = val; }
	ScriptVariant_t(bool val) : m_flags(0), m_type(FIELD_BOOLEAN) { m_bool = val; }
	ScriptVariant_t(HSCRIPT val) : m_flags(0), m_type(FIELD_HSCRIPT) { m_hScript = val; }

	ScriptVariant_t(const Vector3D& val, bool bCopy = false) : m_flags(0), m_type(FIELD_VECTOR) { m_Vec3D = val; };
	ScriptVariant_t(const Vector3D* val, bool bCopy = false) : m_flags(0), m_type(FIELD_VECTOR) { m_Vec3D = *val; };
	ScriptVariant_t(const char* val, bool bCopy = false) : m_flags(0), m_type(FIELD_CSTRING) { if (!bCopy) { m_pszString = val; } else { m_pszString = strdup(val); m_flags |= SV_FREE; } }

	bool IsNull() const { return (m_type == FIELD_VOID); }

	operator int() const { Assert(m_type == FIELD_INTEGER);	return m_int; }
	operator int64() const { Assert(m_type == FIELD_INTEGER); return static_cast<int64>(m_int); }
	operator float() const { Assert(m_type == FIELD_FLOAT);	return m_float; }
	operator const char* () const { Assert(m_type == FIELD_CSTRING); return (m_pszString) ? m_pszString : ""; }
	operator const Vector3D& () const { Assert(m_type == FIELD_VECTOR);	static Vector3D vecNull(0, 0, 0); return m_Vec3D.IsValid() ? m_Vec3D : vecNull; }
	operator char() const { Assert(m_type == FIELD_CHARACTER); return m_char; }
	operator bool() const { Assert(m_type == FIELD_BOOLEAN); return m_bool; }
	operator HSCRIPT() const { Assert(m_type == FIELD_HSCRIPT);	return m_hScript; }


	void operator=(const int i) { m_type = FIELD_INTEGER; m_int = i; }
	void operator=(const float f) { m_type = FIELD_FLOAT; m_float = f; }
	void operator=(const Vector3D& vec) { m_type = FIELD_VECTOR; m_Vec3D = vec; }
	void operator=(const Vector3D* const vec) { m_type = FIELD_VECTOR; m_Vec3D = *vec; }
	void operator=(const char* const psz) { m_type = FIELD_CSTRING; m_pszString = psz; }
	void operator=(const char c) { m_type = FIELD_CHARACTER; m_char = c; }
	void operator=(const bool b) { m_type = FIELD_BOOLEAN; m_bool = b; }
	void operator=(HSCRIPT h) { m_type = FIELD_HSCRIPT; m_hScript = h; }

	void Free() { if (m_flags & SV_FREE) delete m_pszString; } // Generally only needed for return results

	template <typename T>
	T Get() const
	{
		T value;
		AssignTo(&value);
		return value;
	}

	bool AssignTo(float* const pDest) const
	{
		switch (m_type)
		{
		case FIELD_VOID:		*pDest = 0; return false;
		case FIELD_INTEGER:	*pDest = static_cast<float>(m_int); return true;
		case FIELD_FLOAT:		*pDest = m_float; return true;
		case FIELD_BOOLEAN:		*pDest = static_cast<float>(m_bool); return true;
		default:
			DevMsg(eDLL_T::ENGINE, "No conversion from %s to float now\n", ScriptFieldTypeName(m_type));
			return false;
		}
	}

	bool AssignTo(int* const pDest) const
	{
		switch (m_type)
		{
		case FIELD_VOID:		*pDest = 0; return false;
		case FIELD_INTEGER:	*pDest = m_int; return true;
		case FIELD_FLOAT:		*pDest = static_cast<int>(m_float); return true;
		case FIELD_BOOLEAN:		*pDest = m_bool; return true;
		default:
			DevMsg(eDLL_T::ENGINE, "No conversion from %s to int now\n", ScriptFieldTypeName(m_type));
			return false;
		}
	}

	bool AssignTo(bool* const pDest) const
	{
		switch (m_type)
		{
		case FIELD_VOID:		*pDest = 0; return false;
		case FIELD_INTEGER:	*pDest = m_int; return true;
		case FIELD_FLOAT:		*pDest = m_float; return true;
		case FIELD_BOOLEAN:		*pDest = m_bool; return true;
		default:
			DevMsg(eDLL_T::ENGINE, "No conversion from %s to bool now\n", ScriptFieldTypeName(m_type));
			return false;
		}
	}

	bool AssignTo(char** pDest) const
	{
		DevMsg(eDLL_T::ENGINE, "No free conversion of string or vector script variant right now\n");
		// If want to support this, probably need to malloc string and require free on other side [3/24/2008 tom]
		*pDest = const_cast<char*>("");
		return false;
	}

	void AssignTo(ScriptVariant_t* const pDest) const
	{
		pDest->m_type = m_type;
		if (m_type == FIELD_CSTRING)
		{
			pDest->m_pszString = strdup(m_pszString);
			pDest->m_flags |= SV_FREE;
		}
		else
		{
			pDest->m_Vec3D = m_Vec3D;
		}
	}

	union
	{
		int				m_int;
		float			m_float;
		const char* m_pszString;
		char			m_char;
		bool			m_bool;
		HSCRIPT         m_hScript;
		Vector3D        m_Vec3D;
	};

	int16				m_flags;
	int16				m_type;
};
#pragma pack(pop)
static_assert(sizeof(ScriptVariant_t) == 0x10);
//---------------------------------------------------------

struct ScriptClassDescriptor_t
{
	void AddFunction(const SQChar* scriptName, const SQChar* nativeName,
		const SQChar* helpString, const SQChar* returnString,
		const SQChar* parameters, const ScriptDataType_t returnType,
		const ScriptFunctionBindingStorageType_t function)
	{
		const int index = m_FunctionBindings.AddToTail();
		ScriptFunctionBinding_t& binding = m_FunctionBindings.Element(index);

		binding.Init(scriptName, nativeName, helpString, returnString, parameters, returnType, function);
	}

	const char* m_ScriptName;
	const char* m_Classname;
	const char* m_Description;

	ScriptClassDescriptor_t* m_BaseDesc;
	CUtlVector<ScriptFunctionBinding_t> m_FunctionBindings;

	// TODO: CUtlMemory?
	ssize_t m_Unk1;
	ssize_t m_Unk2;
	void* m_Unk3;

	ScriptClassDescriptor_t* m_NextDesc;
};

static_assert(sizeof(ScriptClassDescriptor_t) == 0x60);

#endif // IVSCRIPT_H
