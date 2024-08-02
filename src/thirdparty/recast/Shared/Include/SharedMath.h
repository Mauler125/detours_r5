/**
@defgroup shared Shared

Members in this module are wrappers around the standard math library
*/

#ifndef RECASTDETOURMATH_H
#define RECASTDETOURMATH_H

#include <math.h>

/// The value of PI used by Recast & Detour.
static const float RD_PI = 3.14159265f;

/// The value of Epsilon used by Recast & Detour.
static const float RD_EPS = 1e-6f;

inline float rdMathFabsf(float x) { return fabsf(x); }
inline float rdMathSqrtf(float x) { return sqrtf(x); }
inline float rdMathFloorf(float x) { return floorf(x); }
inline float rdMathCeilf(float x) { return ceilf(x); }
inline float rdMathRoundf(float x) { return roundf(x); }
inline float rdMathCosf(float x) { return cosf(x); }
inline float rdMathSinf(float x) { return sinf(x); }
inline float rdMathAtan2f(float y, float x) { return atan2f(y, x); }
inline bool rdMathIsfinite(float x) { return isfinite(x); }

#endif // RECASTDETOURMATH_H
