/*
   Copyright 2020 Raphael Beck

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/**
 *  @file version.h
 *  @author Raphael Beck
 *  @brief l8w8jwt version checking.
 */

#ifndef L8W8JWT_VERSION_H
#define L8W8JWT_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Current l8w8jwt version number.
 */
#define L8W8JWT_VERSION 240

/**
 * Current l8w8jwt version number (as a human-readable string).
 */
#define L8W8JWT_VERSION_STR "2.4.0"

#if defined(_WIN32) && defined(L8W8JWT_DLL)
#ifdef L8W8JWT_BUILD_DLL
#define L8W8JWT_API __declspec(dllexport)
#else
#define L8W8JWT_API __declspec(dllimport)
#endif
#else
#define L8W8JWT_API
#endif

#ifndef L8W8JWT_SMALL_STACK
/**
 * Set this pre-processor definition to \c 1 if you're using this
 * on a low-memory device with increased risk of stack overflow.
 */
#define L8W8JWT_SMALL_STACK 0
#endif

#ifndef L8W8JWT_PLATFORM_TIME_ALT
/**
 * Set this pre-processor definition to \c 1 if you need to 
 * provide custom implementation of system time API.
 */
#define L8W8JWT_PLATFORM_TIME_ALT 0
#endif

/**
 * Free memory that was allocated by L8W8JWT.
 * @param mem The memory to free.
 */
L8W8JWT_API void l8w8jwt_free(void* mem);

/**
 * Gets the l8w8jwt version number as an integer.
 * @return Version number (e.g. "2.1.4" => 214)
 */
L8W8JWT_API int l8w8jwt_get_version_number(void);

/**
 * Gets the l8w8jwt version number as a nicely formatted string.
 * @param out A writable \c char buffer of at least 32B where to write the version number string into. The string will be NUL-terminated, no worries! Passing \c NULL here is a very bad idea. Undefined, unpleasant, and just... just don't!
 */
L8W8JWT_API void l8w8jwt_get_version_string(char out[32]);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // L8W8JWT_VERSION_H
