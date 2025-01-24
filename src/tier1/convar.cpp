//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "tier0/tslist.h"
#include "tier1/convar.h"

//-----------------------------------------------------------------------------
// Statically constructed list of ConCommandBases, 
// used for registering them with the ICVar interface
//-----------------------------------------------------------------------------
ConCommandBase* ConCommandBase::s_pConCommandBases = NULL;
IConCommandBaseAccessor* ConCommandBase::s_pAccessor = NULL;

static int s_nCVarFlag = 0;

// An unique identifier indicating which DLL this convar came from
static int s_nDLLIdentifier = -1;
static bool s_bRegistered = false;

class CDefaultAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase* const pVar)
	{
		// Link to engine's list instead
		g_pCVar->RegisterConCommand(pVar);
		return true;
	}
};

static CDefaultAccessor s_DefaultAccessor;

//-----------------------------------------------------------------------------
// Called by the framework to register ConCommandBases with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register(int nCVarFlag, IConCommandBaseAccessor* pAccessor)
{
	if (!g_pCVar || s_bRegistered)
		return;

	Assert(s_nDLLIdentifier < 0);
	s_bRegistered = true;
	s_nCVarFlag = nCVarFlag;
	s_nDLLIdentifier = g_pCVar->AllocateDLLIdentifier();

	ConCommandBase* pCur, * pNext;

	ConCommandBase::s_pAccessor = pAccessor ? pAccessor : &s_DefaultAccessor;
	pCur = ConCommandBase::s_pConCommandBases;

	while (pCur)
	{
		pNext = pCur->m_pNext;
		pCur->AddFlags(s_nCVarFlag);
		pCur->Init();
		pCur = pNext;
	}

	g_pCVar->ProcessQueuedMaterialThreadConVarSets();

	ConCommandBase::s_pConCommandBases = NULL;
}

void ConVar_Unregister()
{
	if (!g_pCVar || !s_bRegistered)
		return;

	Assert(s_nDLLIdentifier >= 0);

	// Do this after unregister!!!
	g_pCVar->UnregisterConCommands(s_nDLLIdentifier);
	s_nDLLIdentifier = -1;
	s_bRegistered = false;
}

bool ConVar_IsRegistered()
{
	return s_bRegistered;
}

ConCommandBase::~ConCommandBase(void)
{
	if (m_pszCustomUsageString)
	{
		delete[] m_pszCustomUsageString;
		m_pszCustomUsageString = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command 
//-----------------------------------------------------------------------------
bool ConCommandBase::IsCommand(void) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if ConCommand has requested flags.
// Input  : flag - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsFlagSet(const int flag) const
{
	return (flag & m_nFlags) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
//-----------------------------------------------------------------------------
void ConCommandBase::AddFlags(const int flags)
{
	m_nFlags |= flags;
}

//-----------------------------------------------------------------------------
// Purpose: removes specified flags
//-----------------------------------------------------------------------------
void ConCommandBase::RemoveFlags(const int flags)
{
	m_nFlags &= ~flags;
}

//-----------------------------------------------------------------------------
// Purpose: returns current flags
//-----------------------------------------------------------------------------
int ConCommandBase::GetFlags() const
{
	return m_nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Return name of the command/var
// Output : const char
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetName(void) const
{
	return m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetHelpText(void) const
{
	return m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char* ConCommandBase::GetUsageText(void) const
{
	return m_pszStaticUsageString;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the ConCommandBase usage text.
//-----------------------------------------------------------------------------
void ConCommandBase::SetUsageText(const char* const usageText)
{
	const char* const szCustomString = m_pszCustomUsageString;

	// If a custom usage string has already been set, release it first
	if (szCustomString)
		delete[] szCustomString;

	m_pszCustomUsageString = usageText
		? V_strdup(usageText)
		: nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Has this cvar been registered
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsRegistered(void) const
{
	return m_bRegistered;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the DLL identifier
//-----------------------------------------------------------------------------
int ConCommandBase::GetDLLIdentifier(void) const
{
	return s_nDLLIdentifier;
}

//-----------------------------------------------------------------------------
// Purpose: Used internally by OneTimeInit to initialize.
//-----------------------------------------------------------------------------
void ConCommandBase::Init()
{
	if (s_pAccessor)
	{
		s_pAccessor->RegisterConCommandBase(this);
	}
}

void ConCommandBase::Shutdown()
{
	if (g_pCVar)
	{
		g_pCVar->UnregisterConCommand(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
//			callback - 
//			*pHelpString - 
//			flags - 
//-----------------------------------------------------------------------------
ConCommandBase* ConCommandBase::Create(const char* pName, const char* pHelpString, int flags, const char* pszUsageString)
{
	m_bRegistered = false;

	// Name should be static data
	Assert(pName);
	m_pszName = pName;

	m_pszHelpString = pHelpString ? pHelpString : "";
	m_pszStaticUsageString = pszUsageString ? pszUsageString : "";

	m_pszCustomUsageString = nullptr;

	m_nFlags = flags;

	const int releaseMask = FCVAR_ARCHIVE|FCVAR_USERINFO|FCVAR_CHEAT|
		FCVAR_RELEASE|FCVAR_ARCHIVE_PLAYERPROFILE|FCVAR_CLIENTCMD_CAN_EXECUTE;

	if (!(m_nFlags & releaseMask))
	{
		m_nFlags |= FCVAR_DEVELOPMENTONLY;
	}

	if (!(m_nFlags & FCVAR_UNREGISTERED))
	{
		m_pNext = s_pConCommandBases;
		s_pConCommandBases = this;
	}
	else
	{
		// It's unregistered
		m_pNext = NULL;
	}

	// If s_pAccessor is already set (this ConVar is not a global variable),
	//  register it.
	if (s_pAccessor)
	{
		Init();
	}

	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const ConCommandBase
//-----------------------------------------------------------------------------
ConCommandBase* ConCommandBase::GetNext(void) const
{
	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: Copies string using local new/delete operators
// Input  : *szFrom - 
// Output : char
//-----------------------------------------------------------------------------
char* ConCommandBase::CopyString(const char* szFrom) const
{
	size_t nLen;
	char* szTo;

	nLen = strlen(szFrom);
	if (nLen <= 0)
	{
		szTo = new char[1];
		szTo[0] = 0;
	}
	else
	{
		szTo = new char[nLen + 1];
		memmove(szTo, szFrom, nLen + 1);
	}
	return szTo;
}

//-----------------------------------------------------------------------------
// Default do nothing function
//-----------------------------------------------------------------------------
void DefaultNullSub()
{
	; /*DO NOTHING*/
}

//-----------------------------------------------------------------------------
// Default console command autocompletion function
//-----------------------------------------------------------------------------
int DefaultCompletionFunc(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	return 0;
}

ConCommand::ConCommand(const char* pName, FnCommandCallbackV1_t callback, const char* pHelpString /*= 0*/, 
	int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/, const char* pszUsageString /*= 0*/)
{
	m_fnSupplementalFinishCallBack = DefaultNullSub;
	m_fnSupplementalCallback = (FnCommandSupplementalCallback_t)DefaultNullSub;

	// Set the callback
	m_fnCommandCallbackV1 = callback;
	m_bUsingNewCommandCallback = false;
	m_bUsingCommandCallbackInterface = false;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;

	// Setup the rest
	BaseClass::Create(pName, pHelpString, flags, pszUsageString);
}

ConCommand::ConCommand(const char* pName, FnCommandCallback_t callback, const char* pHelpString /*= 0*/, 
	int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/, const char* pszUsageString /*= 0*/)
{
	m_fnSupplementalFinishCallBack = DefaultNullSub;
	m_fnSupplementalCallback = (FnCommandSupplementalCallback_t)DefaultNullSub;

	// Set the callback
	m_fnCommandCallback = callback;
	m_bUsingNewCommandCallback = true;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;
	m_bUsingCommandCallbackInterface = false;

	// Setup the rest
	BaseClass::Create(pName, pHelpString, flags, pszUsageString);
}

ConCommand::ConCommand(const char* pName, ICommandCallback* pCallback, const char* pHelpString /*= 0*/, 
	int flags /*= 0*/, ICommandCompletionCallback* pCompletionCallback /*= 0*/, const char* pszUsageString /*= 0*/)
{
	m_fnSupplementalFinishCallBack = DefaultNullSub;
	m_fnSupplementalCallback = (FnCommandSupplementalCallback_t)DefaultNullSub;

	// Set the callback
	m_pCommandCallback = pCallback;
	m_bUsingNewCommandCallback = false;
	m_pCommandCompletionCallback = pCompletionCallback;
	m_bHasCompletionCallback = (pCompletionCallback != 0);
	m_bUsingCommandCallbackInterface = true;

	// Setup the rest
	BaseClass::Create(pName, pHelpString, flags, pszUsageString);
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
ConCommand::~ConCommand()
{
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command 
//-----------------------------------------------------------------------------
bool ConCommand::IsCommand(void) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: calls the autocompletion method to get autocompletion suggestions
//-----------------------------------------------------------------------------
int	ConCommand::AutoCompleteSuggest(const char* partial, CUtlVector< CUtlString >& commands)
{
	if (m_bUsingCommandCallbackInterface)
	{
		if (!m_pCommandCompletionCallback)
			return 0;

		return m_pCommandCompletionCallback->CommandCompletionCallback(partial, commands);
	}

	Assert(m_fnCompletionCallback);

	if (!m_fnCompletionCallback)
		return 0;

	char rgpchCommands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH];
	const int iret = (m_fnCompletionCallback)(partial, rgpchCommands);

	for (int i = 0; i < iret; ++i)
	{
		const CUtlString str = rgpchCommands[i];
		commands.AddToTail(str);
	}

	return iret;
}

//-----------------------------------------------------------------------------
// Returns true if the console command can autocomplete 
//-----------------------------------------------------------------------------
bool ConCommand::CanAutoComplete(void) const
{
	return m_bHasCompletionCallback;
}

//-----------------------------------------------------------------------------
// Purpose: invoke the function if there is one
//-----------------------------------------------------------------------------
void ConCommand::Dispatch(const ECommandTarget_t target, const CCommand& command, const bool bCallSupplemental)
{
	// If this is still false towards the end of this function, there is a bug
	// somewhere in code
	bool bRanCallback = false;

	if (m_bUsingNewCommandCallback)
	{
		if (m_fnCommandCallback)
		{
			(*m_fnCommandCallback)(command);
			bRanCallback = true;
		}
	}
	else if (m_bUsingCommandCallbackInterface)
	{
		if (m_pCommandCallback)
		{
			m_pCommandCallback->CommandCallback(command);
			bRanCallback = true;
		}
	}
	else
	{
		if (m_fnCommandCallbackV1)
		{
			(*m_fnCommandCallbackV1)();
			bRanCallback = true;
		}
	}

	if (bCallSupplemental)
	{
		m_fnSupplementalCallback(command, m_fnSupplementalFinishCallBack);
	}

	// Command without callback!!!
	AssertMsg(bRanCallback, "Encountered ConCommand '%s' without a callback!\n", GetName());
}

//-----------------------------------------------------------------------------
// Various constructors
//-----------------------------------------------------------------------------
ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags /* = 0 */)
{
	Create(pName, pDefaultValue, flags);
}

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags, const char* pHelpString, const char* pUsageString)
{
	Create(pName, pDefaultValue, flags, pHelpString, false, 0.0, false, 0.0, nullptr, pUsageString);
}

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags, const char* pHelpString,
	bool bMin, float fMin, bool bMax, float fMax, const char* pUsageString)
{
	Create(pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax, nullptr, pUsageString);
}

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags, const char* pHelpString,
	FnChangeCallback_t callback, const char* pUsageString)
{
	Create(pName, pDefaultValue, flags, pHelpString, false, 0.0, false, 0.0, callback, pUsageString);
}

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags, const char* pHelpString,
	bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t callback, const char* pUsageString)
{
	Create(pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax, callback, pUsageString);
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
ConVar::~ConVar(void)
{
	if (m_Value.m_pszString)
	{
		delete[] m_Value.m_pszString;
		m_Value.m_pszString = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ConVar::IsCommand(void) const
{
	return false;
}

bool ConVar::IsFlagSet(int flag) const
{
	return (flag & m_pParent->m_nFlags) ? true : false;
}

void ConVar::AddFlags(int flags)
{
	m_pParent->m_nFlags |= flags;
}

int ConVar::GetFlags() const
{
	return m_pParent->m_nFlags;
}

const char* ConVar::GetName(void) const
{
	return m_pParent->m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the base ConVar name.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetBaseName(void) const
{
	return m_pParent->m_pszName;
}

int ConVar::GetSplitScreenPlayerSlot(void) const
{
	// Default implementation (certain FCVAR_USERINFO derive a new type of convar and set this)
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar help text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetHelpText(void) const
{
	return m_pParent->m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar usage text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetUsageText(void) const
{
	const char* const szCustomString = m_pParent->m_pszCustomUsageString;

	// If a custom usage string has been set, return that instead
	if (szCustomString)
		return szCustomString;

	return m_pParent->m_pszStaticUsageString;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the ConVar usage text.
//-----------------------------------------------------------------------------
void ConVar::SetUsageText(const char* const usageText)
{
	const char* const szCustomString = m_pParent->m_pszCustomUsageString;

	// If a custom usage string has been set, return that instead
	if (szCustomString)
		delete[] szCustomString;

	m_pParent->m_pszCustomUsageString = usageText
		? V_strdup(usageText)
		: nullptr;
}

bool ConVar::IsRegistered(void) const
{
	return m_pParent->m_bRegistered;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value from string.
// Input  : *pszValue - 
//-----------------------------------------------------------------------------
bool ConVar::InternalSetColorFromString(const char* value)
{
	bool bColor = false;

	// Try pulling RGBA color values out of the string
	int nRGBA[4];
	int nParamsRead = sscanf(value, "%i %i %i %i", 
		&(nRGBA[0]), &(nRGBA[1]), &(nRGBA[2]), &(nRGBA[3]));

	if (nParamsRead >= 3)
	{
		// This is probably a color!
		if (nParamsRead == 3)
		{
			// Assume they wanted full alpha
			nRGBA[3] = 255;
		}

		if (nRGBA[0] >= 0 && nRGBA[0] <= 255 &&
			nRGBA[1] >= 0 && nRGBA[1] <= 255 &&
			nRGBA[2] >= 0 && nRGBA[2] <= 255 &&
			nRGBA[3] >= 0 && nRGBA[3] <= 255)
		{
			// This is definitely a color!
			bColor = true;

			// Stuff all the values into each byte of our int
			unsigned char* pColorElement = ((unsigned char*)&m_Value.m_nValue);
			pColorElement[0] = (unsigned char)nRGBA[0];
			pColorElement[1] = (unsigned char)nRGBA[1];
			pColorElement[2] = (unsigned char)nRGBA[2];
			pColorElement[3] = (unsigned char)nRGBA[3];

			// Copy that value into a float (even though this has little meaning)
			m_Value.m_fValue = (float)(m_Value.m_nValue);
		}
	}

	return bColor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetValue(const char* value)
{
	if (IsFlagSet(FCVAR_MATERIAL_THREAD_MASK))
	{
		if (g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed())
		{
			g_pCVar->QueueMaterialThreadSetValue(this, value);

			if (!IsFlagSet(FCVAR_STUDIO_SYSTEM))
				return;
		}
	}

	Assert(m_pParent == this); // Only valid for root convars.

	char  tempVal[32];
	const char* newVal = value;

	if (!newVal)
		newVal = "";

	if (!InternalSetColorFromString(newVal))
	{
		// Not a color, do the standard thing
		float fNewValue = (float)atof(newVal);
		if (!IsFinite(fNewValue))
		{
			DevWarning(eDLL_T::COMMON, "Warning: %s = '%s' is infinite, clamping value.\n", GetName(), newVal);
			fNewValue = FLT_MAX;
		}

		if (ClampValue(fNewValue))
		{
			V_snprintf(tempVal, sizeof(tempVal), "%f", fNewValue);
			newVal = tempVal;
		}

		// Redetermine value
		m_Value.m_fValue = fNewValue;
		m_Value.m_nValue = (int)(m_Value.m_fValue);
	}

	// Note(amos): send the value that can potentially be null,
	// since we want to track those too
	TrackValueChange(value);

	if (!(m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		ChangeStringValue(newVal);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetFloatValue(float fNewValue)
{
	if (fNewValue == m_Value.m_fValue)
		return;

	if (IsFlagSet(FCVAR_MATERIAL_THREAD_MASK))
	{
		if (g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed())
		{
			g_pCVar->QueueMaterialThreadSetValue(this, fNewValue);
			return;
		}
	}

	Assert(m_pParent == this); // Only valid for root convars.

	// Check bounds
	ClampValue(fNewValue);

	// Redetermine value
	m_Value.m_fValue = fNewValue;
	m_Value.m_nValue = (int)m_Value.m_fValue;

	if (!(m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char tempVal[32];
		V_snprintf(tempVal, sizeof(tempVal), "%f", m_Value.m_fValue);
		ChangeStringValue(tempVal);
	}
	else
	{
		Assert(m_fnChangeCallbacks.Count() == 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetIntValue(int nValue)
{
	if (nValue == m_Value.m_nValue)
		return;

	if (IsFlagSet(FCVAR_MATERIAL_THREAD_MASK))
	{
		if (g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed())
		{
			g_pCVar->QueueMaterialThreadSetValue(this, nValue);
			return;
		}
	}

	Assert(m_pParent == this); // Only valid for root convars.

	float fValue = (float)nValue;
	if (ClampValue(fValue))
	{
		nValue = (int)(fValue);
	}

	// Redetermine value
	m_Value.m_fValue = fValue;
	m_Value.m_nValue = nValue;

	if (!(m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char tempVal[32];
		V_snprintf(tempVal, sizeof(tempVal), "%d", m_Value.m_nValue);
		ChangeStringValue(tempVal);
	}
	else
	{
		Assert(m_fnChangeCallbacks.Count() == 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMaxVal - 
//-----------------------------------------------------------------------------
void ConVar::SetMax(float flMaxVal)
{
	m_pParent->m_fMaxVal = flMaxVal;
	m_pParent->m_bHasMax = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMinVal - 
//-----------------------------------------------------------------------------
void ConVar::SetMin(float flMinVal)
{
	m_pParent->m_fMinVal = flMinVal;
	m_pParent->m_bHasMin = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMinVal - 
// Output : true if there is a min set.
//-----------------------------------------------------------------------------
bool ConVar::GetMin(float& flMinVal) const
{
	flMinVal = m_pParent->m_fMinVal;
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMaxVal - 
// Output : true if there is a max set.
//-----------------------------------------------------------------------------
bool ConVar::GetMax(float& flMaxVal) const
{
	flMaxVal = m_pParent->m_fMaxVal;
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: returns the min value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMinValue(void) const
{
	return m_pParent->m_fMinVal;
}

//-----------------------------------------------------------------------------
// Purpose: returns the max value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMaxValue(void) const
{
	return m_pParent->m_fMaxVal;
}

//-----------------------------------------------------------------------------
// Purpose: checks if ConVar has min value.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::HasMin(void) const
{
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: checks if ConVar has max value.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::HasMax(void) const
{
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar int value.
// Input  : nValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(int nValue)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetIntValue(nValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar float value.
// Input  : flValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(float flValue)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetFloatValue(flValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar string value.
// Input  : *szValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char* pszValue)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetValue(pszValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value.
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(Color value)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetColorValue(value);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetColorValue(Color value)
{
	// Stuff color values into an int
	int nValue = 0;

	unsigned char* pColorElement = (reinterpret_cast<unsigned char*>(&nValue));
	pColorElement[0] = value[0];
	pColorElement[1] = value[1];
	pColorElement[2] = value[2];
	pColorElement[3] = value[3];

	// Call the int internal set
	InternalSetIntValue(nValue);
}

//-----------------------------------------------------------------------------
// Purpose: Check whether to clamp and then perform clamp
// Input  : value - 
// Output : Returns true if value changed
//-----------------------------------------------------------------------------
bool ConVar::ClampValue(float& value)
{
	if (m_bHasMin && (value < m_fMinVal))
	{
		value = m_fMinVal;
		return true;
	}

	if (m_bHasMax && (value > m_fMaxVal))
	{
		value = m_fMaxVal;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tempVal - 
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValue(const char* tempVal)
{
	Assert(!(m_nFlags & FCVAR_NEVER_AS_STRING));

	char* pszOldValue = (char*)stackalloc(m_Value.m_iStringLength);
	memcpy(pszOldValue, m_Value.m_pszString, m_Value.m_iStringLength);

	const size_t len = V_strlen(tempVal) + 1;

	if (len > m_Value.m_iStringLength)
	{
		if (m_Value.m_pszString)
		{
			delete[] m_Value.m_pszString;
		}

		m_Value.m_pszString = new char[len];
		m_Value.m_iStringLength = len;
	}

	memcpy(m_Value.m_pszString, tempVal, len);

	// Invoke any necessary callback function
	for (int i = 0; i < m_fnChangeCallbacks.Count(); ++i)
	{
		const CVChange_t& targetCB = m_fnChangeCallbacks[i];
		targetCB.m_pCallback(this, pszOldValue, NULL, targetCB.m_pUserData);
	}

	if (g_pCVar)
	{
		g_pCVar->CallGlobalChangeCallbacks(this, pszOldValue);
	}

	stackfree(pszOldValue);
}

//-----------------------------------------------------------------------------
// Purpose: Private creation
//-----------------------------------------------------------------------------
void ConVar::Create(const char* pName, const char* pDefaultValue, int flags /*= 0*/,
	const char* pHelpString /*= NULL*/, bool bMin /*= false*/, float fMin /*= 0.0*/,
	bool bMax /*= false*/, float fMax /*= false*/, FnChangeCallback_t callback /*= NULL*/,
	const char* pszUsageString /*= NULL*/)
{
	// Name should be static data
	m_pszDefaultValue = pDefaultValue ? pDefaultValue : "";
	Assert(m_pszDefaultValue);

	m_bHasMin = bMin;
	m_fMinVal = fMin;
	m_bHasMax = bMax;
	m_fMaxVal = fMax;

	m_pParent = this;

	if (callback)
	{
		InstallChangeCallback(callback, NULL, false);
	}

	m_Value.m_iStringLength = strlen(m_pszDefaultValue) + 1;
	m_Value.m_pszString = new char[m_Value.m_iStringLength];
	memcpy(m_Value.m_pszString, m_pszDefaultValue, m_Value.m_iStringLength);

	if (!InternalSetColorFromString(m_Value.m_pszString))
	{
		m_Value.m_fValue = (float)atof(m_Value.m_pszString);
		if (!IsFinite(m_Value.m_fValue))
		{
			DevWarning(eDLL_T::COMMON, "ConVar(%s) defined with infinite float value (%s).\n", pName, m_Value.m_pszString);
			m_Value.m_fValue = FLT_MAX;
			Assert(0);
		}

		// Bounds Check, should never happen, if it does, no big deal
		if (m_bHasMin && (m_Value.m_fValue < m_fMinVal))
		{
			Assert(0);
		}

		if (m_bHasMax && (m_Value.m_fValue > m_fMaxVal))
		{
			Assert(0);
		}

		m_Value.m_nValue = (int)m_Value.m_fValue;
	}


	TrackValueChange(m_Value.m_pszString);

	// Only 1 of the 2 can be set on a ConVar, both means there is a bug in
	// your code, fix it!
	const int nFlagsToCheck = (FCVAR_ARCHIVE | FCVAR_ARCHIVE_PLAYERPROFILE);
	if ((flags & nFlagsToCheck) == nFlagsToCheck)
	{
		Error(eDLL_T::COMMON, EXIT_FAILURE,
			"Convar '%s' is flagged as both FCVAR_ARCHIVE and FCVAR_ARCHIVE_PLAYERPROFILE.\n", pName);
	}

	BaseClass::Create(pName, pHelpString, flags, pszUsageString);
}

//-----------------------------------------------------------------------------
// Purpose: Reset to default value.
//-----------------------------------------------------------------------------
void ConVar::Revert(void)
{
	SetValue(m_pszDefaultValue);
}

//-----------------------------------------------------------------------------
// Purpose: returns the default ConVar value.
// Output : const char
//-----------------------------------------------------------------------------
const char* ConVar::GetDefault(void) const
{
	return m_pParent->m_pszDefaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: sets the default ConVar value.
// Input  : *pszDefault -
//-----------------------------------------------------------------------------
void ConVar::SetDefault(const char* pszDefault)
{
	static const char* pszEmpty = "";
	m_pszDefaultValue = pszDefault ? pszDefault : pszEmpty;
	assert(m_pszDefaultValue);
}

//-----------------------------------------------------------------------------
// Purpose: Install a change callback (there shouldn't already be one....)
// Input  : callback - 
//			userData - 
//			bInvoke - 
//-----------------------------------------------------------------------------
void ConVar::InstallChangeCallback(FnChangeCallback_t callback, ChangeUserData_t userData, bool bInvoke /*=true*/)
{
	if (!callback)
	{
		Warning(eDLL_T::COMMON, "%s: Called with NULL callback; ignoring!!!\n",
			__FUNCTION__);
		return;
	}

	const CVChange_t newCB{ callback, userData };

	if (m_pParent->m_fnChangeCallbacks.Find(newCB)
		!= m_pParent->m_fnChangeCallbacks.InvalidIndex())
	{
		// Same ptr added twice, sigh...
		Warning(eDLL_T::COMMON, "%s: Ignoring duplicate change callback!!!\n",
			__FUNCTION__);
		return;
	}

	m_pParent->m_fnChangeCallbacks.AddToTail(newCB);

	// Call it immediately to set the initial value...
	if (bInvoke)
	{
		callback(this, m_Value.m_pszString, NULL, userData);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Install a change callback (there shouldn't already be one....)
// Input  : callback - 
//-----------------------------------------------------------------------------
void ConVar::RemoveChangeCallback(FnChangeCallback_t callback, ChangeUserData_t userData)
{
	const CVChange_t targetCB{ callback, userData };
	m_pParent->m_fnChangeCallbacks.FindAndRemove(targetCB);
}
