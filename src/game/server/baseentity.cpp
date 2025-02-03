//======= Copyright (c) 1996-2009, Valve Corporation, All rights reserved. ======
//
// Purpose: The base class from which all game entities are derived.
//
//===============================================================================
#include "core/stdafx.h"
#include "baseentity.h"
#include "engine/gl_model_private.h"
#include "engine/modelinfo.h"

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CCollisionProperty* CBaseEntity::CollisionProp()
{
	return &m_Collision;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const CCollisionProperty* CBaseEntity::CollisionProp() const
{
	return &m_Collision;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CServerNetworkProperty* CBaseEntity::NetworkProp()
{
	return &m_Network;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const CServerNetworkProperty* CBaseEntity::NetworkProp() const
{
	return &m_Network;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
model_t* CBaseEntity::GetModel(void)
{
	return (model_t*)g_pModelInfoServer->GetModel(GetModelIndex());
}

inline string_t CBaseEntity::GetModelName(void) const
{
	return m_ModelName;
}

inline int CBaseEntity::GetModelIndex(void) const
{
	return m_nModelIndex;
}

const HSCRIPT CBaseEntity::GetScriptInstance()
{
	return v_CBaseEntity__GetScriptInstance(this);
}