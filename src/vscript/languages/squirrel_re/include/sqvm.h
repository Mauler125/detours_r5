#pragma once
#include "squirrel.h"
#include "sqstate.h"
#include "sqobject.h"

class CSquirrelVM;

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
enum class SQCONTEXT : SQInteger
{
	SERVER = 0,
	CLIENT,
	UI,
	NONE
};

struct SQVM : public CHAINABLE_OBJ
{
	void PrintObjVal(const SQObject* oin, SQObject* oout);

	void Pop();
	void Pop(SQInteger n);

	// push sqobjectptr on to the stack
	void Push(const SQObjectPtr& o);

	SQObjectPtr& Top();
	SQObjectPtr& PopGet();
	SQObjectPtr& GetUp(SQInteger n);
	SQObjectPtr& GetAt(SQInteger n);

	CSquirrelVM* GetScriptVM();
	SQChar* GetContextName();
	SQCONTEXT GetContext();
	eDLL_T GetNativeContext();

	// ================================= //
	_BYTE gap1C[8];
	void* _callstack;
	SQInteger _stacklevel;
	SQInteger _bottom;
	SQObjectPtr* _stackbase;
	SQSharedState* _sharedstate;
	char gap68[16];
	SQInteger _top;
	sqvector<SQObjectPtr> _stack;
	char gap_98[24];
	SQObjectPtr temp_reg;
	char gap_C8[32];
	SQObjectPtr _roottable;
	SQObjectPtr _lasterror;
	char gap_100[48];
	SQInteger _nnativecalls;
	SQBool _suspended;
	SQBool _suspended_root;
	SQInteger _callsstacksize;
	SQInteger _alloccallsstacksize;
	SQInteger suspended_traps;
};
static_assert(offsetof(SQVM, _top) == 0x78);
static_assert(offsetof(SQVM, _nnativecalls) == 0x130);

inline SQObjectPtr& stack_get(HSQUIRRELVM v, SQInteger idx) { return ((idx >= 0) ? (v->_stackbase[idx-1]) : (v->GetUp(idx))); }
#define _ss(_vm_) (_vm_)->_sharedstate

/* ==== SQUIRREL ======================================================================================================================================================== */
inline SQRESULT(*v_SQVM_PrintFunc)(HSQUIRRELVM v, SQChar* fmt, ...);
inline SQRESULT(*v_SQVM_sprintf)(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString);
inline size_t(*v_SQVM_GetErrorLine)(const SQChar* pszFile, SQInteger nLine, SQChar* pszContextBuf, SQInteger nBufLen);
inline SQRESULT(*v_SQVM_WarningCmd)(HSQUIRRELVM v, SQInteger a2);
inline void(*v_SQVM_CompileError)(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn);
inline void(*v_SQVM_LogicError)(SQBool bPrompt);
inline SQInteger(*v_SQVM_ScriptError)(const SQChar* pszFormat, ...);
inline SQInteger(*v_SQVM_RaiseError)(HSQUIRRELVM v, const SQChar* pszFormat, ...);
inline void(*v_SQVM_PrintObjVal)(HSQUIRRELVM v, const SQObject* oin, SQObject* oout);

SQRESULT SQVM_PrintFunc(HSQUIRRELVM v, SQChar* fmt, ...);
SQRESULT SQVM_sprintf(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString);
void SQVM_CompileError(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn);

///////////////////////////////////////////////////////////////////////////////
class VSquirrelVM : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("SQVM_PrintFunc", v_SQVM_PrintFunc);
		LogFunAdr("SQVM_sprintf", v_SQVM_sprintf);
		LogFunAdr("SQVM_GetErrorLine", v_SQVM_GetErrorLine);
		LogFunAdr("SQVM_WarningCmd", v_SQVM_WarningCmd);
		LogFunAdr("SQVM_CompileError", v_SQVM_CompileError);
		LogFunAdr("SQVM_LogicError", v_SQVM_LogicError);
		LogFunAdr("SQVM_ScriptError", v_SQVM_ScriptError);
		LogFunAdr("SQVM_RaiseError", v_SQVM_RaiseError);
		LogFunAdr("SQVM_PrintObjVal", v_SQVM_PrintObjVal);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 ?? ?? 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33").GetPtr(v_SQVM_PrintFunc);
		g_GameDll.FindPatternSIMD("4C 89 4C 24 20 44 89 44 24 18 89 54 24 10 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B").GetPtr(v_SQVM_sprintf);
		g_GameDll.FindPatternSIMD("48 8B C4 55 56 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 83 65 90 FC").GetPtr(v_SQVM_GetErrorLine);
		g_GameDll.FindPatternSIMD("48 83 EC 38 F2 0F 10 05 ?? ?? ?? ??").GetPtr(v_SQVM_LogicError);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 33 DB 48 8D 44 24 ?? 4C 8D 4C 24 ??").GetPtr(v_SQVM_WarningCmd);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 81 EC ?? ?? ?? ?? 48 8B D9 4C 8B F2").GetPtr(v_SQVM_CompileError);
		g_GameDll.FindPatternSIMD("E9 ?? ?? ?? ?? F7 D2").FollowNearCallSelf().GetPtr(v_SQVM_ScriptError);
		g_GameDll.FindPatternSIMD("48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? 53 56 57 48 83 EC 40").GetPtr(v_SQVM_RaiseError);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 50 45 33 ED").GetPtr(v_SQVM_PrintObjVal);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
