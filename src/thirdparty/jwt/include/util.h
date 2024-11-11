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
 *  @file util.h
 *  @author Raphael Beck
 *  @brief Useful utility functions.
 */

#ifndef L8W8JWT_UTIL_H
#define L8W8JWT_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include "version.h"

/**
 * Converts a hex-encoded string to a binary array. <p>
 * A NUL-terminator is appended at the end of the output buffer, so make sure to allocate at least <c>(hexstr_length / 2) + 1</c> bytes!
 * @param hexstr The hex string to convert.
 * @param hexstr_length Length of the \p hexstr
 * @param output Where to write the converted binary data into.
 * @param output_size Size of the output buffer (make sure to allocate at least <c>(hexstr_length / 2) + 1</c> bytes!).
 * @param output_length [OPTIONAL] Where to write the output array length into. This is always gonna be <c>hexstr_length / 2</c>, but you can still choose to write it out just to be sure. If you want to omit this: no problem.. just pass <c>NULL</c>!
 * @return <c>0</c> if conversion succeeded. <c>1</c> if one or more required arguments were <c>NULL</c> or invalid. <c>2</c> if the hexadecimal string is in an invalid format (e.g. not divisible by 2). <c>3</c> if output buffer size was insufficient (needs to be at least <c>(hexstr_length / 2) + 1</c> bytes).
 */
L8W8JWT_API int l8w8jwt_hexstr2bin(const char* hexstr, const size_t hexstr_length, unsigned char* output, const size_t output_size, size_t* output_length);

/**
 * Compares two strings ignoring UPPER vs. lowercase.
 * @param str1 String to compare.
 * @param str2 String to compare to.
 * @param n How many characters of the string should be compared (starting from index 0)?
 * @return If the strings are equal, <code>0</code> is returned. Otherwise, something else.
 */
L8W8JWT_API int l8w8jwt_strncmpic(const char* str1, const char* str2, size_t n);

#ifndef L8W8JWT_PLATFORM_MALLOC_ALT
/**
 * Set this pre-processor definition to \c 1 if you need to
 * provide custom implementation of malloc
 */
#define L8W8JWT_PLATFORM_MALLOC_ALT 0
#endif

#ifndef L8W8JWT_PLATFORM_CALLOC_ALT
/**
 * Set this pre-processor definition to \c 1 if you need to
 * provide custom implementation of calloc
 */
#define L8W8JWT_PLATFORM_CALLOC_ALT 0
#endif

#ifndef L8W8JWT_PLATFORM_REALLOC_ALT
/**
 * Set this pre-processor definition to \c 1 if you need to
 * provide custom implementation of realloc
 */
#define L8W8JWT_PLATFORM_REALLOC_ALT 0
#endif

#if L8W8JWT_PLATFORM_MALLOC_ALT
extern void *(*l8w8jwt_malloc) (size_t size);
#else
#define l8w8jwt_malloc malloc
#endif

#if L8W8JWT_PLATFORM_CALLOC_ALT
extern void *(*l8w8jwt_calloc) (size_t nmemb, size_t size);
#else
#define l8w8jwt_calloc calloc
#endif

#if L8W8JWT_PLATFORM_REALLOC_ALT
extern void *(*l8w8jwt_realloc) (void *ptr, size_t size);
#else
#define l8w8jwt_realloc realloc
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // L8W8JWT_UTIL_H