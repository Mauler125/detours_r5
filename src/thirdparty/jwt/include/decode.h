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
 *  @file decode.h
 *  @author Raphael Beck
 *  @brief Core DECODE function for l8w8jwt. Use this to decode and validate a JWT!
 */

#ifndef L8W8JWT_DECODE_H
#define L8W8JWT_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "algs.h"
#include "claim.h"
#include "version.h"
#include "retcodes.h"
#include "timehelper.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifndef L8W8JWT_MAX_KEY_SIZE
#define L8W8JWT_MAX_KEY_SIZE 8192
#endif

/**
 * Enum containing the validation result flags.
 */
enum l8w8jwt_validation_result {
    /**
     * The JWT is valid (according to the passed validation parameters).
     */
    L8W8JWT_VALID = (unsigned)0,

    /**
     * The issuer claim is invalid.
     */
    L8W8JWT_ISS_FAILURE = (unsigned)1 << (unsigned)0,

    /**
     * The subject claim is invalid.
     */
    L8W8JWT_SUB_FAILURE = (unsigned)1 << (unsigned)1,

    /**
     * The audience claim is invalid.
     */
    L8W8JWT_AUD_FAILURE = (unsigned)1 << (unsigned)2,

    /**
     * The JWT ID claim is invalid.
     */
    L8W8JWT_JTI_FAILURE = (unsigned)1 << (unsigned)3,

    /**
     * The token is expired.
     */
    L8W8JWT_EXP_FAILURE = (unsigned)1 << (unsigned)4,

    /**
     * The token is not yet valid.
     */
    L8W8JWT_NBF_FAILURE = (unsigned)1 << (unsigned)5,

    /**
     * The token was not issued yet, are you from the future?
     */
    L8W8JWT_IAT_FAILURE = (unsigned)1 << (unsigned)6,

    /**
     * The token was potentially tampered with: its signature couldn't be verified.
     */
    L8W8JWT_SIGNATURE_VERIFICATION_FAILURE = (unsigned)1 << (unsigned)7,

    /**
     * The token's "typ" claim validation failed.
     */
    L8W8JWT_TYP_FAILURE = (unsigned)1 << (unsigned)8
};

static void l8w8jwt_get_validation_result_desc(enum l8w8jwt_validation_result res, char* out_buffer, const size_t buffer_size)
{
#define JWT_OUTPUT_MSG(msg) snprintf(out_buffer, buffer_size, "%s", msg)
#define JWT_FLAG_STATUS(flag, msg) if(res & flag) { JWT_OUTPUT_MSG(msg); return; }

    if (res == L8W8JWT_VALID)
    {
        JWT_OUTPUT_MSG("Success");
        return;
    }

    JWT_FLAG_STATUS(L8W8JWT_ISS_FAILURE, "Issuer claim is invalid");
    JWT_FLAG_STATUS(L8W8JWT_SUB_FAILURE, "Subject claim is invalid");
    JWT_FLAG_STATUS(L8W8JWT_AUD_FAILURE, "Audience claim is invalid");
    JWT_FLAG_STATUS(L8W8JWT_JTI_FAILURE, "JTI claim is invalid");
    JWT_FLAG_STATUS(L8W8JWT_EXP_FAILURE, "Token has expired");
    JWT_FLAG_STATUS(L8W8JWT_NBF_FAILURE, "Token is not yet valid");
    JWT_FLAG_STATUS(L8W8JWT_IAT_FAILURE, "Token has not been issued yet");
    JWT_FLAG_STATUS(L8W8JWT_SIGNATURE_VERIFICATION_FAILURE, "Token signature is invalid");
    JWT_FLAG_STATUS(L8W8JWT_TYP_FAILURE, "Token type is invalid");

#undef JWT_OUTPUT_MSG
#undef JWT_FLAG_STATUS
}

/**
 * Struct containing the parameters to use for decoding and validating a JWT.
 */
struct l8w8jwt_decoding_params
{
    /**
     * The token to decode and validate.
     */
    char* jwt;

    /**
     * The jwt string length.
     */
    size_t jwt_length;

    /**
     * The signature algorithm ID. <p>
     * [0;2] = HS256/384/512 | [3;5] = RS256/384/512 | [6;8] = PS256/384/512 | [9;11] = ES256/384/512 <p>
     * This affects what should be the value of {@link #verification_key}
     */
    int alg;

    /**
     * [OPTIONAL] The issuer claim (who issued the JWT?). <p>
     * Set to <code>NULL</code> if you don't want to validate the issuer. <p>
     * The JWT will only pass verification if its <code>iss</code> claim matches this string.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.1
     */
    char* validate_iss;

    /**
     * validate_iss string length.
     */
    size_t validate_iss_length;

    /**
     * [OPTIONAL] The subject claim (who is the JWT about?). <p>
     * Set to <code>NULL</code> if you don't want to validate the subject claim. <p>
     * The JWT will only pass verification if its <code>sub</code> matches this string.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.2
     */
    char* validate_sub;

    /**
     * validate_sub string length.
     */
    size_t validate_sub_length;

    /**
     * [OPTIONAL] The audience claim (who is the JWT intended for? Who is the intended JWT's recipient?). <p>
     * Set to <code>NULL</code> if you don't want to validate the audience. <p>
     * The JWT will only pass verification if its <code>aud</code> matches this string.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.3
     */
    char* validate_aud;

    /**
     * validate_aud string length.
     */
    size_t validate_aud_length;

    /**
     * [OPTIONAL] The JWT ID. Provides a unique identifier for the token. <p>
     * Set to <code>NULL</code> if you don't want to validate the jti claim. <p>
     * The JWT will only pass verification if its <code>jti</code> matches this string.
     * @see https://tools.ietf.org/html/rfc7519#section-4.1.7
     */
    char* validate_jti;

    /**
     * validate_jti claim length.
     */
    size_t validate_jti_length;

    /**
     * Should the expiration claim be verified?
     * If this is set to <code>1</code>, the <code>exp</code> claim will be compared to the current date and time + {@link #exp_tolerance_seconds}
     */
    int validate_exp;

    /**
     * Should the "not before" claim be verified?
     * If this is set to <code>1</code>, the <code>nbf</code> claim will be compared to the current date and time + {@link #nbf_tolerance_seconds}
     */
    int validate_nbf;

    /**
     * Should the "issued at" claim be verified?
     * If this is set to <code>1</code>, the <code>iat</code> claim will be compared to the current date and time + {@link #iat_tolerance_seconds}
     */
    int validate_iat;

    /**
     * Small inconsistencies in time can happen, or also latency between clients and servers.
     * That's just life. You can forgive a few seconds of expiration, but don't exaggerate this! <p>
     * Only taken into consideration if {@link #validate_exp} is set to <code>1</code>.
     */
    uint8_t exp_tolerance_seconds;

    /**
     * The amount of seconds to subtract from the current time when comparing the "not before" claim, to allow for a small tolerance time frame.
     * Only taken into consideration if {@link #validate_nbf} is set to <code>1</code>.
     */
    uint8_t nbf_tolerance_seconds;

    /**
     * The amount of seconds to subtract from the current time when comparing the "issued at" claim, to allow for a small tolerance time frame.
     * Only taken into consideration if {@link #validate_iat} is set to <code>1</code>.
     */
    uint8_t iat_tolerance_seconds;

    /**
     * The key to use for verifying the token's signature
     * (e.g. if you chose HS256 as algorithm, this will be the HMAC secret; for RS512 this will be the PEM-formatted public RSA key string, etc...).
     */
    unsigned char* verification_key;

    /**
     * Length of the {@link #verification_key}
     */
    size_t verification_key_length;

    /**
     * [OPTIONAL] The typ claim (what type is the token?). <p>
     * Set to <code>NULL</code> if you don't want to validate the "typ" claim. <p>
     */
    char* validate_typ;

    /**
     * validate_typ string length.
     */
    size_t validate_typ_length;
};

/**
 * Initializes a {@link #l8w8jwt_decoding_params} instance by setting its fields to default values.
 * @param params The l8w8jwt_decoding_params to initialize (set to default values).
 */
L8W8JWT_API void l8w8jwt_decoding_params_init(struct l8w8jwt_decoding_params* params);

/**
 * Validates a set of l8w8jwt_decoding_params.
 * @param params The l8w8jwt_decoding_params to validate.
 * @return Return code as defined in retcodes.h
 */
L8W8JWT_API int l8w8jwt_validate_decoding_params(struct l8w8jwt_decoding_params* params);

/**
 * Decode (and validate) a JWT using specific parameters. <p>
 * The resulting {@link #l8w8jwt_validation_result} written into the passed "out_validation_result" pointer
 * contains validation failure flags (see the {@link #l8w8jwt_validation_result} enum docs for more details). <p>
 *
 * This only happens if decoding also succeeded: if the token is malformed, nothing will be written into "out_validation_result". <p>
 *
 * If validation succeeds, the {@link #l8w8jwt_validation_result} receives the value 0 (enum value <code>L8W8JWT_VALID</code>). <p>
 *
 * The same applies to the "out_claims" argument: it is only allocated and written to if it (obviously) isn't <code>NULL</code> and if the decoding was also successful!
 *
 * @param params The parameters to use for decoding and validating the token.
 *
 * @param out_validation_result Where to write the validation result flags into (0 means success). In case of a decoding failure this is set to -1 (or <code>~L8W8JWT_VALID</code>)!
 *
 * @param out_claims
 * [OPTIONAL] Where the decoded claims (header + payload claims together) should be written into.
 * This pointer will be dereferenced + allocated, so make sure to pass a fresh pointer!
 * If you don't need the claims, set this to <code>NULL</code> (they will only be validated, e.g. signature, exp, etc...).
 * Check the note down below for more infos!
 *
 * @param out_claims_length Where to write the decoded claims count into. This will receive the value of how many claims were written into "out_claims" (0 if you decided to set "out_claims" to <code>NULL</code>).
 *
 * @note If you decide to keep the claims stored in the <code>out_claims</code> parameter, REMEMBER to call {@link #l8w8jwt_free_claims()} on it once you're done using them!
 *
 * @return Return code as defined in retcodes.h (this is NOT the validation result that's written into the out_validation_result argument; the returned int describes whether the actual parsing/decoding part failed).
 */
L8W8JWT_API int l8w8jwt_decode(struct l8w8jwt_decoding_params* params, enum l8w8jwt_validation_result* out_validation_result, struct l8w8jwt_claim** out_claims, size_t* out_claims_length);

/**
 * Decode (and validate) a JWT using specific parameters,
 * but instead of writing the collection of claims contained in the payload into an array of <code>l8w8jwt_validation_result</code>
 * like in the standard {@link l8w8jwt_decode} function,
 * the raw JWT payload's JSON string is written into the {@link #out_payload_json} string, such that it can be parsed externally. <p>
 *
 * The resulting {@link #l8w8jwt_validation_result} written into the passed "out_validation_result" pointer
 * contains validation failure flags (see the {@link #l8w8jwt_validation_result} enum docs for more details). <p>
 *
 * This only happens if decoding also succeeded: if the token is malformed, nothing will be written into "out_validation_result". <p>
 *
 * If validation succeeds, the {@link #l8w8jwt_validation_result} receives the value 0 (enum value <code>L8W8JWT_VALID</code>). <p>
 *
 * The same applies to the "out_payload_json" argument:
 * it is only allocated and written to if it (obviously) isn't <code>NULL</code> and if the decoding procedure was also successful!
 *
 * @param params The parameters to use for decoding and validating the token.
 *
 * @param out_validation_result Where to write the validation result flags into (0 means success). In case of a decoding failure this is set to -1 (or <code>~L8W8JWT_VALID</code>)!
 *
 * @param out_header Where to write the decoded JWT header JSON string into.
 *
 * @param out_header_length Where to write the length of {@link out_header} into.
 *
 * @param out_payload Where to write the decoded JWT's payload JSON string into, so that it can be parsed externally instead of by l8w8jwt directly.
 *
 * @param out_payload_length Where to write the length of {@link out_payload} into.
 *
 * @param out_signature Where to write the JWT's signature into.
 *
 * @param out_signature_length Where to write the length of {@link out_signature} into.
 *
 * @return Return code as defined in retcodes.h (this is NOT the validation result that's written into the {@link out_validation_result} argument; the returned int describes whether the actual parsing/decoding part failed).
 */
L8W8JWT_API int l8w8jwt_decode_raw(struct l8w8jwt_decoding_params* params, enum l8w8jwt_validation_result* out_validation_result, char** out_header, size_t* out_header_length, char** out_payload, size_t* out_payload_length, uint8_t** out_signature, size_t* out_signature_length);

/**
 * Decodes a JWT without validating anything: neither claims nor signature. Just raw decoding, no validation!
 *
 * @param params The parameters to use for decoding and validating the token.
 *
 * @param out_header Where to write the decoded JWT header JSON string into.
 *
 * @param out_header_length Where to write the length of {@link out_header} into.
 *
 * @param out_payload Where to write the decoded JWT's payload JSON string into, so that it can be parsed externally instead of by l8w8jwt directly.
 *
 * @param out_payload_length Where to write the length of {@link out_payload} into.
 *
 * @param out_signature Where to write the JWT's signature into.
 *
 * @param out_signature_length Where to write the length of {@link out_signature} into.
 *
 * @return Return code as defined in retcodes.h
 */
L8W8JWT_API int l8w8jwt_decode_raw_no_validation(struct l8w8jwt_decoding_params* params, char** out_header, size_t* out_header_length, char** out_payload, size_t* out_payload_length, uint8_t** out_signature, size_t* out_signature_length);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // L8W8JWT_DECODE_H
