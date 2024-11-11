#include "version.h"
#include <time.h>

/**
 * For now use the standard type for l8w8jwt_time_t. 
 * Support for adding alternate types could be added in the future, if needed.
*/
typedef time_t l8w8jwt_time_t;

/**
 * Define alternate implementation of system `time` API
*/
#if L8W8JWT_PLATFORM_TIME_ALT
extern l8w8jwt_time_t (*l8w8jwt_time)(l8w8jwt_time_t *time);
#else
#define l8w8jwt_time   time
#endif
