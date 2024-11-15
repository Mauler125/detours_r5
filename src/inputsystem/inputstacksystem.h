//===== Copyright © 1996-2010, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#ifndef INPUTCLIENTSTACK_H
#define INPUTCLIENTSTACK_H
#ifdef _WIN32
#pragma once
#endif
#include "tier1/utlstack.h"
#include "inputsystem/iinputstacksystem.h"

//-----------------------------------------------------------------------------
// An input context
//-----------------------------------------------------------------------------
struct InputContext_t
{
	InputCursorHandle_t m_hCursorIcon;
	bool m_bEnabled;
	bool m_bCursorVisible;
	bool m_bMouseCaptureEnabled;
};

//-----------------------------------------------------------------------------
// Stack system implementation
//-----------------------------------------------------------------------------
class CInputStackSystem : public CTier1AppSystem< IInputStackSystem >
{
	typedef CTier1AppSystem< IInputStackSystem > BaseClass;

	// Methods of IAppSystem
	// NOTE: currently, the implementation in the game engine is used. If the
	// vtable ever gets swapped with the implementation in the SDK, make sure
	// to implement BaseClass::Shutdown() and uncomment the functions below !!!
	// The implementation in this SDK is identical to that of the engine.
public:
	//virtual const AppSystemInfo_t* GetDependencies();
	//virtual void Shutdown();

	// Methods of IInputStackSystem
public:
	virtual InputContextHandle_t PushInputContext();
	virtual void PopInputContext( InputContextHandle_t& hContext );
	virtual void EnableInputContext( InputContextHandle_t hContext, bool bEnable );
	virtual void SetCursorVisible( InputContextHandle_t hContext, bool bVisible );
	virtual void SetCursorIcon( InputContextHandle_t hContext, InputCursorHandle_t hCursor );
	virtual void SetMouseCapture( InputContextHandle_t hContext, bool bEnable );
	virtual void SetCursorPosition( InputContextHandle_t hContext, int x, int y );
	virtual bool IsTopmostEnabledContext( InputContextHandle_t hContext ) const;

private:
	// Updates the cursor based on the current state of the input stack
	void UpdateCursorState();

	CUtlStack< InputContext_t* > m_ContextStack;
};

extern CInputStackSystem* g_pInputStackSystem;

///////////////////////////////////////////////////////////////////////////////
class VInputStackSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_InputStackSystem", g_pInputStackSystem);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pInputStackSystem = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 E8 ?? ?? ?? ?? 33 C9").OffsetSelf(0x120)
			.FindPatternSelf("48 8D", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CInputStackSystem*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};
///////////////////////////////////////////////////////////////////////////////


#endif // INPUTCLIENTSTACK_H
