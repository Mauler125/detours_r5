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
public:
	virtual void Shutdown();
	virtual const AppSystemInfo_t* GetDependencies();

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

// NOTE: we use the engine's implementation of CInputStackSystem, even though
// we have the entire class implemented in the SDK. If, for whatever reason,
// the SDK's implementation is used, make sure all methods are tested properly
// first before fully migrating to the SDK's implementation. The only method
// that appeared to have changed compared to other source game interfaces is
// CInputStackSystem::PopInputContext(), which now actually takes the context
// handle to pop it rather than pushing/popping handles in an explicit order.
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
