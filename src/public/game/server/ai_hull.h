//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef AI_HULL_H
#define AI_HULL_H

#include "ai_navmesh.h"
#include "bspflags.h"
#include "mathlib/vector.h"

//=========================================================
// Link Properties. These hulls must correspond to the hulls
// in g_hullProperties!
//=========================================================
enum Hull_e
{
	HULL_HUMAN,
	HULL_MEDIUM,
	HULL_FLYING_VEHICLE,
	HULL_SMALL,
	HULL_TITAN,
	HULL_GOLIATH,
	HULL_PROWLER,
	//--------------------------------------------
	NUM_HULLS,					// Confirmed, see [r5apex_ds + CB2765]
	HULL_NONE = -1				// No Hull (appears after num hulls as we don't want to count it)
};

struct Hull_s
{
	Hull_s(const char* pName, int bit, const Vector3D& _mins, const Vector3D& _maxs,
		  const float _stepHeight, const float _scale, const float _unk10, const float _unk11,
		  const unsigned int _traceMask, NavMeshType_e _navMeshType)

		: hullName(pName), hullBit(bit)
		, mins(_mins), maxs(_maxs)
		, stepHeight(_stepHeight), extentScale(_scale)
		, unk10(_unk10), unk11(_unk11)
		, traceMask(_traceMask), navMeshType(_navMeshType) {}

	const char* hullName;
	int hullBit;

	Vector3D mins;
	Vector3D maxs;

	// The maximum height an object can have
	// before the hull could no longer step
	// over it.
	float stepHeight;

	// The scale to apply to the hull size
	// when using it as extents to query a
	// point on the navmesh.
	float extentScale;

	float unk10;
	float unk11;

	int traceMask;
	NavMeshType_e navMeshType;
};

enum HullBits_e
{
	bits_HUMAN_HULL          = (1 << 0),
	bits_MEDIUM_HULL         = (1 << 1),
	bits_FLYING_VEHICLE_HULL = (1 << 2),
	bits_SMALL_HULL          = (1 << 3),
	bits_TITAN_HULL          = (1 << 4),
	bits_GOLIATH_HULL        = (1 << 5),
	bits_PROWLER_HULL        = (1 << 6),
};

static inline const char* g_aiHullNames[NUM_HULLS] = {
	"HULL_HUMAN",
	"HULL_MEDIUM",
	"HULL_FLYING_VEHICLE",
	"HULL_SMALL",
	"HULL_TITAN",
	"HULL_GOLIATH",
	"HULL_PROWLER",
};

static inline const Hull_s g_aiHullProperties[NUM_HULLS] = {
	{g_aiHullNames[HULL_HUMAN]         , bits_HUMAN_HULL         , Vector3D(-16.0f, -16.0f, 0.0f)  , Vector3D(16.0f, 16.0f, 72.0f)   , 18.0f, 0.5f       , 60.0f , 250.0f, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_NOAIRDROP|CONTENTS_OPERATORCLIP|CONTENTS_BULLETCLIP|CONTENTS_BLOCKLOS, NAVMESH_SMALL},
	{g_aiHullNames[HULL_MEDIUM]        , bits_MEDIUM_HULL        , Vector3D(-48.0f, -48.0f, 0.0f)  , Vector3D(48.0f, 48.0f, 150.0f)  , 32.0f, 0.5f       , 256.0f, 256.0f, CONTENTS_HITBOX|CONTENTS_DEBRIS                                                                                , NAVMESH_MEDIUM},
	{g_aiHullNames[HULL_FLYING_VEHICLE], bits_FLYING_VEHICLE_HULL, Vector3D(-150.0f, -150.0f, 0.0f), Vector3D(150.0f, 150.0f, 300.0f), 80.0f, 0.5f       , 0.0f  , 0.0f  , CONTENTS_EMPTY                                                                                                 , NAVMESH_LARGE},
	{g_aiHullNames[HULL_SMALL]         , bits_SMALL_HULL         , Vector3D(-16.0f, -16.0f, 0.0f)  , Vector3D(16.0f, 16.0f, 32.0f)   , 18.0f, 0.5f       , 0.0f  , 0.0f  , CONTENTS_EMPTY                                                                                                 , NAVMESH_SMALL},
	{g_aiHullNames[HULL_TITAN]         , bits_TITAN_HULL         , Vector3D(-60.0f, -60.0f, 0.0f)  , Vector3D(60.0f, 60.0f, 235.0f)  , 80.0f, 0.89999998f, 60.0f , 512.0f, CONTENTS_HITBOX|CONTENTS_DEBRIS|CONTENTS_OPERATORCLIP                                                          , NAVMESH_LARGE},
	{g_aiHullNames[HULL_GOLIATH]       , bits_GOLIATH_HULL       , Vector3D(-80.0f, -80.0f, 0.0f)  , Vector3D(80.0f, 80.0f, 235.0f)  , 80.0f, 0.69999999f, 100.0f, 768.0f, CONTENTS_HITBOX|CONTENTS_DEBRIS|CONTENTS_OPERATORCLIP                                                          , NAVMESH_EXTRA_LARGE},
	{g_aiHullNames[HULL_PROWLER]       , bits_PROWLER_HULL       , Vector3D(-40.0f, -40.0f, 0.0f)  , Vector3D(40.0f, 40.0f, 72.0f)   , 18.0f, 0.5f       , 60.0f , 250.0f, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_NOAIRDROP|CONTENTS_OPERATORCLIP|CONTENTS_BULLETCLIP|CONTENTS_BLOCKLOS, NAVMESH_MED_SHORT},
};

//=============================================================================
//	>> CAI_Hull
//=============================================================================
namespace NAI_Hull
{
	inline const Vector3D& Mins(const Hull_e id) { return g_aiHullProperties[id].mins; }
	inline const Vector3D& Maxs(const Hull_e id) { return g_aiHullProperties[id].maxs; }

	inline float Length(const Hull_e id) { return (g_aiHullProperties[id].maxs.x - g_aiHullProperties[id].mins.x); }
	inline float Width(const Hull_e id)  { return (g_aiHullProperties[id].maxs.y - g_aiHullProperties[id].mins.y); }
	inline float Height(const Hull_e id) { return (g_aiHullProperties[id].maxs.z - g_aiHullProperties[id].mins.z); }
	inline float Radius(const Hull_e id) { return Width(id) * 0.5f; }

	inline float StepHeight(const Hull_e id) { return g_aiHullProperties[id].stepHeight; }
	inline float ExtentScale(const Hull_e id) { return g_aiHullProperties[id].extentScale; }

	inline int Bits(const Hull_e id) { return g_aiHullProperties[id].hullBit; }
	inline const char* Name(const Hull_e id) { return g_aiHullProperties[id].hullName; }

	inline unsigned int TraceMask(const Hull_e id) { return g_aiHullProperties[id].traceMask; };

	inline NavMeshType_e NavMeshType(const Hull_e id) { return g_aiHullProperties[id].navMeshType; }
	inline Hull_e LookupId(const char* const szName)
	{
		if (!szName)
		{
			// Return the first if null.
			return HULL_HUMAN;
		}
		for (int i = 0; i < NUM_HULLS; i++)
		{
			if (stricmp(szName, NAI_Hull::Name((Hull_e)i)) == 0)
			{
				return (Hull_e)i;
			}
		}

		return HULL_HUMAN;
	}
};

#endif // AI_HULL_H
