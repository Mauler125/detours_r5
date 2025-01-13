//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#ifndef INPUTENUMS_H
#define INPUTENUMS_H

// Standard maximum +/- value of a joystick axis
#define MAX_BUTTONSAMPLE			32768

#if !defined( _X360 )
#define INVALID_USER_ID		-1
#else
#define INVALID_USER_ID		XBX_INVALID_USER_ID
#endif

enum
{
#ifdef _PS3
	MAX_JOYSTICKS = 7,
#else
	MAX_JOYSTICKS = 4,
#endif
	MOUSE_BUTTON_COUNT = 5,
	MAX_NOVINT_DEVICES = 2,
};

enum JoystickAxis_t
{
	JOY_AXIS_X = 0,
	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_R,
	JOY_AXIS_U,
	JOY_AXIS_V,
	MAX_JOYSTICK_AXES,
};

enum JoystickType_t
{
	JOYSTICK_TYPE_NONE = 0,
	JOYSTICK_TYPE_XBOX360,
	JOYSTICK_TYPE_XBOX1,
	JOYSTICK_TYPE_PS4
};

//-----------------------------------------------------------------------------
// Events
//-----------------------------------------------------------------------------
enum InputEventType_t
{
	IE_ButtonPressed = 0,	// m_nData contains a ButtonCode_t
	IE_ButtonReleased,		// m_nData contains a ButtonCode_t
	IE_ButtonDoubleClicked,	// m_nData contains a ButtonCode_t
	IE_AnalogValueChanged,	// m_nData contains an AnalogCode_t, m_nData2 contains the value

	IE_Unknown8 = 8, // Unknown what this is/does, its used in [r5apex.exe+0x297722] and [r5apex.exe+0x297ACD]

	IE_FirstSystemEvent = 100,
	IE_Quit = IE_FirstSystemEvent,
	IE_ControllerInserted,	// m_nData contains the controller ID
	IE_ControllerUnplugged,	// m_nData contains the controller ID
	IE_Close,
	IE_WindowSizeChanged,	// m_nData contains width, m_nData2 contains height, m_nData3 = 0 if not minimized, 1 if minimized
	IE_PS_CameraUnplugged,  // m_nData contains code for type of disconnect.  
	IE_PS_Move_OutOfView,   // m_nData contains bool (0, 1) for whether the move is now out of view (1) or in view (0)

	IE_FirstUIEvent = 200,
	IE_LocateMouseClick = IE_FirstUIEvent,
	IE_SetCursor,
	IE_KeyTyped,
	IE_KeyCodeTyped,
	IE_InputLanguageChanged,
	IE_IMESetWindow,
	IE_IMEStartComposition,
	IE_IMEComposition,
	IE_IMEEndComposition,
	IE_IMEShowCandidates,
	IE_IMEChangeCandidates,
	IE_IMECloseCandidates,
	IE_IMERecomputeModes,
	IE_OverlayEvent,

	IE_FirstVguiEvent = 1000,	// Assign ranges for other systems that post user events here
	IE_FirstAppEvent = 2000,
};

struct InputEvent_t
{
	int m_nType;				// Type of the event (see InputEventType_t)
	int m_nTick;				// Tick on which the event occurred
	int m_nData;				// Generic 32-bit data, what it contains depends on the event
	int m_nData2;				// Generic 32-bit data, what it contains depends on the event
	int m_nData3;				// Generic 32-bit data, what it contains depends on the event
};

#endif // INPUTENUMS_H
