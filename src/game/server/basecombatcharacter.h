//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Base combat character with no AI
//
// $NoKeywords: $
//===========================================================================//

#ifndef BASECOMBATCHARACTER_H
#define BASECOMBATCHARACTER_H
#include "baseanimatingoverlay.h"
#include "game/server/ai_hull.h"

struct WeaponDropInfo
{
	Vector3D weaponPosition;
	char prevDropFrameCounter;
	char dropFrameCounter;
	char gap_e[2];
	Vector3D weaponAngles;
	float weaponPositionTime;
};

/* 1410 */
struct WeaponInventory
{
	void* __vftable;
	EHANDLE weapons[9];
	EHANDLE offhandWeapons[6];
	EHANDLE activeWeapons[3];
};

struct CTether
{
	void* __vftable;
	Vector3D pos;
	float health;
	float nextSoundTime;
	float creationTime;
	int scriptID;
};

//-----------------------------------------------------------------------------
// Purpose: This should contain all of the combat entry points / functionality 
// that are common between NPCs and players
//-----------------------------------------------------------------------------
class CBaseCombatCharacter : public CBaseAnimatingOverlay
{
public:
	// Nav hull type
	Hull_e  GetHullType() const          { return m_eHull; }
	void    SetHullType(Hull_e hullType) { m_eHull = hullType; }

private:
	bool m_bPreventWeaponPickup;
	char gap_15b1[3];
	float m_phaseShiftTimeStart;
	float m_phaseShiftTimeEnd;
	float m_flNextAttack;
	float m_lastFiredTime;
	EHANDLE m_lastFiredWeapon;
	float m_raiseFromMeleeEndTime;
	float m_nextFlamethrowerStatusEffectUpdateTime;
	EHANDLE m_lastFlamethrowerStatusEffectInflictor;
	EHANDLE m_lastFlamethrowerStatusEffectAttacker;
	int m_sharedEnergyCount;
	int m_sharedEnergyTotal;
	int m_sharedEnergyLockoutThreshold;
	float m_lastSharedEnergyRegenTime;
	float m_sharedEnergyRegenRate;
	float m_sharedEnergyRegenDelay;
	float m_lastSharedEnergyTakeTime;
	Hull_e m_eHull;
	float m_fieldOfViewCos;
	Vector3D m_HackedGunPos;
	float m_impactEnergyScale;
	WeaponDropInfo m_weaponDropInfo;
	float m_physicsMass;
	float m_flDamageAccumulator;
	int m_prevHealth;
	float m_healthChangeRate;
	float m_healthChangeAmount;
	float m_healthChangeStartTime;
	float m_lastHealthChangeTime;
	int m_lastHitGroup;
	Vector3D m_lastDamageDir;
	Vector3D m_lastDamageForce;
	int m_deathPackage;
	Vector3D m_deathDirection2DInverse;
	int m_CurrentWeaponProficiency;
	float m_flEnemyAccurcyMultiplier;
	int m_npcPriorityOverride;
	int m_healthPerSegment;
	char gap_1684[4];
	WeaponInventory m_inventory;
	char m_selectedWeapons[2];
	char gap_16da[2];
	EHANDLE m_latestPrimaryWeapons[2];
	EHANDLE m_latestPrimaryWeaponsIndexZeroOrOne[2];
	char m_latestNonOffhandWeapons[2];
	char m_selectedOffhands[3];
	char m_selectedOffhandsPendingHybridAction[3];
	char m_lastCycleSlot;
	char gap_16f5[3];
	EHANDLE m_weaponGettingSwitchedOut[2];
	bool m_showActiveWeapon3p[2];
	char gap_1702[2];
	int m_weaponPermission;
	float m_weaponDelayEnableTime;
	char m_weaponDisabledFlags;
	bool m_hudInfo_visibilityTestAlwaysPasses;
	bool m_weaponDisabledInScript;
	char gap_170f[1];
	EHANDLE m_removeWeaponOnSelectSwitch;
	EHANDLE m_latestMeleeWeapon;
	bool m_doOffhandAnim;
	bool m_wantInventoryChangedScriptCall;
	bool m_doInventoryChangedScriptCall;
	char gap_171b[1];
	float m_cloakReactEndTime;
	CTether m_tethers[2];
	EHANDLE m_titanSoul;
	Vector3D m_lastFootstepDamagePos;
	bool m_lastFootstepDamageOnGround;
	char gap_1781[3];
	int m_muzzleAttachment[2];
	int m_weaponHandAttachment;
	int m_weaponAltHandAttachment;
	int m_prevNearestNode;
	int m_nearestNode;
	float m_nearestNodeCheckTime;
	Vector3D m_nearestNodeCheckPos;
	int m_nearestPolyRef[5];
	Vector3D m_nearestPolyCheckPos[5];
	int m_contextAction;
	char m_weaponAnimEvents[16648];
	char m_targetInfoIconName[64];
	bool m_titanStepDamage;
	char gap_5949[3];
	EHANDLE m_latest3pWeaponGettingEquipped[2];
	char gap_5954[12];
};

static_assert(sizeof(CBaseCombatCharacter) == 0x5960);

#endif // BASECOMBATCHARACTER_H
