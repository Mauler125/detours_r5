//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef ENTS_SHARED_H
#define ENTS_SHARED_H

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CClientFrame;
class bf_read;

// Used to classify entity update types in DeltaPacketEntities.
enum EntityUpdateType_e
{
	EnterPVS = 0,	// Entity came back into pvs, create new entity if one doesn't exist

	LeavePVS,		// Entity left pvs

	DeltaEnt,		// There is a delta for this entity.
	PreserveEnt,	// Entity stays alive but no delta ( could be LOD, or just unchanged )

	Finished,		// finished parsing entities successfully
	Failed,			// parsing error occured while reading entities
};

// Base entity info class.
struct CEntityInfo
{
	virtual	~CEntityInfo() {};

	bool m_bAsDelta;

	CClientFrame* m_pFrom;
	CClientFrame* m_pTo;

	EntityUpdateType_e m_UpdateType;

	int m_nOldEntity; // current entity index in m_pFrom
	int m_nNewEntity; // current entity index in m_pTo

	int m_nHeaderBase;
	int m_nHeaderCount;
};

// Flags for delta encoding header
enum EntityUpdateFlags_e
{
	FHDR_ZERO = 0x0,
	FHDR_LEAVEPVS = 0x1,
	FHDR_DELETE = 0x2,
	FHDR_ENTERPVS = 0x4,
};

// Passed around the read functions.
class CEntityReadInfo : public CEntityInfo
{
	bf_read* m_pBuf;
	EntityUpdateFlags_e m_UpdateFlags; // from the subheader
	bool m_bIsEntity;
};

#endif // ENTS_SHARED_H
