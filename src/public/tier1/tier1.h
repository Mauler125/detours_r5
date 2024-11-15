//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//
#ifndef TIER1_H
#define TIER1_H
#include "appframework/IAppSystem.h"

//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem for tier2 libraries
//-----------------------------------------------------------------------------
template< class IInterface, int ConVarFlag = 0 > 
class CTier1AppSystem : public CTier0AppSystem< IInterface >
{
	typedef CTier0AppSystem< IInterface > BaseClass;

public:
	virtual bool Connect( const CreateInterfaceFn factory ) { return true; };
	virtual void Disconnect() {};
	virtual void* QueryInterface(const char* const pInterfaceName) { return NULL; };
	virtual InitReturnVal_t Init() { return INIT_OK; };
	virtual void Shutdown() {};
	virtual const AppSystemInfo_t* GetDependencies() { return NULL; }
};

#endif // TIER1_H
