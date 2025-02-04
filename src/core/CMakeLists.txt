cmake_minimum_required( VERSION 3.16 )

macro( add_sdk_project PROJECT_NAME )
add_module( "shared_lib" ${PROJECT_NAME} "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

if( NOT ${PROJECT_NAME} STREQUAL "dedicated" )
file( GLOB PNG_SOURCES
    "${ENGINE_SOURCE_DIR}/resource/png/*.png"
)
add_sources( SOURCE_GROUP "Resource"
    "${ENGINE_SOURCE_DIR}/resource/r5dev.rc"
    "${PNG_SOURCES}"
)
endif()

add_sources( SOURCE_GROUP "Core"
    "assert.h"
    "dllmain.cpp"
    "init.cpp"
    "init.h"
    "logdef.cpp"
    "logdef.h"
    "logger.cpp"
    "logger.h"
    "r5dev.h"
    "resource.h"
    "shared_pch.h"
    "stdafx.cpp"
    "stdafx.h"
    "termutil.cpp"
    "termutil.h"
)

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "advapi32.lib"
    "bcrypt.lib"
    "crypt32.lib"
    "dbghelp.lib"
    "wldap32.lib"
    "ws2_32.lib"
    "Rpcrt4.lib"
    "iphlpapi.lib"
    "Winmm.lib"

    "vpc"
    "memoverride"
    "tier0"
    "tier1"
    "tier2"
    "launcher"
    "appframework"

    "vstdlib"
    "vpklib"
    "mathlib"
    "vphysics"

    "SigCache_Pb"
    "LiveAPI_Pb"
    "NetCon_Pb"

    "rson"
    "rtech_game"
    "playlists"
    "stryder"

    "liblzham"
    "libzstd"
    "liblz4"

    "libdetours"
    "libcurl"
    "libprotobuf"
    "libspdlog"
    "libdetour"
    "navdebugutils"
    "navsharedcommon"

    "EAThread"
    "DirtySDK"

    "libmbedcrypto"
    "libmbedtls"
    "libmbedx509"

    "networksystem"
    "pluginsystem"
    "filesystem"
    "datacache"
    "EbisuSDK"
    "GFSDK"
    "RDSDK"

    "localize"

    "vscript"
    "game_shared_static"
)

if( NOT ${PROJECT_NAME} STREQUAL "dedicated" )
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "libimgui"
    "codecs"

    "inputsystem"
    "materialsystem"

    "studiorender"

    "vguimatsurface"
    "vgui"
    "rui"

    "particles"

    "d3d11.lib"
    "${THIRDPARTY_SOURCE_DIR}/nvapi/amd64/nvapi64.lib"
)
endif()

if( NOT ${PROJECT_NAME} STREQUAL "client" )
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "libjwt"
)
endif()

# Determine the compiler definitions and link libraries per project.
if( ${PROJECT_NAME} STREQUAL "gamesdk" )
end_sources()

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "engine"
    "server_static"
    "client_static"
)
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "GAMESDK"
)

elseif( ${PROJECT_NAME} STREQUAL "dedicated" )
end_sources()

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "materialsystem_nodx" # Needs the No-DirectX version for patching.
    "engine_ds"
    "server_static"
)
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "MATERIALSYSTEM_NODX"
    "SERVER_DLL"
    "DEDICATED"
)

elseif( ${PROJECT_NAME} STREQUAL "client" )
end_sources( "${BUILD_OUTPUT_DIR}/bin/x64_retail/" )

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "engine_cl"
    "client_static"
)
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "CLIENT_DLL"
)

endif()

target_include_directories( ${PROJECT_NAME} PRIVATE
	"${THIRDPARTY_SOURCE_DIR}/dirtysdk/include/"
	"${THIRDPARTY_SOURCE_DIR}/ea/"
)

target_link_options( ${PROJECT_NAME} PRIVATE
    "/STACK:8000000" # Match game executable stack reserve size

    # The only reason this has to be done, is because
    # the _xxxx_dbg malloc functions could otherwise
    # throw a linker error, as they are already defined
    # in the CRT library, but there doesn't seem to be
    # a standard way to override these, unlike the
    # _xxxx_base malloc functions. Since this is only
    # relevant for debug builds, this linker flag should
    # only be toggled for DEBUG!!!
    $<$<CONFIG:Debug>:/FORCE:MULTIPLE>
)

add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -P ${ENGINE_SOURCE_DIR}/cmake/PostBuild.cmake
)

if( ${PROJECT_NAME} STREQUAL "dedicated" )
set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_COMMAND "r5apex_ds.exe"
)
else()
set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_COMMAND "r5apex.exe"
)
endif()

if( ${PROJECT_NAME} STREQUAL "client" )
set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_COMMAND_ARGUMENTS "-wconsole -ansicolor -dev -devsdk -noserverdll"
)
else()
set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_COMMAND_ARGUMENTS "-wconsole -ansicolor -dev -devsdk"
    VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir)../../../${BUILD_OUTPUT_DIR}/"
)
endif()

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir)../../../${BUILD_OUTPUT_DIR}/"
)

endmacro()

add_sdk_project( "gamesdk" )
add_sdk_project( "dedicated" )
add_sdk_project( "client" )
