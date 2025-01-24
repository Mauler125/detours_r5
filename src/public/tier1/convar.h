//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $NoKeywords: $
//===========================================================================//

#ifndef CONVAR_H
#define CONVAR_H

#include "mathlib/color.h"
#include "public/iconvar.h"
#include "public/iconcommand.h"
#include "tier1/cmd.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"

//-----------------------------------------------------------------------------
// Purpose: The base console invoked command/cvar interface
//-----------------------------------------------------------------------------
class ConCommandBase
{
public:
	virtual ~ConCommandBase(void);

	virtual bool IsCommand(void) const;
	virtual bool IsFlagSet(const int nFlags) const;

	virtual void AddFlags(const int nFlags);
	virtual void RemoveFlags(const int nFlags);

	virtual int GetFlags(void) const;
	virtual const char* GetName(void) const;
	virtual const char* GetHelpText(void) const;

	virtual const char* GetUsageText(void) const;
	virtual void        SetUsageText(const char* const usageText);

	virtual bool IsRegistered(void) const;

	virtual int GetDLLIdentifier() const;
	virtual ConCommandBase* Create(const char* szName, const char* szHelpString,
		int nFlags, const char* pszUsageString);

	virtual void				Init();
	void						Shutdown();

	ConCommandBase* GetNext(void) const;
	char* CopyString(const char* szFrom) const;

//private:
	// Next ConVar in chain
	// Prior to register, it points to the next convar in the DLL.
	// Once registered, though, m_pNext is reset to point to the next
	// convar in the global list
	ConCommandBase*          m_pNext;          //0x0008

	// Has the cvar been added to the global list?
	bool                     m_bRegistered;    //0x0010

	// Static data.
	const char*              m_pszName;        //0x0018
	const char*              m_pszHelpString;  //0x0020

	const char*              m_pszStaticUsageString; //0x0028
	const char*              m_pszCustomUsageString; //0x0030

	// ConVar flags
	int                      m_nFlags;         //0x0038

	// ConVars add themselves to this list for the executable. 
	// Then ConVar_Register runs through  all the console variables 
	// and registers them into a global list stored in vstdlib.dll
	static ConCommandBase* s_pConCommandBases;

	// ConVars in this executable use this 'global' to access values.
	static IConCommandBaseAccessor* s_pAccessor;
};
static_assert(sizeof(ConCommandBase) == 0x40);

///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: The console invoked command
//-----------------------------------------------------------------------------
class ConCommand : public ConCommandBase
{
	friend class CCvar;
public:
	typedef ConCommandBase BaseClass;

	ConCommand(const char* pName, FnCommandCallbackV1_t callback,
		const char* pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0, const char* pszUsageString = 0);
	ConCommand(const char* pName, FnCommandCallback_t callback,
		const char* pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0, const char* pszUsageString = 0);
	ConCommand(const char* pName, ICommandCallback* pCallback,
		const char* pHelpString = 0, int flags = 0, ICommandCompletionCallback* pCommandCompletionCallback = 0, const char* pszUsageString = 0);

	virtual ~ConCommand(void);

	virtual	bool IsCommand(void) const;

	virtual int AutoCompleteSuggest(const char* partial, CUtlVector< CUtlString >& commands);
	virtual bool CanAutoComplete(void) const;

	// Invoke the function
	virtual void Dispatch(const ECommandTarget_t target, const CCommand& command, const bool bCallSupplemental);

//private:
	FnCommandSupplementalFinishCallback_t m_fnSupplementalFinishCallBack; //0x0040
	FnCommandSupplementalCallback_t m_fnSupplementalCallback;  //0x0048

	// NOTE: To maintain backward compatibility, we have to be very careful:
	// All public virtual methods must appear in the same order always
	// since engine code will be calling into this code, which *does not match*
	// in the mod code; it's using slightly different, but compatible versions
	// of this class. Also: Be very careful about adding new fields to this class.
	// Those fields will not exist in the version of this class that is instanced
	// in mod code.

	// Call this function when executing the command
	union
	{
		FnCommandCallbackV1_t m_fnCommandCallbackV1;
		FnCommandCallback_t m_fnCommandCallback;
		ICommandCallback* m_pCommandCallback;
	};

	union
	{
		FnCommandCompletionCallback m_fnCompletionCallback;
		ICommandCompletionCallback* m_pCommandCompletionCallback;
	};

	bool m_bHasCompletionCallback : 1;
	bool m_bUsingNewCommandCallback : 1;
	bool m_bUsingCommandCallbackInterface : 1;
};
static_assert(sizeof(ConCommand) == 0x68);

///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: A console variable
//-----------------------------------------------------------------------------
class ConVar : public ConCommandBase, public IConVar
{
	friend class CCvar;
	friend class ConVarRef;

public:
	typedef ConCommandBase BaseClass;

	ConVar(const char* pName, const char* pDefaultValue, int flags = 0);

	ConVar(const char* pName, const char* pDefaultValue, int flags,
		const char* pHelpString, const char* pUsageString = 0);
	ConVar(const char* pName, const char* pDefaultValue, int flags,
		const char* pHelpString, bool bMin, float fMin, bool bMax, float fMax, const char* pUsageString = 0);
	ConVar(const char* pName, const char* pDefaultValue, int flags,
		const char* pHelpString, FnChangeCallback_t callback, const char* pUsageString = 0);
	ConVar(const char* pName, const char* pDefaultValue, int flags,
		const char* pHelpString, bool bMin, float fMin, bool bMax, float fMax,
		FnChangeCallback_t callback, const char* pUsageString = 0);

	virtual ~ConVar(void);

	virtual bool                IsCommand(void) const;
	virtual bool                IsFlagSet(const int flag) const;

	virtual void                AddFlags(const int flags);
	virtual int                 GetFlags() const;

	// Return name of cvar
	virtual const char*         GetName(void) const;

	// Return name of command (usually == GetName(), except in case of FCVAR_SS_ADDED vars
	virtual const char*         GetBaseName(void) const;
	virtual int                 GetSplitScreenPlayerSlot() const;

	// Return help text for cvar
	virtual const char*         GetHelpText(void) const;

	virtual const char*         GetUsageText(void) const;
	virtual void                SetUsageText(const char* const usageText);

	virtual bool                IsRegistered(void) const;

	// Install a change callback (there shouldn't already be one....)
	void                        InstallChangeCallback(FnChangeCallback_t callback, ChangeUserData_t userData, bool bInvoke);
	void                        RemoveChangeCallback(FnChangeCallback_t callback, ChangeUserData_t userData);

	int                         GetChangeCallbackCount() const { return m_pParent->m_fnChangeCallbacks.Count(); }
	FnChangeCallback_t          GetChangeCallback(int slot) const { return m_pParent->m_fnChangeCallbacks[slot].m_pCallback; }

	FORCEINLINE bool            GetBool(void) const;
	FORCEINLINE float           GetFloat(void) const;
	FORCEINLINE int             GetInt(void) const;
	FORCEINLINE Color           GetColor(void) const;
	FORCEINLINE const char*     GetString(void) const;

	// Any function that allocates/frees memory needs to be virtual or else you'll have crashes
	//  from alloc/free across dll/exe boundaries.
	
	// These just call into the IConCommandBaseAccessor to check flags and set the var (which ends up calling InternalSetValue).
	virtual void                SetValue(const char *value);
	virtual void                SetValue(float value);
	virtual void                SetValue(int value);
	void                        SetValue(Color value);

	// Reset to default value
	void                        Revert(void);

	// True if it has a min/max setting
	bool                        HasMin(void) const;
	bool                        HasMax(void) const;

	void                        SetMax(float flMaxValue);
	void                        SetMin(float flMinValue);

	bool                        GetMin(float& flMinValue) const;
	bool                        GetMax(float& flMaxValue) const;

	float                       GetMinValue(void) const;
	float                       GetMaxValue(void) const;

	const char*                 GetDefault(void) const;
	void                        SetDefault(const char* pszDefault);

private:
	bool                        InternalSetColorFromString(const char* value);

	// Called by CCvar when the value of a var is changing.
	virtual void                InternalSetValue(const char* pszValue);

	// For CVARs marked FCVAR_NEVER_AS_STRING
	virtual void                InternalSetFloatValue(float flValue);
	virtual void                InternalSetIntValue(int nValue);

	virtual void                InternalSetColorValue(Color value);

	// DoNothing in the engine, probably for tracking/debugging cvar strings in debug.
	virtual void                TrackValueChange(const char* value) { };

	virtual bool                ClampValue(float& flValue);

	virtual void                ChangeStringValue(const char* pszTempValue);

	virtual void                Create(const char* pName, const char* pDefaultValue, int flags = 0,
		const char* pHelpString = 0, bool bMin = false, float fMin = 0.0, bool bMax = false, float fMax = false,
		FnChangeCallback_t callback = 0, const char* pszUsageString = 0);

protected:
	struct CVValue_t
	{
		char*      m_pszString;
		size_t     m_iStringLength;
		float      m_fValue;
		int        m_nValue;
	};

	struct CVChange_t
	{
		bool operator ==(const CVChange_t& other) const
		{
			return (other.m_pCallback == m_pCallback && !m_pUserData);
		}

		FnChangeCallback_t m_pCallback;
		ChangeUserData_t   m_pUserData; // Typically used for syncing cvars with VGUI sliders.
	};

	ConVar*        m_pParent;         //0x0048
	const char*    m_pszDefaultValue; //0x0050
	CVValue_t      m_Value;           //0c0058
	bool           m_bHasMin;         //0x0070
	float          m_fMinVal;         //0x0074
	bool           m_bHasMax;         //0x0078
	float          m_fMaxVal;         //0x007C
	CUtlVector<CVChange_t> m_fnChangeCallbacks; //0x0080
};
static_assert(sizeof(ConVar) == 0xA0);

///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a boolean.
// Output : bool
//-----------------------------------------------------------------------------
FORCEINLINE bool ConVar::GetBool(void) const
{
	return !!GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float.
// Output : float
//-----------------------------------------------------------------------------
FORCEINLINE float ConVar::GetFloat(void) const
{
	return m_pParent->m_Value.m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an integer.
// Output : int
//-----------------------------------------------------------------------------
FORCEINLINE int ConVar::GetInt(void) const
{
	return m_pParent->m_Value.m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color.
// Output : Color
//-----------------------------------------------------------------------------
FORCEINLINE Color ConVar::GetColor(void) const
{
	unsigned char* pColorElement = (reinterpret_cast<unsigned char*>(&m_pParent->m_Value.m_nValue));
	return Color(pColorElement[0], pColorElement[1], pColorElement[2], pColorElement[3]);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string.
// Output : const char *
//-----------------------------------------------------------------------------
FORCEINLINE const char* ConVar::GetString(void) const
{
	if (m_nFlags & FCVAR_NEVER_AS_STRING)
	{
		return "FCVAR_NEVER_AS_STRING";
	}

	char const* str = m_pParent->m_Value.m_pszString;
	return str ? str : "";
}

//-----------------------------------------------------------------------------
// Called by the framework to register ConCommands with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register(int nCVarFlag = 0, IConCommandBaseAccessor* pAccessor = NULL);
void ConVar_Unregister();

bool ConVar_IsRegistered();

#endif // CONVAR_H
