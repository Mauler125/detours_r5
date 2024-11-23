cmake_minimum_required( VERSION 3.16 )

set( FOLDER_CONTEXT "Foundation" )
add_subdirectory( vpc ) # VPC and Tier0 must be the first as this creates the shared PCH!
add_subdirectory( tier0 )
add_subdirectory( tier1 )
add_subdirectory( tier2 )
add_subdirectory( launcher )
add_subdirectory( appframework  )

set( FOLDER_CONTEXT "Libraries" )
add_subdirectory( mathlib )
add_subdirectory( vpklib )
add_subdirectory( vstdlib )
add_subdirectory( vphysics )
add_subdirectory( ebisusdk )
add_subdirectory( codecs )
add_subdirectory( geforce )
add_subdirectory( radeon )

set( FOLDER_CONTEXT "Protocols" )
add_subdirectory( protoc )

set( FOLDER_CONTEXT "Respawn" )
add_subdirectory( rtech )

set( FOLDER_CONTEXT "Thirdparty/Debugging" )
add_subdirectory( thirdparty/detours )
add_subdirectory( thirdparty/spdlog )

set( FOLDER_CONTEXT "Thirdparty/Navigation" )
add_subdirectory( thirdparty/recast )

set( FOLDER_CONTEXT "Thirdparty/Compression" )
add_subdirectory( thirdparty/lzham )
add_subdirectory( thirdparty/fastlz )
add_subdirectory( thirdparty/bzip2 )
add_subdirectory( thirdparty/zlib )
add_subdirectory( thirdparty/zstd )
add_subdirectory( thirdparty/lz4 )
add_subdirectory( thirdparty/lzma )
add_subdirectory( thirdparty/zip )

set( FOLDER_CONTEXT "Thirdparty/Security" )
add_subdirectory( thirdparty/mbedtls )
add_subdirectory( thirdparty/jwt )

set( FOLDER_CONTEXT "Thirdparty/Multimedia" )
add_subdirectory( thirdparty/sdl )
add_subdirectory( thirdparty/imgui )
add_subdirectory( thirdparty/cppnet )

set( FOLDER_CONTEXT "Thirdparty/Networking" )
add_subdirectory( thirdparty/protobuf )
add_subdirectory( thirdparty/curl )
add_subdirectory( thirdparty/dirtysdk )

set( FOLDER_CONTEXT "Thirdparty/Threading" )
add_subdirectory( thirdparty/ea/EAThread )

set( FOLDER_CONTEXT "Tools" )
add_subdirectory( sdklauncher )
add_subdirectory( netconsole )
add_subdirectory( naveditor )
add_subdirectory( revpk )

set( FOLDER_CONTEXT "System" )
add_subdirectory( networksystem )
add_subdirectory( pluginsystem )
add_subdirectory( materialsystem )
add_subdirectory( inputsystem )
add_subdirectory( filesystem )
add_subdirectory( datacache )
add_subdirectory( studiorender )
add_subdirectory( localize )
add_subdirectory( engine )
add_subdirectory( vguimatsurface )
add_subdirectory( vgui )
add_subdirectory( loader )

set( FOLDER_CONTEXT "Plugins" )
add_subdirectory( pluginsdk )

set( FOLDER_CONTEXT "Game" )
add_subdirectory( vscript )
add_subdirectory( game )

set( FOLDER_CONTEXT "/" )
add_subdirectory( core )
