#ifndef CLIENT_INPUT_H
#define CLIENT_INPUT_H
#include "inputsystem/inputstacksystem.h"

#include "game/shared/weapon_types.h"
#include "game/shared/shared_activity.h"
#include "game/shared/ehandle.h"
#include "game/shared/pingcmd.h"
#include "game/shared/usercmd.h"
#include "game/client/iinput.h"

#include "kbutton.h"

FORWARD_DECLARE_HANDLE( InputContextHandle_t );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CKeyboardKey
{
public:
	// Name for key
	char				name[ 32 ];
	// Pointer to the underlying structure
	kbutton_t			*pkey;
	// Next key in key list.
	CKeyboardKey		*next;
};

class CInput : public IInput
{
	friend class C_Player;
public:
	virtual void sub_140701C40() = 0;
	virtual void sub_140701D60() = 0;
	virtual void sub_140701D90() = 0;

	virtual int GetMaxAbilityBindingCount( void ) = 0;
	virtual int GetMaxAbilityBindingLength( void ) = 0;

	virtual void sub_140701DC0() = 0;

	virtual void SetJoystickDebounce( int nSlot = -1 ) = 0;
	virtual void ClearJoystickDebounce( int nSlot = -1 ) = 0;

	virtual float		GetLastSwapSelectTime() = 0;
	virtual void		SetLastSwapSelectTime( float lastSelectTime ) = 0;
	virtual float		Unknown_GetFloat20() = 0;
	virtual void		Unknown_SetFloat20( float a2 ) = 0;
	virtual float		Unknown_GetFloat24() = 0;
	virtual void		Unknown_SetFloat24( float a2 ) = 0;

	virtual float		GetLastButtonPressTime( void ) = 0;
	virtual void		SetLastButtonPressTime( float lastPressTime ) = 0;
	virtual void		LinkButtonPair( int button1, int button2, int output ) = 0;

	virtual void sub_140701720() = 0;

	virtual void SetCustomWeaponActivity( sharedactivity_e weaponActivity ) = 0;
	virtual void ActivateOffhandWeaponByIndex( char index /*TODO[ AMOS ]: needs enum, enumVal 6 = INVALID; see Script_ActivateOffhandWeaponByIndex*/ ) = 0;

	virtual void ChangeControllerMode( bool newMode ) = 0;

	virtual void SetZoomScale( float zoomScale ) = 0;      // Used for SetBigMapZoomScale script func
	virtual float GetZoomScale() = 0;                      // Used for GetBigMapZoomScale script func
	virtual void SetZoomAnchor( Vector2D& inAnchor ) = 0;  // Used for SetBigMapZoomAnchor script func
	virtual void GetZoomAnchor( Vector2D& outAnchor ) = 0; // Used for SetBigMapZoomAnchor script func

public: // Hook statics
	static void VSetCustomWeaponActivity( CInput* pInput, sharedactivity_e weaponActivity );

protected:
	typedef struct
	{
		unsigned int AxisFlags;
		unsigned int AxisMap;
		unsigned int ControlMap;
	} joy_axis_t;

	// NOTE: this has to be public because we otherwise couldn't properly set
	// the prototypes for functions we obtain from the engine through our sig
	// scanner.
public:
	struct UserInput_t
	{
		float m_flAccumulatedMouseXMovement;
		float m_flAccumulatedMouseYMovement;
		float m_flRemainingJoystickSampleTime;
		float m_flKeyboardSampleTime;
		_BYTE gap10[12];
		float m_flLastSwapSelectTime;
		float m_flUnk20;
		float m_flUnk24;
		float m_flLastButtonPressTime;

		// Joystick Axis data
		joy_axis_t m_rgAxes[ MAX_JOYSTICK_AXES ];

		// Is the 3rd person camera using the mouse?
		bool m_fCameraInterceptingMouse;
		// Are we in 3rd person view?
		bool m_fCameraInThirdPerson;
		// Should we move view along with mouse?
		bool m_fCameraMovingWithMouse;
		// What is the current camera offset from the view origin?
		Vector3D m_vecCameraOffset;
		// Is the camera in distance moving mode?
		bool m_fCameraDistanceMove;
		// Old and current mouse position readings.
		int m_nCameraOldX;
		int m_nCameraOldY;
		int m_nCameraX;
		int m_nCameraY;

		float unkFloat_B0;
		int m_nPreviousCmdButtons;
		bool m_unknown_A0;
		float m_flUnknownTime_A4;
		_BYTE gapB0[8];

		// User linked button pairs.
		LinkedInputPairs_t m_linkedInput;

		QAngle m_angPreviousViewAngles;
		QAngle m_angPreviousViewAnglesTilt;
		float m_flLastForwardMove;
		int m_nClearInputState;
		CUserCmdExtended* m_pCommands;
		PingCommand_s m_pingCommands[ NUM_PING_COMMANDS ];
		int m_nPrimarySelectedInventorySlot;
		int m_nSecondarySelectedInventorySlot_MAYBE;
		bool m_bUnknown160;
		bool m_bUnknown161;
		__int16 m_weaponSelect_Or_weaponMods_see_140701430;
		sharedactivity_e m_weaponActivity;
		bool m_activatedWeaponIndex;
		CameraThirdData_t* m_pCameraThirdData;
		int m_nCamCommand;
		float m_flPreviousJoystickForwardMove;
		float m_flPreviousJoystickSideMove;
		float m_flPreviousJoystickYaw;
		float m_flPreviousJoystickPitch;
		float m_flJoystickDebounceYawAmount;
		float m_flJoystickDebouncePitchAmount;
		bool m_bJoystickDebounced;
		float m_flJoystickDebounceYaw;
		float m_flJoystickDebouncePitch;
		float m_flSomeInputSampleFrameTime;
		_BYTE gap1A0[4];

		// Set until polled by CreateMove and cleared
		CHandle< void* /*C_WeaponX*/ > m_hSelectedWeapon;
		bool m_bAutoAim_UnknownBool1AC;
		bool m_bAutoAim_UnknownBool1AD;
		bool m_bUnknownBool1AE;
		bool m_bUnknownBool1AF;
		bool m_bAutoAim_UnknownBool1B0;
		bool m_bAutoAim_UnknownBool1B1;
		bool m_bUnknownBool1B2;
		float m_flUnknownFloat1B4;
		float m_flUnknownFloat1B8;
		QAngle m_angUnknown1BC;
		QAngle m_angUnknown1C8;
		bool m_bUnknown1D4;
		bool m_bUnknown1D5;
		bool m_bZooming;
		_BYTE gap1D7[29];
		float m_flZoomScale;
		Vector2D m_vecZoomAnchor;
		_BYTE gap200_endsAt_E30[3120];
	};

private:
	// Has the mouse been initialized?
	bool m_fMouseInitialized;
	// Is the mosue active?
	bool m_fMouseActive;
	// Has the joystick advanced initialization been run?
	bool m_fJoystickAdvancedInit;
	// Between controller and mouse, what's the primary input
	bool m_bControllerMode;

	bool m_bUnk1;
	bool m_bUnk2;
	bool m_bUnk3;
	bool m_bUnk4;

	// List of queryable keys
	CKeyboardKey* m_pKeys;

	UserInput_t m_User;

	InputContextHandle_t m_hInputContext;
	InputContext_t m_hPushedInputContext;
};

inline void(*v_CInput__SetCustomWeaponActivity)(CInput* pInput, sharedactivity_e weaponActivity);

inline IInput* g_pInput_VFTable = nullptr;
inline CInput* g_pInput = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VInput : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CInput::`vftable'", g_pInput_VFTable);
		LogFunAdr("CInput::SetCustomWeaponActivity", v_CInput__SetCustomWeaponActivity);
		LogVarAdr("g_Input", g_pInput);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("89 91 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC CC F3 0F 11 89 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC F3 0F 10 81 ?? ?? ?? ??")
			.GetPtr(v_CInput__SetCustomWeaponActivity);
	}
	virtual void GetVar(void) const
	{
		g_pInput = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B 5D 57").FollowNearCallSelf().
			FindPatternSelf("48 8B 05").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CInput*>();
	}
	virtual void GetCon(void) const
	{
		g_pInput_VFTable = g_GameDll.GetVirtualMethodTable(".?AVCInput@@").RCast<IInput*>();
	}
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // CLIENT_INPUT_H
