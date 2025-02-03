#ifndef C_BASEENTITY_H
#define C_BASEENTITY_H

#include "game/shared/collisionproperty.h"
#include "game/shared/particleproperty.h"
#include "game/shared/predictioncopy.h"

#include "vscript/ivscript.h"
#include "icliententity.h"

#include "mathlib/vector.h"

// How many data slots to use when in multiplayer.
#define MULTIPLAYER_BACKUP 750

class C_BaseEntity : public IClientEntity, public IClientModelRenderable
{
public:
	const HSCRIPT GetScriptInstance();

protected:
	char pad0[16];
	void* unkHandle;
	char pad1[16];
	int m_iEFlags;
	char gap_5c[8];
	float m_currentFrame__animCycle_MAYBE;
	char gap_68[16];
	HSCRIPT m_hEntityScriptInstance;
	const char* m_iszScriptId;
	char gap_88[16];
	int m_fFlags;
	char gap_9c[12];
	__int16 m_modelIndex;
	char gap_aa[14];
	Vector3D m_viewOffset;
	char gap_c4[4];
	int m_currentFrame__weaponGettingSwitchedOut_MAYBE[2];
	bool m_currentFrame__showActiveWeapon3p_MAYBE[2];
	char gap_d2[70];
	int m_pMoveParent;
	char gap_11c[10];
	Vector3D m_vecAngVelocity;
	Vector3D m_angAbsRotation;
	Vector3D m_vecAbsVelocity;
	Vector3D m_vecAbsOrigin;
	Vector3D m_localOrigin;
	Vector3D m_localAngles;
	char gap_170[600];
	float m_flGravity;
	float m_flProxyRandomValue;
	Vector3D m_vecBaseVelocity;
	EHANDLE m_hGroundEntity;
	char gap_3e0[4];
	float m_flMaxspeed;
	int m_visibilityFlags;
	int m_fEffects;
	int m_iTeamNum;
	char gap_3f4[16];
	int m_passThroughFlags;
	int m_passThroughThickness;
	float m_passThroughDirection;
	Vector3D m_deathVelocity;
	Vector3D m_vecVelocity;
	Vector3D m_angNetworkAngles;
	float m_flFriction;
	char gap_438[4];
	EHANDLE m_hOwnerEntity;
	bool m_bRenderWithViewModels;
	unsigned __int8 m_nRenderFX;
	char gap_442[15];
	unsigned __int8 m_nRenderMode;
	unsigned __int8 m_MoveType;
	unsigned __int8 m_MoveCollide;
	char gap_454[4];
	CCollisionProperty m_Collision;
	char gap_4d8[238];
	int m_nNextThinkTick;
	char gap_5c4[396];
	int m_phaseShiftFlags;
	char gap_754[200];
	CParticleProperty m_Particle;
	char pad_bottom_1[368];
	int m_animNetworkFlags;
	bool m_networkAnimActive;
	char gap_a2d[1];
	bool m_animActive;
	bool m_animCollisionEnabled;
	bool m_animPlantingEnabled;
	char gap_9d1[47];
};

static_assert(sizeof(C_BaseEntity) == 0xA00);

inline HSCRIPT(*v_C_BaseEntity__GetScriptInstance)(C_BaseEntity* thisp);

///////////////////////////////////////////////////////////////////////////////
class VC_BaseEntity : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("C_BaseEntity::GetScriptInstance", v_C_BaseEntity__GetScriptInstance);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 56 48 83 EC ?? 80 B9 ?? ?? ?? ?? ?? 48 8B F1 75").GetPtr(v_C_BaseEntity__GetScriptInstance);
	}
	virtual void GetVar(void) const {}
	virtual void GetCon(void) const {}
	virtual void Detour(const bool bAttach) const {};
};
///////////////////////////////////////////////////////////////////////////////

#endif // C_BASEENTITY_H
