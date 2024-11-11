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
 *  @file encode.h
 *  @author Raphael Beck
 *  @brief Core ENCODE function for l8w8jwt. Use this to encode a JWT header + payload WITHOUT signing.
 */

#ifndef L8W8JWT_ENCODE_H
#define L8W8JWT_ENCODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "algs.h"
#include "claim.h"
#include "version.h"
#include "retcodes.h"
#include "timehelper.h"
#include <stddef.h>

#ifndef L8W8JWT_MAX_KEY_SIZE
#define L8W8JWT_MAX_KEY_SIZE 8192
#endif 

/**
 * Struct containing the parameters to use for creating a JWT with l8w8jwt.
 */
L8W8JWT_API struct l8w8jwt_encoding_params
{
    /**
     * The signature algorithm ID. <p>
     * [0;2] = HS256/384/512 | [3;5] = RS256/384/512 | [6;8] = PS256/384/512 | [9;11] = ES256/384/512
     */
    int alg;

    /**
     * [OPTIONAL] The issuer claim (who issued the JWT?). Can be omitted by setting this to <code>NULL</code>.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.1
     */
    char* iss;

    /**
     * iss claim string length.
     */
    size_t iss_length;

    /**
     * [OPTIONAL] The subject claim (who is the JWT about?). Set to <code>NULL</code> if you don't want it in your token.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.2
     */
    char* sub;

    /**
     * sub claim string length.
     */
    size_t sub_length;

    /**
     * [OPTIONAL] The audience claim (who is the JWT intended for? Who is the intended JWT's recipient?).
     * Set this to <code>NULL</code> if you don't wish to add this claim to the token.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.3
     */
    char* aud;

    /**
     * aud claim string length.
     */
    size_t aud_length;

    /**
     * [OPTIONAL] The JWT ID. Provides a unique identifier for the token. Can be omitted by setting this to <code>NULL</code>.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.7
     */
    char* jti;

    /**
     * jti claim string length.
     */
    size_t jti_length;

    /**
     * Expiration time claim; specifies when this token should stop being valid (in seconds since Unix epoch). <p>
     * If you want to omit this, set this to <code>0</code>, but do NOT FORGET to set it to something,
     * otherwise it will be set to whatever random value was in the memory where this variable resides.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.4
     */
    l8w8jwt_time_t exp;

    /**
     * "Not before" time claim; specifies when this token should start being valid (in seconds since Unix epoch). <p>
     * If you want to omit this, set this to <code>0</code>, but do NOT FORGET to set it to something,
     * otherwise it will be set to whatever random value was in the memory where this variable resides.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.5
     */
    l8w8jwt_time_t nbf;

    /**
     * "Issued at" timestamp claim; specifies when this token was emitted (in seconds since Unix epoch). <p>
     * If you want to omit this, set this to <code>0</code>, but do NOT FORGET to set it to something,
     * otherwise it will be set to whatever random value was in the memory where this variable resides.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.6
     */
    l8w8jwt_time_t iat;

    /**
     * [OPTIONAL] Array of additional claims to include in the JWT's header like for example "kid" or "cty"; pass <code>NULL</code> if you don't wish to add any! <p>
     * Avoid header claims such as <code>typ</code> and <code>alg</code>, since those are written by the encoding function itself.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.7
     */
    struct l8w8jwt_claim* additional_header_claims;

    /**
     * [OPTIONAL] The additional_header_claims array size; pass <code>0</code> if you don't wish to include any custom claims!
     */
    size_t additional_header_claims_count;

    /**
     * [OPTIONAL] Array of additional claims to include in the JWT's payload; pass <code>NULL</code> if you don't wish to add any! <p>
     * Registered claim names such as "iss", "exp", etc... have their own dedicated field within this struct: do not include those in this array to prevent uncomfortable duplicates!
     * @see https://tools.ietf.org/html/rfc7519#section-4
     */
    struct l8w8jwt_claim* additional_payload_claims;

    /**
     * [OPTIONAL] The additional_payload_claims array size; pass <code>0</code> if you don't wish to include any custom claims!
     */
    size_t additional_payload_claims_count;

    /**
     * The secret key to use for signing the token
     * (e.g. if you chose HS256 as algorithm, this will be the HMAC secret; for RS512 this will be the private PEM-formatted RSA key string, and so on...).
     */
    unsigned char* secret_key;

    /**
     * Length of the secret_key
     */
    size_t secret_key_length;

    /**
     * If the secret key requires a password for usage, please assign it to this field. <p>
     * You can only omit this when using JWT algorithms "HS256", "HS384" or "HS512" (it's ignored in that case actually). <p>
     * Every other algorithm requires you to at least set this to <code>NULL</code> if the {@link #secret_key} isn't password-protected.
     */
    unsigned char* secret_key_pw;

    /**
     * The secret key's password length (if there is any). If there's none, set this to zero!
     */
    size_t secret_key_pw_length;

    /**
     * Where the encoded token should be written into
     * (will be malloc'ed, so make sure to <code>l8w8jwt_free()</code> this as soon as you're done using it!).
     */
    char** out;

    /**
     * Where the output token string length should be written into.
     */
    size_t* out_length;
};

/**
 * Initializes a {@link #l8w8jwt_encoding_params} instance by setting its fields to default values.
 * @param params The l8w8jwt_encoding_params to initialize (set to default values).
 */
L8W8JWT_API void l8w8jwt_encoding_params_init(struct l8w8jwt_encoding_params* params);

/**
 * Validates a set of l8w8jwt_encoding_params.
 * @param params The l8w8jwt_encoding_params to validate.
 * @return Return code as defined in retcodes.h
 */
L8W8JWT_API int l8w8jwt_validate_encoding_params(struct l8w8jwt_encoding_params* params);

/**
 * Creates, signs and encodes a Json-Web-Token. <p>
 * An example output could be: <code>eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6InNvbWUta2V5LWlkLWhlcmUtMDEyMzQ1NiJ9.eyJpYXQiOjE1Nzk2NDUzNTUsImV4cCI6MTU3OTY0NTk1NSwic3ViIjoiR29yZG9uIEZyZWVtYW4iLCJpc3MiOiJCbGFjayBNZXNhIiwiYXVkIjoiQWRtaW5pc3RyYXRvciJ9.uk4EEoq0ql_SguLto5EWzklakpzO-6GE2U26crB8vUY</code> <p>
 * @param params The token encoding parameters (e.g. "alg", "iss", "exp", etc...).
 * @return Return code as defined in retcodes.h
 * @see l8w8jwt_encoding_params
 */
L8W8JWT_API int l8w8jwt_encode(struct l8w8jwt_encoding_params* params);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // L8W8JWT_ENCODE_H
