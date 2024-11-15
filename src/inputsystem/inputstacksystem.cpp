//===== Copyright © 1996-2010, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#include "inputsystem.h"
#include "inputstacksystem.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Allocates an input context, pushing it on top of the input stack, 
// thereby giving it top priority
//-----------------------------------------------------------------------------
InputContextHandle_t CInputStackSystem::PushInputContext()
{
	InputContext_t *pContext = new InputContext_t;
	pContext->m_bEnabled = true;
	pContext->m_bCursorVisible = true;
	pContext->m_bMouseCaptureEnabled = false;
	pContext->m_hCursorIcon = g_pInputSystem->GetStandardCursor( INPUT_CURSOR_ARROW );
	m_ContextStack.Push( pContext );

	UpdateCursorState();

	return (InputContextHandle_t)pContext;
}


//-----------------------------------------------------------------------------
// Pops the provided input context off the input stack, and destroys it.
//-----------------------------------------------------------------------------
void CInputStackSystem::PopInputContext( InputContextHandle_t& hContext )
{
	const int nCount = m_ContextStack.Count();

	if ( nCount == 0 )
		return;

	int i = 0;
	InputContext_t *pContext = *m_ContextStack.Base();

	// Find the context.
	for ( ; pContext != (InputContext_t*)hContext; pContext++ )
	{
		if ( ++i == nCount )
		{
			Assert( 0 );
			return;
		}
	}

	m_ContextStack.PopAt( i );
	hContext = INPUT_CONTEXT_HANDLE_INVALID;

	UpdateCursorState();
}


//-----------------------------------------------------------------------------
// Enables/disables an input context, allowing something lower on the
// stack to have control of input. Disabling an input context which
// owns mouse capture
//-----------------------------------------------------------------------------
void CInputStackSystem::EnableInputContext( InputContextHandle_t hContext, bool bEnable )
{
	InputContext_t *pContext = ( InputContext_t* )hContext;
	if ( !pContext )
		return;

	if ( pContext->m_bEnabled == bEnable )
		return;

	// Disabling an input context will deactivate mouse capture, if it's active
	if ( !bEnable )
	{
		SetMouseCapture( hContext, false );
	}

	pContext->m_bEnabled = bEnable;

	// Updates the cursor state since the stack changed
	UpdateCursorState();
}


//-----------------------------------------------------------------------------
// Allows a context to make the cursor visible;
// the topmost enabled context wins
//-----------------------------------------------------------------------------
void CInputStackSystem::SetCursorVisible( InputContextHandle_t hContext, bool bVisible )
{
	InputContext_t *pContext = ( InputContext_t* )hContext;
	if ( !pContext )
		return;

	if ( pContext->m_bCursorVisible == bVisible )
		return;

	pContext->m_bCursorVisible = bVisible;

	// Updates the cursor state since the stack changed
	UpdateCursorState();
}


//-----------------------------------------------------------------------------
// Allows a context to set the cursor icon;
// the topmost enabled context wins
//-----------------------------------------------------------------------------
void CInputStackSystem::SetCursorIcon( InputContextHandle_t hContext, InputCursorHandle_t hCursor )
{
	InputContext_t *pContext = ( InputContext_t* )hContext;
	if ( !pContext )
		return;

	if ( pContext->m_hCursorIcon == hCursor )
		return;

	pContext->m_hCursorIcon = hCursor;

	// Updates the cursor state since the stack changed
	UpdateCursorState();
}


//-----------------------------------------------------------------------------
// Allows a context to enable mouse capture. Disabling an input context
// deactivates mouse capture. Capture will occur if it happens on the
// topmost enabled context
//-----------------------------------------------------------------------------
void CInputStackSystem::SetMouseCapture( InputContextHandle_t hContext, bool bEnable )
{
	InputContext_t *pContext = ( InputContext_t* )hContext;
	if ( !pContext )
		return;

	if ( pContext->m_bMouseCaptureEnabled == bEnable )
		return;

	pContext->m_bMouseCaptureEnabled = bEnable;

	// Updates the cursor state since the stack changed
	UpdateCursorState();
}


//-----------------------------------------------------------------------------
// Allows a context to set the mouse position. It only has any effect if the
// specified context is the topmost enabled context
//-----------------------------------------------------------------------------
void CInputStackSystem::SetCursorPosition( InputContextHandle_t hContext, int x, int y )
{
	if ( IsTopmostEnabledContext( hContext ) )
	{
		g_pInputSystem->SetCursorPosition( x, y );
	}
}


//-----------------------------------------------------------------------------
// This this context the topmost enabled context?
//-----------------------------------------------------------------------------
bool CInputStackSystem::IsTopmostEnabledContext( InputContextHandle_t hContext ) const
{
	InputContext_t *pContext = ( InputContext_t* )hContext;
	if ( !pContext )
		return false;

	int nCount = m_ContextStack.Count();
	for ( int i = nCount; --i >= 0; )
	{
		InputContext_t *pStackContext = m_ContextStack[i];
		if ( !pStackContext->m_bEnabled )
			continue;

		return ( pStackContext == pContext );
	}
	return false;
}


//-----------------------------------------------------------------------------
// Updates the cursor based on the current state of the input stack
//-----------------------------------------------------------------------------
void CInputStackSystem::UpdateCursorState()
{
	int nCount = m_ContextStack.Count();
	for ( int i = nCount; --i >= 0; )
	{
		InputContext_t *pContext = m_ContextStack[i];
		if ( !pContext->m_bEnabled )
			continue;

		if ( !pContext->m_bCursorVisible )
		{
			g_pInputSystem->SetCursorIcon( INPUT_CURSOR_HANDLE_INVALID );
		}
		else
		{
			g_pInputSystem->SetCursorIcon( pContext->m_hCursorIcon );
		}

		if ( pContext->m_bMouseCaptureEnabled )
		{
			g_pInputSystem->EnableMouseCapture( g_pInputSystem->GetAttachedWindow() );
		}
		else
		{
			g_pInputSystem->DisableMouseCapture( );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Get dependencies
//-----------------------------------------------------------------------------
//static AppSystemInfo_t s_Dependencies[] =
//{
//	{ "inputsystem" DLL_EXT_STRING, INPUTSYSTEM_INTERFACE_VERSION },
//	{ NULL, NULL }
//};
//
//const AppSystemInfo_t* CInputStackSystem::GetDependencies()
//{
//	return s_Dependencies;
//}


//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------
//void CInputStackSystem::Shutdown()
//{
//	// Delete any leaked contexts
//	while( m_ContextStack.Count() )
//	{
//		InputContext_t *pContext = NULL;
//		m_ContextStack.Pop( pContext );
//		delete pContext;
//	}
//
//	BaseClass::Shutdown();
//}

//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------
CInputStackSystem* g_pInputStackSystem = nullptr;
