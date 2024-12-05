//===============================================================================//
//
// Purpose: VSquirrel VM
//
//===============================================================================//
#include "tier0/fasttimer.h"
#include "vscript/vscript.h"
#include "pluginsystem/modsystem.h"
#include "sqclosure.h"
#include "sqfuncproto.h"
#include "sqstring.h"
#include "vsquirrel.h"

static ConVar script_profile_codecalls("script_profile_codecalls", "0", FCVAR_DEVELOPMENTONLY, "Prints duration of native calls to script functions.", "0 = none, 1 = slow calls, 2 = all ( !slower! )");

// Callbacks for registering abstracted script functions.
void(*ServerScriptRegister_Callback)(CSquirrelVM* const s) = nullptr;
void(*ClientScriptRegister_Callback)(CSquirrelVM* const s) = nullptr;
void(*UiScriptRegister_Callback)(CSquirrelVM* const s) = nullptr;

// Callbacks for registering script enums.
void(*ServerScriptRegisterEnum_Callback)(CSquirrelVM* const s) = nullptr;
void(*ClientScriptRegisterEnum_Callback)(CSquirrelVM* const s) = nullptr;
void(*UIScriptRegisterEnum_Callback)(CSquirrelVM* const s) = nullptr;

// Admin panel functions, NULL on dedicated and client only builds.
void(*UiServerScriptRegister_Callback)(CSquirrelVM* const s) = nullptr;
void(*UiAdminPanelScriptRegister_Callback)(CSquirrelVM* const s) = nullptr;

// Registering constants in scripts.
void(*ScriptConstantRegister_Callback)(CSquirrelVM* const s) = nullptr;

//---------------------------------------------------------------------------------
// Purpose: Initialises a Squirrel VM instance
// Output : True on success, false on failure
//---------------------------------------------------------------------------------
bool CSquirrelVM::Init(CSquirrelVM* s, SQCONTEXT context, SQFloat curTime)
{
	// original func always returns true, added check just in case.
	if (!CSquirrelVM__Init(s, context, curTime))
	{
		return false;
	}

	Msg((eDLL_T)context, "Created %s VM: '0x%p'\n", s->GetVM()->_sharedstate->_contextname, s);

	switch (context)
	{
	case SQCONTEXT::SERVER:
		g_pServerScript = s;

		if (ServerScriptRegister_Callback)
			ServerScriptRegister_Callback(s);

		break;
	case SQCONTEXT::CLIENT:
		g_pClientScript = s;

		if (ClientScriptRegister_Callback)
			ClientScriptRegister_Callback(s);

		break;
	case SQCONTEXT::UI:
		g_pUIScript = s;

		if (UiScriptRegister_Callback)
			UiScriptRegister_Callback(s);

		if (UiServerScriptRegister_Callback)
			UiServerScriptRegister_Callback(s);

		if (UiAdminPanelScriptRegister_Callback)
			UiAdminPanelScriptRegister_Callback(s);

		break;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: destroys the signal entry list head
// Input  : *s - 
//			v - 
//			f - 
// Output : true on success, false otherwise
//---------------------------------------------------------------------------------
bool CSquirrelVM::DestroySignalEntryListHead(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f)
{
	SQBool result = CSquirrelVM__DestroySignalEntryListHead(s, v, f);
	s->RegisterConstant("DEVELOPER", developer->GetInt());

	// Must have one.
	Assert(ScriptConstantRegister_Callback);
	ScriptConstantRegister_Callback(s);

	return result;
}

//---------------------------------------------------------------------------------
// Purpose: registers a global constant
// Input  : *name - 
//			value - 
//---------------------------------------------------------------------------------
SQRESULT CSquirrelVM::RegisterConstant(const SQChar* name, SQInteger value)
{
	return CSquirrelVM__RegisterConstant(this, name, value);
}

//---------------------------------------------------------------------------------
// Purpose: runs text as script on the VM
// Input  : *script - 
// Output : true on success, false otherwise
//---------------------------------------------------------------------------------
bool CSquirrelVM::Run(const SQChar* const script)
{
	Assert(m_hVM);

	bool success = false;
	SQBufState bufState(script);

	if (SQ_SUCCEEDED(sq_compilebuffer(m_hVM, &bufState, "unnamed", -1, SQTrue)))
	{
		SQObject hScript;
		sq_getstackobj(m_hVM, -1, &hScript);

		sq_addref(m_hVM, &hScript);
		sq_pop(m_hVM, 1);

		if (ExecuteFunction((HSCRIPT)&hScript, NULL, 0, NULL, NULL) == SCRIPT_DONE)
			success = true;

		sq_release(m_hVM, &hScript);
	}

	return success;
}

//---------------------------------------------------------------------------------
// Purpose: executes a function by handle
// Input  : hFunction - 
//			*pArgs - 
//			nArgs - 
//			*pReturn - 
//			hScope - 
// Output : SCRIPT_DONE on success, SCRIPT_ERROR otherwise
//---------------------------------------------------------------------------------
ScriptStatus_t CSquirrelVM::ExecuteFunction(HSCRIPT hFunction, void** pArgs, unsigned int nArgs, void* pReturn, HSCRIPT hScope)
{
	const SQObjectPtr* const f = reinterpret_cast<SQObjectPtr*>(hFunction);

	const SQClosure* const closure = _closure(*f);
	const SQFunctionProto* const fp = _funcproto(closure->_function);

	// Only bother doing a timer if the funcproto is not nullptr.
	// This should always be true unless something has gone badly wrong.
	const bool hasFuncProto = fp != nullptr;
	const char* functionName = hasFuncProto ? _stringval(fp->_funcname) : "(no funcproto)";

	Assert(hasFuncProto);
	CFastTimer callTimer;

	// Start a timer for any named function call
	if (hasFuncProto)
		callTimer.Start();

	// NOTE: pArgs and pReturn are most likely of type 'ScriptVariant_t', needs to be reversed.
	const ScriptStatus_t result = CSquirrelVM__ExecuteFunction(this, hFunction, pArgs, nArgs, pReturn, hScope);

	if (hasFuncProto)
	{
		// End the timer as soon as possible after the call has completed to make sure the time is accurate.
		callTimer.End();

		const int printMode = script_profile_codecalls.GetInt();

		// If print mode is not "none"
		if (printMode > 0)
		{
			const double durationMS = callTimer.GetDuration().GetMillisecondsF();

			// If printMode is set to "all", or the duration is greater than or equal to one millisecond.
			if (printMode == 2 || (printMode == 1 && durationMS >= 1.f))
				Msg(this->GetNativeContext(), "Script function '%s' took %.3fms\n", functionName, durationMS);
		}
	}

	return result;
}

ScriptStatus_t Script_ExecuteFunction(CSquirrelVM* s, HSCRIPT hFunction, void** pArgs, unsigned int nArgs, void* pReturn, HSCRIPT hScope)
{
	return s->ExecuteFunction(hFunction, pArgs, nArgs, pReturn, hScope);
}

//---------------------------------------------------------------------------------
// Purpose: executes a code callback
// Input  : *name - 
// Output : true on success, false otherwise
//---------------------------------------------------------------------------------
bool CSquirrelVM::ExecuteCodeCallback(const SQChar* const name)
{
	return CSquirrelVM__ExecuteCodeCallback(this, name);
}

//---------------------------------------------------------------------------------
// Purpose: registers a code function
// Input  : *s - 
//			*scriptName - 
//			*nativeName - 
//			*helpString - 
//			*returnString - 
//			*parameters - 
//			*function - 
//---------------------------------------------------------------------------------
SQRESULT CSquirrelVM::RegisterFunction(const SQChar* scriptName, const SQChar* nativeName,
	const SQChar* helpString, const SQChar* returnString, const SQChar* parameters, void* function)
{
	ScriptFunctionBinding_t binding;
	binding.Init(scriptName, nativeName, helpString, returnString, parameters, 5, function);

	SQRESULT results = CSquirrelVM__RegisterFunction(this, &binding, 1);
	return results;
}

//---------------------------------------------------------------------------------
// Purpose: sets current VM as the global precompiler
// Input  : *name - 
//			value - 
//---------------------------------------------------------------------------------
void CSquirrelVM::SetAsCompiler(RSON::Node_t* rson)
{
	const SQCONTEXT context = GetContext();
	switch (context)
	{
	case SQCONTEXT::SERVER:
	{
		v_Script_SetServerPrecompiler(context, rson);
		break;
	}
	case SQCONTEXT::CLIENT:
	case SQCONTEXT::UI:
	{
		v_Script_SetClientPrecompiler(context, rson);
		break;
	}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Precompiles mod scripts
//---------------------------------------------------------------------------------
void CSquirrelVM::CompileModScripts()
{
	FOR_EACH_VEC(ModSystem()->GetModList(), i)
	{
		const CModSystem::ModInstance_t* mod = ModSystem()->GetModList()[i];

		if (!mod->IsEnabled())
			continue;

		if (!mod->m_bHasScriptCompileList)
			continue;

		// allocs parsed rson buffer
		RSON::Node_t* rson = mod->LoadScriptCompileList();

		if (!rson)
			Error(GetNativeContext(), NO_ERROR, 
				"%s: Failed to load RSON file '%s'\n", 
				__FUNCTION__, mod->GetScriptCompileListPath().Get());

		const char* scriptPathArray[MAX_PRECOMPILED_SCRIPTS];
		int scriptCount = 0;

		SetAsCompiler(rson);

		if (Script_ParseScriptList(
			GetContext(),
			mod->GetScriptCompileListPath().Get(),
			rson,
			(char**)scriptPathArray, &scriptCount,
			nullptr, 0))
		{
			std::vector<char*> newScriptPaths;
			for (int j = 0; j < scriptCount; ++j)
			{
				// add "::MOD::" to the start of the script path so it can be
				// identified from Script_LoadScript later, this is so we can
				// avoid script naming conflicts by removing the engine's
				// forced directory of "scripts/vscripts/" and adding the mod
				// path to the start
				CUtlString scriptPath;
				scriptPath.Format("%s%s%s%s",
					MOD_SCRIPT_PATH_IDENTIFIER, mod->GetBasePath().Get(),
					GAME_SCRIPT_PATH, scriptPathArray[j]);

				char* pszScriptPath = _strdup(scriptPath.Get());

				// normalise slash direction
				V_FixSlashes(pszScriptPath);

				newScriptPaths.emplace_back(pszScriptPath);
				scriptPathArray[j] = pszScriptPath;
			}

			switch (GetContext())
			{
			case SQCONTEXT::SERVER:
			{
				CSquirrelVM__PrecompileServerScripts(this, GetContext(), (char**)scriptPathArray, scriptCount);
				break;
			}
			case SQCONTEXT::CLIENT:
			case SQCONTEXT::UI:
			{
				CSquirrelVM__PrecompileClientScripts(this, GetContext(), (char**)scriptPathArray, scriptCount);
				break;
			}
			}

			// clean up our allocated script paths
			for (char* path : newScriptPaths)
			{
				free(path);
			}
		}

		RSON_Free(rson, AlignedMemAlloc());
		AlignedMemAlloc()->Free(rson);
	}
}

//---------------------------------------------------------------------------------
void VSquirrel::Detour(const bool bAttach) const
{
	DetourSetup(&CSquirrelVM__Init, &CSquirrelVM::Init, bAttach);
	DetourSetup(&CSquirrelVM__DestroySignalEntryListHead, &CSquirrelVM::DestroySignalEntryListHead, bAttach);
	DetourSetup(&CSquirrelVM__ExecuteFunction, &Script_ExecuteFunction, bAttach);
}
