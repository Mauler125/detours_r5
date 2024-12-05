#ifndef VSQUIRREL_H
#define VSQUIRREL_H
#include "tier1/utlmap.h"
#include "tier1/utlhash.h"
#include "tier1/utlbuffer.h"

#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "vscript/languages/squirrel_re/include/sqstate.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "vscript/ivscript.h"

#include "rtech/rson.h"

#define MAX_PRECOMPILED_SCRIPTS 1024

class CSquirrelVM
{
public:
	static bool Init(CSquirrelVM* s, SQCONTEXT context, float curtime);
	static bool DestroySignalEntryListHead(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f);

	void CompileModScripts();
	void SetAsCompiler(RSON::Node_t* rson);

	SQRESULT RegisterFunction(const SQChar* scriptname, const SQChar* nativename, const SQChar* helpstring, const SQChar* returntype, const SQChar* parameters, void* functor);
	SQRESULT RegisterConstant(const SQChar* name, SQInteger value);

	FORCEINLINE HSQUIRRELVM GetVM() const { return m_hVM; }
	FORCEINLINE SQCONTEXT GetContext() const { return m_iContext; }
	FORCEINLINE eDLL_T GetNativeContext() const { return (eDLL_T)GetContext(); }

	bool Run(const SQChar* const script);

	ScriptStatus_t ExecuteFunction(HSCRIPT hFunction, void** pArgs, unsigned int nArgs, void* pReturn, HSCRIPT hScope);
	bool ExecuteCodeCallback(const SQChar* const name);

private:
	bool unk_00;
	SQChar pad0[7];
	HSQUIRRELVM m_hVM;
	void* m_hDbg;
	SQObjectPtr m_ErrorString;
	SQChar pad3[8];
	SQInteger m_nTick;
	int unk_34;
	SQCONTEXT m_iContext;
	SQChar pad6[4];
	CUtlMap<SQClass*, CUtlHashFastGenericHash> m_TypeMap;
	CUtlBuffer* m_pBuffer;
	CUtlMap<void*, void*> m_PtrMap;
	bool unk_A8;
	int64_t unk_B0;
	int64_t unk_B8;
	bool unk_C0;
	int64_t unk_C8;
	int64_t unk_D0;
};

extern void(*ServerScriptRegister_Callback)(CSquirrelVM* const s);
extern void(*ClientScriptRegister_Callback)(CSquirrelVM* const s);
extern void(*UiScriptRegister_Callback)(CSquirrelVM* const s);

extern void(*ServerScriptRegisterEnum_Callback)(CSquirrelVM* const s);
extern void(*ClientScriptRegisterEnum_Callback)(CSquirrelVM* const s);
extern void(*UIScriptRegisterEnum_Callback)(CSquirrelVM* const s);

extern void(*UiServerScriptRegister_Callback)(CSquirrelVM* const s);
extern void(*UiAdminPanelScriptRegister_Callback)(CSquirrelVM* const s);

extern void(*ScriptConstantRegister_Callback)(CSquirrelVM* const s);

inline bool(*CSquirrelVM__Init)(CSquirrelVM* s, SQCONTEXT context, SQFloat curtime);
inline bool(*CSquirrelVM__DestroySignalEntryListHead)(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f);
inline SQRESULT(*CSquirrelVM__RegisterFunction)(CSquirrelVM* s, ScriptFunctionBinding_t* binding, SQInteger a1);
inline SQRESULT(*CSquirrelVM__RegisterConstant)(CSquirrelVM* s, const SQChar* name, SQInteger value);

#ifndef DEDICATED
inline bool(*CSquirrelVM__PrecompileClientScripts)(CSquirrelVM* vm, SQCONTEXT context, char** scriptArray, int scriptCount);
#endif

#ifndef CLIENT_DLL
inline bool(*CSquirrelVM__PrecompileServerScripts)(CSquirrelVM* vm, SQCONTEXT context, char** scriptArray, int scriptCount);
#endif
inline ScriptStatus_t(*CSquirrelVM__ExecuteFunction)(CSquirrelVM* s, HSCRIPT hFunction, void** pArgs, unsigned int nArgs, void* pReturn, HSCRIPT hScope);
inline bool(*CSquirrelVM__ExecuteCodeCallback)(CSquirrelVM* s, const SQChar* callbackName);

inline bool(*CSquirrelVM__ThrowError)(CSquirrelVM* vm, HSQUIRRELVM v);

#ifndef CLIENT_DLL
inline CSquirrelVM* g_pServerScript;
#endif // !CLIENT_DLL

#ifndef DEDICATED
inline CSquirrelVM* g_pClientScript;
inline CSquirrelVM* g_pUIScript;

inline bool* g_bUIScriptInitialized;
#endif // !DEDICATED

#define DEFINE_SCRIPTENUM_NAMED(s, enumName, startValue, ...) \
	HSQUIRRELVM const v = s->GetVM(); \
	const eDLL_T context = static_cast<eDLL_T>(s->GetContext());\
	sq_startconsttable(v); \
	sq_pushstring(v, enumName, -1); \
	sq_newtable(v); \
	const char* const enumFields[] = { __VA_ARGS__ }; \
	int enumValue = startValue; \
	for (int i = 0; i < V_ARRAYSIZE(enumFields); i++) { \
		sq_pushstring(v, enumFields[i], -1); \
		sq_pushinteger(v, enumValue++); \
		if (sq_newslot(v, -3) < 0) \
			Error(context, EXIT_FAILURE, "Error adding entry '%s' for enum '%s'.", enumFields[i], enumName); \
	} \
	if (sq_newslot(v, -3) < 0) \
		Error(context, EXIT_FAILURE, "Error adding enum '%s' to const table.", enumName); \
	sq_endconsttable(v); \

// Use this to return from any script func
#define SCRIPT_CHECK_AND_RETURN(v, val) \
	{ \
		SQSharedState* const sharedState = v->_sharedstate; \
		if (sharedState->_internal_error) { \
			\
				CSquirrelVM__ThrowError(sharedState->_scriptvm, v); \
				return SQ_ERROR; \
		} \
		return val; \
	}

///////////////////////////////////////////////////////////////////////////////
class VSquirrel : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CSquirrelVM::Init", CSquirrelVM__Init);
		LogFunAdr("CSquirrelVM::DestroySignalEntryListHead", CSquirrelVM__DestroySignalEntryListHead);

		LogFunAdr("CSquirrelVM::RegisterConstant", CSquirrelVM__RegisterConstant);
		LogFunAdr("CSquirrelVM::RegisterFunction", CSquirrelVM__RegisterFunction);
#ifndef CLIENT_DLL
		LogFunAdr("CSquirrelVM::PrecompileServerScripts", CSquirrelVM__PrecompileServerScripts);
#endif // !CLIENT_DLL
#ifndef DEDICATED
		LogFunAdr("CSquirrelVM::PrecompileClientScripts", CSquirrelVM__PrecompileClientScripts);
#endif // !DEDICATED
		LogFunAdr("CSquirrelVM::ExecuteFunction", CSquirrelVM__ExecuteFunction);
		LogFunAdr("CSquirrelVM::ExecuteCodeCallback", CSquirrelVM__ExecuteCodeCallback);
		LogFunAdr("CSquirrelVM::ThrowError", CSquirrelVM__ThrowError);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 0F 28 74 24 ?? 48 89 1D ?? ?? ?? ??").FollowNearCallSelf().GetPtr(CSquirrelVM__Init);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 50 44 8B 42").GetPtr(CSquirrelVM__DestroySignalEntryListHead);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 4C 8B").GetPtr(CSquirrelVM__RegisterConstant);
		g_GameDll.FindPatternSIMD("48 83 EC 38 45 0F B6 C8").GetPtr(CSquirrelVM__RegisterFunction);

#ifndef CLIENT_DLL
		// sv scripts.rson compiling
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 0F B6 F0 48 85 DB").FollowNearCallSelf().GetPtr(CSquirrelVM__PrecompileServerScripts);
#endif

#ifndef DEDICATED
		// cl/ui scripts.rson compiling
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 44 0F B6 F0 48 85 DB").FollowNearCallSelf().GetPtr(CSquirrelVM__PrecompileClientScripts);
#endif
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 83 FB 5C").FollowNearCallSelf().GetPtr(CSquirrelVM__ExecuteFunction);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? C6 47 1C 01").FollowNearCallSelf().GetPtr(CSquirrelVM__ExecuteCodeCallback);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? BB ?? ?? ?? ?? 8B C3").FollowNearCallSelf().GetPtr(CSquirrelVM__ThrowError);
	}
	virtual void GetVar(void) const
	{
#ifndef DEDICATED
		CMemory p_Script_InitUIVM = g_GameDll.FindPatternSIMD("48 83 EC ?? 80 3D ?? ?? ?? ?? ?? 74 ?? 32 C0");
		p_Script_InitUIVM.Offset(0x10).FindPatternSelf("80 3D").ResolveRelativeAddressSelf(2, 7).GetPtr(g_bUIScriptInitialized);
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};

#endif // VSQUIRREL_H
