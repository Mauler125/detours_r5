cmake_minimum_required( VERSION 3.16 )
add_module( "exe" "sdklauncher" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Foundation"
    "${ENGINE_SOURCE_DIR}/tier0/plat_time.cpp"
    "${ENGINE_SOURCE_DIR}/public/tier0/platform.h"
)

add_sources( SOURCE_GROUP "Core"
    "${ENGINE_SOURCE_DIR}/core/logger.cpp"
    "${ENGINE_SOURCE_DIR}/core/logger.h"
    "${ENGINE_SOURCE_DIR}/core/logdef.cpp"
    "${ENGINE_SOURCE_DIR}/core/logdef.h"
    "${ENGINE_SOURCE_DIR}/core/termutil.cpp"
    "${ENGINE_SOURCE_DIR}/core/termutil.h"
)

add_sources( SOURCE_GROUP "Windows"
    "${ENGINE_SOURCE_DIR}/windows/console.cpp"
    "${ENGINE_SOURCE_DIR}/windows/console.h"
)

add_sources( SOURCE_GROUP "App"
    "sdklauncher.cpp"
    "sdklauncher.h"
    "sdklauncher_const.h"
    "sdklauncher_utils.cpp"
    "sdklauncher_utils.h"
)

add_sources( SOURCE_GROUP "GUI"
    "advanced_surface.cpp"
    "advanced_surface.h"
    "base_surface.cpp"
    "base_surface.h"
    "download_surface.cpp"
    "download_surface.h"
)

add_sources( SOURCE_GROUP "Resource"
    "sdklauncher_res.h"
    "${ENGINE_SOURCE_DIR}/resource/sdklauncher.rc"
)

add_sources( SOURCE_GROUP "Windows"
    "${ENGINE_SOURCE_DIR}/windows/window.cpp"
    "${ENGINE_SOURCE_DIR}/windows/window.h"
)

end_sources()

set_target_properties( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME
    "launcher"
)
set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_COMMAND "launcher.exe"
    VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir)../../../${BUILD_OUTPUT_DIR}/"
)
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "_TOOLS"
    "SDKLAUNCHER"
)
target_precompile_headers( ${PROJECT_NAME} PRIVATE
    "sdklauncher_pch.h"
)
target_link_options( ${PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS" )
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "tier1"
    "tier2"

    "filesystem_std"
    "vstdlib"

    "libdetours"
    "libcppkore"
    "libspdlog"
    "libcurl"
    
    "liblzma"
    "libzlib"
    "libbzip2"
    "libzip"

    "crypt32.lib"
    "ws2_32.lib"
    "wldap32.lib"
    "Rpcrt4.lib"
)

add_module( "exe" "sdkupdater" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "sdkupdater.cpp"
    "sdkupdater.h"
)

end_sources( "${BUILD_OUTPUT_DIR}/bin/" )

set_target_properties( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME
    "updater"
)

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "tier1"
)