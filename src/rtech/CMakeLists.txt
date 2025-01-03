cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "rtech_game" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Async"
   "async/asyncio.cpp"
   "async/asyncio.h"
)

add_sources( SOURCE_GROUP "Pak"
   "pak/pakparse.cpp"
   "pak/pakparse.h"

   "pak/pakalloc.cpp"
   "pak/pakalloc.h"

   "pak/pakencode.cpp"
   "pak/pakencode.h"

   "pak/pakdecode.cpp"
   "pak/pakdecode.h"

   "pak/pakpatch.cpp"
   "pak/pakpatch.h"

   "pak/pakstate.cpp"
   "pak/pakstate.h"

   "pak/pakstream.cpp"
   "pak/pakstream.h"

   "pak/paktools.cpp"
   "pak/paktools.h"
)

add_sources( SOURCE_GROUP "LiveAPI"
   "liveapi/liveapi.cpp"
   "liveapi/liveapi.h"
)

add_sources( SOURCE_GROUP "Public"
   "${ENGINE_SOURCE_DIR}/public/rtech/iasync.h"
   "${ENGINE_SOURCE_DIR}/public/rtech/ipakfile.h"
   "${ENGINE_SOURCE_DIR}/public/rtech/istreamdb.h"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE
    "${THIRDPARTY_SOURCE_DIR}/dirtysdk/include/"
    "${THIRDPARTY_SOURCE_DIR}/ea/"
)

add_module( "lib" "rson" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "RSON"
    "rdf/rson.cpp"
)

add_sources( SOURCE_GROUP "Public"
   "${ENGINE_SOURCE_DIR}/public/rtech/rson.h"
)

end_sources()

add_module( "lib" "playlists" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
   "playlists/playlists.cpp"
   "playlists/playlists.h"
)

end_sources()

add_module( "lib" "rui" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
   "rui/rui.cpp"
   "rui/rui.h"
)

end_sources()

add_module( "lib" "stryder" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
   "stryder/stryder.cpp"
   "stryder/stryder.h"
)

end_sources()
