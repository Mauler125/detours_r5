#ifndef C_PLAYER_H
#define C_PLAYER_H

#include "icliententity.h"
#include "icliententitylist.h"
#include "iclientnetworkable.h"
#include "iclientrenderable.h"
#include "iclientthinkable.h"
#include "iclientunknown.h"
#include "ihandleentity.h"
#include "playerstate.h"
#include "vscript/ivscript.h"

#include "input.h"
#include "gamepad.h"

#include "c_baseentity.h"
#include "c_baseanimating.h"
#include "c_baseanimatingoverlay.h"
#include "c_basecombatcharacter.h"
#include "c_playerlocaldata.h"
#include "r1/c_weapon_x.h"

#include "game/shared/r1/grapple.h"
#include "game/shared/status_effect.h"
#include "game/shared/player_viewoffset.h"
#include "game/shared/player_melee.h"

class CInput;
struct CInput::UserInput_t;


struct PlayerZiplineData_Client
{
	void* _vftable;
	bool m_ziplineReenableWeapons;
	char gap_9[3];
	float m_mountingZiplineDuration;
	float m_mountingZiplineAlpha;
	float m_ziplineStartTime;
	float m_ziplineEndTime;
	Vector3D m_mountingZiplineSourcePosition;
	Vector3D m_mountingZiplineSourceVelocity;
	Vector3D m_mountingZiplineTargetPosition;
	char gap_40[12];
	Vector3D m_ziplineUsePosition;
	float m_slidingZiplineAlpha;
	Vector3D m_lastMoveDir2D;
	bool m_ziplineReverse;
};

class C_KnockBack
{
	void* _vftable;
	Vector3D velocity;
	float beginTime;
	float endTime;
};

struct JoyAngle_t
{
	// The joystick rotation amount.
	QAngle rotation;

	// The joystick counter amount (used for auto-aim).
	QAngle counter;
};

class C_Player : public C_BaseCombatCharacter
{
public:
	static void CurveLook(C_Player* player, CInput::UserInput_t* input, float a3, float a4, float a5, int a6, float inputSampleFrametime, bool runAimAssist, JoyAngle_t* a9);
	bool CheckMeleeWeapon();

	inline bool IsZooming() const { return m_bZooming; }
private:
	bool unk;
	bool m_bZooming;
	char gap_1882[2];
	float m_zoomToggleOnStartTime;
	float m_zoomBaseFrac;
	float m_zoomBaseTime;
	float m_zoomFullStartTime;
	char gap_1894[124];
	int m_lastUCmdSimulationTicks;
	float m_lastUCmdSimulationRemainderTime;
	char gap_1918[280];
	C_PlayerLocalData m_Local;
	char gap_1a30[32];
	float m_currentFramePlayer__timeBase;
	char gap_1d1c[4];
	StatusEffectTimedData m_currentFramePlayer__statusEffectsTimedPlayerCUR[10];
	StatusEffectEndlessData m_currentFramePlayer__statusEffectsEndlessPlayerCUR[10];
	float m_currentFramePlayer__m_flHullHeight;
	float m_currentFramePlayer__m_traversalAnimProgress;
	float m_currentFramePlayer__m_sprintTiltFrac;
	char gap_1ebc[12];
	int m_currentFramePlayer__m_ammoPoolCount[8];
	char gap_1ee8[432];
	Vector3D m_currentFrameLocalPlayer__m_stepSmoothingOffset;
	Vector3D m_currentFrameLocalPlayer__m_vecPunchBase_Angle;
	Vector3D m_currentFrameLocalPlayer__m_vecPunchBase_AngleVel;
	Vector3D m_currentFrameLocalPlayer__m_vecPunchWeapon_Angle;
	Vector3D m_currentFrameLocalPlayer__m_vecPunchWeapon_AngleVel;
	char gap_20d4[48];
	Quaternion m_currentFrameLocalPlayer__m_localGravityRotation;
	char gap_2114[4];
	CPlayerState pl;
	char gap_2118[132];
	int m_ammoPoolCapacity;
	char gap_21a0[714];
	int m_gestureSequences[8];
	float m_gestureStartTimes[8];
	float m_gestureBlendInDuration[8];
	float m_gestureBlendOutDuration[8];
	float m_gestureFadeOutStartTime[8];
	float m_gestureFadeOutDuration[8];
	int m_gestureAutoKillBitfield;
	char gap_25c0[24];
	int m_afButtonLast;
	int m_afButtonPressed;
	int m_afButtonReleased;
	int m_nButtons;
	int m_nImpulse;
	int m_flPhysics;
	float m_flStepSoundTime;
	float m_flTimeAllSuitDevicesOff;
	float m_fStickySprintMinTime;
	bool m_bPlayedSprintStartEffects;
	char gap_25fd[7];
	bool m_fIsSprinting;
	bool m_fIsWalking;
	char gap_2606[2];
	float m_sprintStartedTime;
	float m_sprintStartedFrac;
	float m_sprintEndedTime;
	float m_sprintEndedFrac;
	float m_stickySprintStartTime;
	float m_damageImpulseNoDecelEndTime;
	char gap_2620[12];
	int m_duckState;
	int m_leanState;
	bool m_doingHalfDuck;
	bool m_canStand;
	char gap_2636[2];
	Vector3D m_StandHullMin;
	Vector3D m_StandHullMax;
	Vector3D m_DuckHullMin;
	Vector3D m_DuckHullMax;
	char gap_2668[4];
	Vector3D m_upDir;
	Vector3D m_upDirPredicted;
	Vector3D m_lastWallRunStartPos;
	int m_wallRunCount;
	bool m_wallRunWeak;
	bool m_shouldBeOneHanded;
	char gap_2696[2];
	float m_oneHandFraction;
	float m_animAimPitch;
	float m_animAimYaw;
	float m_wallRunPushAwayTime;
	char gap_26a8[8];
	float m_wallrunRetryTime;
	Vector3D m_wallrunRetryPos;
	Vector3D m_wallrunRetryNormal;
	char gap_26cc[24];
	float m_wallHangTime;
	int m_traversalState;
	int m_traversalType;
	Vector3D m_traversalBegin;
	Vector3D m_traversalMid;
	Vector3D m_traversalEnd;
	float m_traversalMidFrac;
	Vector3D m_traversalForwardDir;
	Vector3D m_traversalRefPos;
	float m_traversalProgress;
	float m_traversalStartTime;
	float m_traversalHandAppearTime;
	float m_traversalReleaseTime;
	float m_traversalBlendOutStartTime;
	Vector3D m_traversalBlendOutStartOffset;
	float m_traversalYawDelta;
	char gap_2754[8];
	float m_wallDangleJumpOffTime;
	bool m_wallDangleMayHangHere;
	bool m_wallDangleForceFallOff;
	bool m_wallDangleLastPushedForward;
	char gap_2763[1];
	int m_wallDangleDisableWeapon;
	float m_wallDangleClimbProgressFloor;
	bool m_wallClimbSetUp;
	bool m_wallHanging;
	char gap_276e[2];
	GrappleData m_grapple;
	char gap_2770[16];
	bool m_grappleActive;
	bool m_grappleNeedWindowCheck;
	char gap_2802[2];
	int m_grappleNextWindowHint;
	char gap_2808[12];
	bool m_slowMoEnabled;
	bool m_sliding;
	bool m_slideLongJumpAllowed;
	char gap_2817[1];
	float m_lastSlideTime;
	float m_lastSlideBoost;
	int m_gravityGrenadeStatusEffect;
	bool m_bIsStickySprinting;
	char gap_2825[3];
	float m_prevMoveYaw;
	float m_sprintTiltVel;
	char gap_2830[24];
	int m_hViewModels[3];
	char gap_2854[4];
	Player_ViewOffsetEntityData m_viewOffsetEntity;
	char gap_2858[294];
	int m_activeZipline;
	int m_lastZipline;
	float m_lastZiplineDetachTime;
	bool m_ziplineValid3pWeaponLayerAnim;
	char gap_29a5[3];
	int m_ziplineState;
	char gap_29ac[4];
	PlayerZiplineData_Client m_zipline;
	Vector3D m_ziplineViewOffsetPosition;
	Vector3D m_ziplineViewOffsetVelocity;
	int m_ziplineGrenadeEntity;
	int m_ziplineGrenadeBeginStationEntity;
	int m_ziplineGrenadeBeginStationAttachmentIndex;
	char gap_2a44[8];
	int m_playAnimationType;
	bool m_detachGrappleOnPlayAnimationEnd;
	char gap_2a51[3];
	int m_playAnimationNext[2];
	char gap_2a5c[12];
	bool m_boosting;
	bool m_activateBoost;
	bool m_repeatedBoost;
	char gap_2a6b[1];
	float m_boostMeter;
	bool m_jetpack;
	bool m_activateJetpack;
	bool m_jetpackAfterburner;
	bool m_gliding;
	float m_glideMeter;
	float m_glideRechargeDelayAccumulator;
	bool m_hovering;
	bool m_isPerformingBoostAction;
	char gap_2a7e[2];
	float m_lastJumpHeight;
	char gap_2a84[76];
	Vector3D m_slipAirRestrictDirection;
	float m_slipAirRestrictTime;
	char gap_2ae0[400];
	PlayerMelee_PlayerData m_melee;
	bool m_useCredit;
	char gap_2ca9[979];
	float m_wallRunStartTime;
	float m_wallRunClearTime;
	float m_onSlopeTime;
	Vector3D m_lastWallNormal;
	bool m_dodging;
	char gap_3095[3];
	float m_lastDodgeTime;
	Vector3D m_vecPreviouslyPredictedOrigin;
	char gap_30a8[12];
	float m_flTimeLastTouchedWall;
	float m_timeJetpackHeightActivateCheckPassed;
	float m_flTimeLastTouchedGround;
	float m_flTimeLastJumped;
	float m_flTimeLastLanded;
	float m_flLastLandFromHeight;
	float m_usePressedTime;
	float m_lastUseTime;
	char gap_30d4[12];
	Vector3D m_lastFakeFloorPos;
	bool m_bHasJumpedSinceTouchedGround;
	bool m_bDoMultiJumpPenalty;
	bool m_dodgingInAir;
	char gap_30ef[185];
	bool m_thirdPerson;
	char gap_31A9[263];
	bool m_activeViewmodelModifiers[35];
	char gap_32d3[701];
	float m_lastMoveInputTime;
	int m_ignoreEntityForMovementUntilNotTouching;
	char gap_3598[1224];
	float m_gameMovementUtil__m_surfaceFriction;
	char gap_3a64[120];
	int m_lungeTargetEntity;
	bool m_isLungingToPosition;
	char gap_3ae1[3];
	Vector3D m_lungeTargetPosition;
	Vector3D m_lungeStartPositionOffset;
	Vector3D m_lungeEndPositionOffset;
	float m_lungeStartTime;
	float m_lungeEndTime;
	bool m_lungeCanFly;
	bool m_lungeLockPitch;
	char gap_3b12[2];
	float m_lungeStartPitch;
	float m_lungeSmoothTime;
	float m_lungeMaxTime;
	float m_lungeMaxEndSpeed;
	char gap_3b24[828];
	Vector3D m_vPrevGroundNormal;
	char gap_3e6c[440];
	Vector3D m_pushAwayFromTopAcceleration;
	char gap_4030[28];
	bool m_controllerModeActive;
	char gap_404d[23];
	float m_skydiveForwardPoseValueVelocity;
	float m_skydiveForwardPoseValueTarget;
	float m_skydiveForwardPoseValueCurrent;
	float m_skydiveSidePoseValueVelocity;
	float m_skydiveSidePoseValueTarget;
	float m_skydiveSidePoseValueCurrent;
	float m_skydiveYawVelocity;
	char gap_4080[24];
	int m_freefallState;
	float m_freefallStartTime;
	float m_freefallEndTime;
	float m_freefallAnticipateStartTime;
	float m_freefallAnticipateEndTime;
	float m_freefallDistanceToLand;
	float m_skydiveDiveAngle;
	bool m_skydiveIsDiving;
	char gap_40b5[3];
	float m_skydiveSpeed;
	float m_skydiveStrafeAngle;
	bool m_skydiveFreelookEnabled;
	char gap_40c1[3];
	Vector3D m_skydiveFreelookLockedAngle;
	float m_skydivePlayerPitch;
	float m_skydivePlayerYaw;
	bool m_skydiveFollowing;
	char gap_40d9[3];
	Vector3D m_skydiveUnfollowVelocity;
	char gap_40e8[1];
	bool m_skydiveIsNearLeviathan;
	char gap_40ea[2];
	Vector3D m_skydiveLeviathanHitPosition;
	Vector3D m_skydiveLeviathanHitNormal;
	Vector3D m_skydiveSlipVelocity;
	char gap_4110[16];
	C_KnockBack m_playerKnockBacks[4];
	char pad_41a0[32];
};

static_assert(sizeof(C_Player) == 0x41C0);

// move to combatcharacter!
inline C_WeaponX* (*C_BaseCombatCharacter__GetActiveWeapon)(C_BaseCombatCharacter* thisptr);

inline float (*C_Player__GetZoomFrac)(C_Player* thisptr);
inline int (*C_Player__GetAimSpeed)(C_Player* thisptr, bool isZoomed);
inline bool (*C_Player__IsInTimeShift)(C_Player* thisptr);

inline void (*C_Player__CurveLook)(C_Player* player, CInput::UserInput_t* input, float a3, float a4, float a5, int a6, float inputSampleFrametime, bool runAimAssist, JoyAngle_t* joyAngle);

///////////////////////////////////////////////////////////////////////////////
class V_Player : public IDetour
{
	virtual void GetAdr(void) const
	{
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 0F 2F 87").FollowNearCallSelf().GetPtr(C_Player__GetZoomFrac);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 83 F8 ?? 74 ?? F2 0F 10 8B").FollowNearCallSelf().GetPtr(C_Player__GetAimSpeed);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 3A D8 75").FollowNearCallSelf().GetPtr(C_Player__IsInTimeShift);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? 48 8D 45").FollowNearCallSelf().GetPtr(C_Player__CurveLook);

		g_GameDll.FindPatternSIMD("48 83 EC ?? 48 8B 01 FF 90 ?? ?? ?? ?? 48 83 C0 ?? 4C 8D 40").GetPtr(C_BaseCombatCharacter__GetActiveWeapon);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // C_PLAYER_H
