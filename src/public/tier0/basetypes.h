#pragma once
#include "common/sdkint.h"

/*-----------------------------------------------------------------------------
 * _basetypes
 *-----------------------------------------------------------------------------*/

// This is a trick to get the DLL extension off the -D option on the command line.
#define DLLExtTokenPaste(x) #x
#define DLLExtTokenPaste2(x) DLLExtTokenPaste(x)
#define DLL_EXT_STRING DLLExtTokenPaste2( _DLL_EXT )

//-----------------------------------------------------------------------------
// Set up platform defines.
//-----------------------------------------------------------------------------
#ifdef _WIN32
#define IsPlatformLinux()	0
#define IsPlatformPosix()	0
#define IsPlatformOSX()		0
#define IsOSXOpenGL()		0
#define IsPlatformPS3()		0
#define IsPlatformPS3_PPU()	0
#define IsPlatformPS3_SPU()	0
#define PLATFORM_WINDOWS	1
#define PLATFORM_OPENGL 0

#ifndef _X360
#define IsPlatformX360() 0
#define IsPlatformWindowsPC() 1
#define PLATFORM_WINDOWS_PC 1

#ifdef _WIN64
#define IsPlatformWindowsPC64() 1
#define IsPlatformWindowsPC32() 0
#define PLATFORM_WINDOWS_PC64 1
#else
#define IsPlatformWindowsPC64() 0
#define IsPlatformWindowsPC32() 1
#define PLATFORM_WINDOWS_PC32 1
#endif

#else // _X360

#define IsPlatformWindowsPC()	0
#define IsPlatformWindowsPC64() 0
#define IsPlatformWindowsPC32() 0
#define IsPlatformX360()		1
#define PLATFORM_X360 1

#endif // _X360
#elif defined(_PS3)

// Adding IsPlatformOpenGL() to help fix a bunch of code that was using IsPosix() to infer if the DX->GL translation layer was being used.
#if defined( DX_TO_GL_ABSTRACTION )
#define IsPlatformOpenGL() true
#else
#define IsPlatformOpenGL() false
#endif

#define IsPlatformX360()		0
#define IsPlatformPS3()			1
#ifdef SPU
#define IsPlatformPS3_PPU()		0
#define IsPlatformPS3_SPU()		1
#else
#define IsPlatformPS3_PPU()		1
#define IsPlatformPS3_SPU()		0
#endif
#define IsPlatformWindowsPC()	0
#define IsPlatformWindowsPC64()	0
#define IsPlatformWindowsPC32()	0
#define IsPlatformPosix()		1
#define PLATFORM_POSIX 1
#define PLATFORM_OPENGL 0

#define IsPlatformLinux() 0
#define IsPlatformOSX() 0
#define IsOSXOpenGL() 0


#elif defined(POSIX)
#define IsPlatformX360()		0
#define IsPlatformPS3()			0
#define IsPlatformPS3_PPU()		0
#define IsPlatformPS3_SPU()		0
#define IsPlatformWindowsPC()	0
#define IsPlatformWindowsPC64()	0
#define IsPlatformWindowsPC32()	0
#define IsPlatformPosix()		1
#define PLATFORM_POSIX 1

#if defined( LINUX ) && !defined( OSX ) // for havok we define both symbols, so don't let the osx build wander down here
#define IsPlatformLinux() 1
#define IsPlatformOSX() 0
#define IsOSXOpenGL() 0
#define PLATFORM_OPENGL 0
#define PLATFORM_LINUX 1
#elif defined ( OSX )
#define IsPlatformLinux() 0
#define IsPlatformOSX() 1
#define IsOSXOpenGL() 1
#define PLATFORM_OSX 1
#define PLATFORM_OPENGL 1
#else
#define IsPlatformLinux() 0
#define IsPlatformOSX() 0
#define IsOSXOpenGL() 0
#define PLATFORM_OPENGL 0
#endif

#else
#error
#endif

//////////////////////////////////////////////////////////////////////////

//#ifndef schema
//#define schema namespace ValveSchemaMarker {}
//#endif
//#define noschema
//#define schema_pragma( ... )
//#define META( ... )
//#define TYPEMETA( ... )

//-----------------------------------------------------------------------------
// Old-school defines we're going to support since much code uses them
//-----------------------------------------------------------------------------
#define IsLinux()	IsPlatformLinux() 
#define IsOSX()		IsPlatformOSX()
#define IsPosix()	IsPlatformPosix()
#define IsX360()	IsPlatformX360()
#define IsPS3()		IsPlatformPS3()

#define MAX_SPLITSCREEN_CLIENT_BITS 0 // R5 doesn't support splitscreen; engine is hardcoded to only have 1 player.
#define MAX_SPLITSCREEN_CLIENTS	( 1 << MAX_SPLITSCREEN_CLIENT_BITS ) // 1 // this should == MAX_JOYSTICKS in InputEnums.h

#define MAX_PLAYERS 128 // Absolute max R5 players.
#define MAX_TEAMS   126 // Absolute max R5 teams.

#define MAX_MAP_NAME_HOST 64 // Max host BSP file name len.
#define MAX_MAP_NAME      64 // Max BSP file name len.

#define SDK_VERSION "VGameSDK012" // Increment this with every /breaking/ SDK change (i.e. security/backend changes breaking compatibility).
#define SDK_ARRAYSIZE(arr) ((sizeof(arr) / sizeof(*arr))) // Name due to IMGUI implementation and NT implementation that we shouldn't share across everywhere.

#define SDK_SYSTEM_CFG_PATH "cfg/system/"
#define SDK_USER_CFG_PATH "cfg/user/"

#define VALID_CHARSTAR(star) (star && star[0]) // Check if char* is valid and not empty.

// #define COMPILETIME_MAX and COMPILETIME_MIN for max/min in constant expressions
#define COMPILETIME_MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#define COMPILETIME_MAX( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#ifndef MIN
#define MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

#ifndef MAX
#define MAX( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#endif

#ifdef __cplusplus

template< class T, class Y, class X >
inline T clamp(T const& val, Y const& minVal, X const& maxVal)
{
	if (val < minVal)
		return minVal;
	else if (val > maxVal)
		return maxVal;
	else
		return val;
}

// This is the preferred clamp operator. Using the clamp macro can lead to
// unexpected side-effects or more expensive code. Even the clamp (all
// lower-case) function can generate more expensive code because of the
// mixed types involved.
template< class T >
T Clamp(T const& val, T const& minVal, T const& maxVal)
{
	if (val < minVal)
		return minVal;
	else if (val > maxVal)
		return maxVal;
	else
		return val;
}

// This is the preferred Min operator. Using the MIN macro can lead to unexpected
// side-effects or more expensive code.
template< class T >
T Min(T const& val1, T const& val2)
{
	return val1 < val2 ? val1 : val2;
}

// This is the preferred Max operator. Using the MAX macro can lead to unexpected
// side-effects or more expensive code.
template< class T >
T Max(T const& val1, T const& val2)
{
	return val1 > val2 ? val1 : val2;
}

template <typename T>
void Swap(T& a, T& b)
{
	T temp = a;
	a = b;
	b = temp;
}

template <typename T>
inline T AlignValue(T val, uintptr_t alignment)
{
	return (T)(((uintp)val + alignment - 1) & ~(alignment - 1));
}

// Pad a number so it lies on an N byte boundary.
// So PAD_NUMBER(0,4) is 0 and PAD_NUMBER(1,4) is 4
#define PAD_NUMBER(number, boundary) \
	( ((number) + ((boundary)-1)) / (boundary) ) * (boundary)

#else

#define clamp(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))

#endif

#define fsel(c,x,y) ( (c) >= 0 ? (x) : (y) )

// integer conditional move
// if a >= 0, return x, else y
#define isel(a,x,y) ( ((a) >= 0) ? (x) : (y) )

// if x = y, return a, else b
#define ieqsel(x,y,a,b) (( (x) == (y) ) ? (a) : (b))

// if the nth bit of a is set (counting with 0 = LSB),
// return x, else y
// this is fast if nbit is a compile-time immediate 
#define ibitsel(a, nbit, x, y) ( ( ((a) & (1 << (nbit))) != 0 ) ? (x) : (y) )

// MSVC CRT uses 0x7fff while gcc uses MAX_INT, leading to mismatches between platforms
// As a result, we pick the least common denominator here.  This should be used anywhere
// you might typically want to use RAND_MAX
#define VALVE_RAND_MAX 0x7fff

#define FORWARD_DECLARE_HANDLE(name) typedef struct name##__ *name

#ifndef NOTE_UNUSED
#define NOTE_UNUSED(x)	(void)(x)	// for pesky compiler / lint warnings
#endif

#ifndef M_PI
	#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#define M_PI_F		((float)(M_PI))	// Shouldn't collide with anything.

// NJS: Inlined to prevent floats from being autopromoted to doubles, as with the old system.
#ifndef RAD2DEG
	#define RAD2DEG( x  )  ( (float)(x) * (float)(180.f / M_PI_F) )
#endif

#ifndef DEG2RAD
	#define DEG2RAD( x  )  ( (float)(x) * (float)(M_PI_F / 180.f) )
#endif

typedef int qboolean;

typedef float				vec_t;
typedef float				vec3_t[3];

struct vrect_t
{
	int      x, y, width, height;
	vrect_t* pnext;
};

constexpr int MSG_NOSIGNAL             = 0;

//-----------------------------------------------------------------------------
// Declares a type-safe handle type; you can't assign one handle to the next
//-----------------------------------------------------------------------------

// 32-bit pointer handles.

// Typesafe 8-bit and 16-bit handles.
template< class HandleType >
class CBaseIntHandle
{
public:

	inline bool			operator==(const CBaseIntHandle& other) { return m_Handle == other.m_Handle; }
	inline bool			operator!=(const CBaseIntHandle& other) { return m_Handle != other.m_Handle; }

	// Only the code that doles out these handles should use these functions.
	// Everyone else should treat them as a transparent type.
	inline HandleType	GetHandleValue() { return m_Handle; }
	inline void			SetHandleValue(HandleType val) { m_Handle = val; }

	typedef HandleType	HANDLE_TYPE;

protected:

	HandleType	m_Handle;
};

template< class DummyType >
class CIntHandle16 : public CBaseIntHandle< unsigned short >
{
public:
	inline			CIntHandle16() {}

	static inline	CIntHandle16<DummyType> MakeHandle(HANDLE_TYPE val)
	{
		return CIntHandle16<DummyType>(val);
	}

protected:
	inline			CIntHandle16(HANDLE_TYPE val)
	{
		m_Handle = val;
	}
};


template< class DummyType >
class CIntHandle32 : public CBaseIntHandle< uint32 >
{
public:
	inline			CIntHandle32() {}

	static inline	CIntHandle32<DummyType> MakeHandle(HANDLE_TYPE val)
	{
		return CIntHandle32<DummyType>(val);
	}

protected:
	inline			CIntHandle32(HANDLE_TYPE val)
	{
		m_Handle = val;
	}
};


// NOTE: This macro is the same as windows uses; so don't change the guts of it
#define DECLARE_HANDLE_16BIT(name)	typedef CIntHandle16< struct name##__handle * > name;
#define DECLARE_HANDLE_32BIT(name)	typedef CIntHandle32< struct name##__handle * > name;

#define DECLARE_POINTER_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#define FORWARD_DECLARE_HANDLE(name) typedef struct name##__ *name

#define DECLARE_DERIVED_POINTER_HANDLE( _name, _basehandle ) struct _name##__ : public _basehandle##__ {}; typedef struct _name##__ *_name
#define DECLARE_ALIASED_POINTER_HANDLE( _name, _alias ) typedef struct _alias##__ *name

#define ExecuteNTimes( nTimes, x )	\
	{								\
	static int __executeCount=0;\
	if ( __executeCount < nTimes )\
		{							\
			++__executeCount;		\
			x;						\
		}							\
	}


#define ExecuteOnce( x )			ExecuteNTimes( 1, x )

#define UID_PREFIX generated_id_
#define UID_CAT1(a,c) a ## c
#define UID_CAT2(a,c) UID_CAT1(a,c)
#define EXPAND_CONCAT(a,c) UID_CAT1(a,c)
#ifdef _MSC_VER
#define UNIQUE_ID UID_CAT2(UID_PREFIX,__COUNTER__)
#else
#define UNIQUE_ID UID_CAT2(UID_PREFIX,__LINE__)
#endif

#define _MKSTRING(arg) #arg
#define MKSTRING(arg) _MKSTRING(arg)
