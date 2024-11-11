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

#include "include/version.h"
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void l8w8jwt_free(void* mem)
{
    free(mem);
}

int l8w8jwt_get_version_number()
{
    return (int)L8W8JWT_VERSION;
}

void l8w8jwt_get_version_string(char out[32])
{
    const char version_string[] = L8W8JWT_VERSION_STR;
    const size_t version_string_length = sizeof(version_string) - 1;

    memcpy(out, version_string, version_string_length);
    out[version_string_length] = '\0';
}

#ifdef __cplusplus
} // extern "C"
#endif