//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#ifndef IINPUTSYSTEM_H
#define IINPUTSYSTEM_H

#include "inputsystem/ButtonCode.h"
#include "inputsystem/AnalogCode.h"
#include "inputsystem/InputEnums.h"

///-----------------------------------------------------------------------------
/// A handle to a cursor icon
///-----------------------------------------------------------------------------
DECLARE_POINTER_HANDLE( InputCursorHandle_t );
#define INPUT_CURSOR_HANDLE_INVALID ( (InputCursorHandle_t)0 )

///-----------------------------------------------------------------------------
/// Input event callback
///-----------------------------------------------------------------------------
typedef bool ( *InputEventCallback_t ) ( const InputEvent_t& eventCallback );

///-----------------------------------------------------------------------------
/// An enumeration describing well-known cursor icons
///-----------------------------------------------------------------------------
enum InputStandardCursor_t
{
	INPUT_CURSOR_NONE = 0,
	INPUT_CURSOR_ARROW,
	INPUT_CURSOR_IBEAM,
	INPUT_CURSOR_HOURGLASS,
	INPUT_CURSOR_CROSSHAIR,
	INPUT_CURSOR_WAITARROW,
	INPUT_CURSOR_UP,
	INPUT_CURSOR_SIZE_NW_SE,
	INPUT_CURSOR_SIZE_NE_SW,
	INPUT_CURSOR_SIZE_W_E,
	INPUT_CURSOR_SIZE_N_S,
	INPUT_CURSOR_SIZE_ALL,
	INPUT_CURSOR_NO,
	INPUT_CURSOR_HAND,

	INPUT_CURSOR_COUNT
};

//-----------------------------------------------------------------------------
// Main interface for input. This is a low-level interface
//-----------------------------------------------------------------------------
#define INPUTSYSTEM_INTERFACE_VERSION	"InputSystemVersion001"
abstract_class IInputSystem : public IAppSystem
{
public:
	/// Attach, detach input system from a particular window
	/// This window should be the root window for the application
	/// Only 1 window should be attached at any given time.
	virtual void AttachToWindow( const void* const hWnd ) = 0;
	virtual void DetachFromWindow( ) = 0;

	/// Enables/disables input. PollInputState will not update current 
	/// button/analog states when it is called if the system is disabled.
	virtual void EnableInput( const bool bEnable ) = 0;

	/// Enables/disables the windows message pump. PollInputState will not.
	/// Peek/Dispatch messages if this is disabled.
	virtual void EnableMessagePump( const bool bEnable ) = 0;

	/// Gets the time of the last polling in ms.
	virtual int GetPollTick() const = 0;

	/// Is a button down? "Buttons" are binary-state input devices (mouse buttons, keyboard keys).
	virtual bool IsButtonDown( const ButtonCode_t code ) const = 0;

	/// Returns the tick at which the button was pressed and released.
	virtual int GetButtonPressedTick( const ButtonCode_t code ) const = 0;

	/// Returns the joystick deadzone index for connected hardware.
	virtual JoystickType_t GetJoystickType( ) const = 0;

	/// DoNothing; VFTable padding.
	virtual bool ReturnFalse( ) const = 0;

	/// Polls the current input state.
	virtual void PollInputState( const InputEventCallback_t eventCallback ) = 0;

	/// Posts a user-defined event into the event queue; this is expected
	/// to be called in overridden wndprocs connected to the root panel.
	virtual void PostUserEvent( const InputEvent_t &event ) = 0;
	virtual void PostUserEvent( const InputEventType_t type ) = 0;

	/// Returns the number of joysticks
	virtual int GetJoystickCount( ) const = 0;

	/// Sample the joystick and append events to the input queue.
	virtual void SampleDevices( void ) = 0;

	virtual void SetRumble( const float fLeftMainMotor, const float fRightMainMotor, const float fLeftTriggerMotor, const float fRightTriggerMotor, const int userId = INVALID_USER_ID ) = 0;
	virtual void StopRumble( const int userId = INVALID_USER_ID ) = 0;

	/// Resets the input state.
	virtual void ResetInputState() = 0;

	/// Convert back + forth between ButtonCode/AnalogCode + strings.
	virtual const char* ButtonCodeToString( const ButtonCode_t code ) const = 0;
	virtual ButtonCode_t StringToButtonCode( const char* const pString ) const = 0;

	/// Sleeps until input happens. Pass a negative number to sleep infinitely.
	virtual void SleepUntilInput( const int nMaxSleepTimeMS = -1 ) = 0;

	/// Convert back + forth between virtual codes + button codes
	virtual ButtonCode_t VirtualKeyToButtonCode( const int nVirtualKey ) const = 0;
	virtual int ButtonCodeToVirtualKey( const ButtonCode_t code ) const = 0;

	/// Sets the cursor position.
	virtual void SetCursorPosition( const int x, const int y ) = 0;

	/// Tells the input system to generate UI-related events, defined
	/// in inputsystem/inputenums.h (see IE_FirstUIEvent)
	/// We could have multiple clients that care about UI-related events
	/// so we refcount the clients with an Add/Remove strategy. If there
	/// are no interested clients, the UI events are not generated.
	virtual void AddUIEventListener() = 0;
	virtual void RemoveUIEventListener() = 0;

	/// Creates a cursor using one of the well-known cursor icons.
	virtual InputCursorHandle_t GetStandardCursor( const InputStandardCursor_t id ) = 0;

	/// Loads a cursor defined in a file.
	virtual InputCursorHandle_t LoadCursorFromFile( const char* const pFileName, const char* const pPathID = NULL ) = 0;

	/// Sets the cursor icon.
	virtual void SetCursorIcon( const InputCursorHandle_t hCursor ) = 0;

	/// Gets the cursor position.
	virtual void GetCursorPosition( const int* const pX, const int* const pY ) = 0;

	/// Mouse capture.
	virtual void EnableMouseCapture( const PlatWindow_t hWnd ) = 0;
	virtual void DisableMouseCapture( ) = 0;

	// Mouse/Joystick cursor visibility, tell inputsystem when we hide stuff rather than querying the OS which is expensive on OSX.
	virtual void SetMouseCursorVisible( const bool bVisible ) = 0;
	virtual void SetJoystickCursorVisible( const bool bVisible ) = 0;

	/// Reset the current cursor icon.  Used to reset the icon in the case of alt+tabs where the cursor has been forced to a different
	/// icon because it was outside of the client rect during the reload.
	virtual void ResetCursorIcon() = 0;

	// read and clear accumulated raw input values.
	virtual void GetRawMouseAccumulators( int& accumX, int& accumY ) = 0;
};

#endif // IINPUTSYSTEM_H
