cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "libjwt" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
	"base64.c"
	"claim.c"
	"decode.c"
	"encode.c"
	"util.c"
	"version.c"
)

add_sources( SOURCE_GROUP "Include"
	"include/algs.h"
	"include/base64.h"
	"include/claim.h"
	"include/decode.h"
	"include/encode.h"
	"include/retcodes.h"
	"include/timehelper.h"
	"include/util.h"
	"include/version.h"
)

end_sources()
thirdparty_suppress_warnings()

target_include_directories( ${PROJECT_NAME} PRIVATE
    "${THIRDPARTY_SOURCE_DIR}/mbedtls/include/"
)
