set(PROJECT_NAME ZipLib)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "ZipArchive.h"
    "ZipArchiveEntry.h"
    "ZipFile.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Header_Files__compression
    "compression/compression_interface.h"
)
source_group("Header Files\\compression" FILES ${Header_Files__compression})

set(Header_Files__compression__bzip2
    "compression/bzip2/bzip2_decoder.h"
    "compression/bzip2/bzip2_decoder_properties.h"
    "compression/bzip2/bzip2_encoder.h"
    "compression/bzip2/bzip2_encoder_properties.h"
)
source_group("Header Files\\compression\\bzip2" FILES ${Header_Files__compression__bzip2})

set(Header_Files__compression__deflate
    "compression/deflate/deflate_decoder.h"
    "compression/deflate/deflate_decoder_properties.h"
    "compression/deflate/deflate_encoder.h"
    "compression/deflate/deflate_encoder_properties.h"
)
source_group("Header Files\\compression\\deflate" FILES ${Header_Files__compression__deflate})

set(Header_Files__compression__lzma
    "compression/lzma/lzma_decoder.h"
    "compression/lzma/lzma_decoder_properties.h"
    "compression/lzma/lzma_encoder.h"
    "compression/lzma/lzma_encoder_properties.h"
)
source_group("Header Files\\compression\\lzma" FILES ${Header_Files__compression__lzma})

set(Header_Files__compression__lzma__detail
    "compression/lzma/detail/lzma_alloc.h"
    "compression/lzma/detail/lzma_handle.h"
    "compression/lzma/detail/lzma_header.h"
    "compression/lzma/detail/lzma_in_stream.h"
    "compression/lzma/detail/lzma_out_stream.h"
)
source_group("Header Files\\compression\\lzma\\detail" FILES ${Header_Files__compression__lzma__detail})

set(Header_Files__compression__store
    "compression/store/store_decoder.h"
    "compression/store/store_decoder_properties.h"
    "compression/store/store_encoder.h"
    "compression/store/store_encoder_properties.h"
)
source_group("Header Files\\compression\\store" FILES ${Header_Files__compression__store})

set(Header_Files__detail
    "detail/EndOfCentralDirectoryBlock.h"
    "detail/ZipCentralDirectoryFileHeader.h"
    "detail/ZipGenericExtraField.h"
    "detail/ZipLocalFileHeader.h"
)
source_group("Header Files\\detail" FILES ${Header_Files__detail})

set(Header_Files__methods
    "methods/Bzip2Method.h"
    "methods/DeflateMethod.h"
    "methods/ICompressionMethod.h"
    "methods/LzmaMethod.h"
    "methods/StoreMethod.h"
    "methods/ZipMethodResolver.h"
)
source_group("Header Files\\methods" FILES ${Header_Files__methods})

set(Header_Files__streams
    "streams/compression_decoder_stream.h"
    "streams/compression_encoder_stream.h"
    "streams/crc32stream.h"
    "streams/memstream.h"
    "streams/nullstream.h"
    "streams/serialization.h"
    "streams/substream.h"
    "streams/teestream.h"
    "streams/zip_cryptostream.h"
)
source_group("Header Files\\streams" FILES ${Header_Files__streams})

set(Header_Files__streams__streambuffs
    "streams/streambuffs/compression_decoder_streambuf.h"
    "streams/streambuffs/compression_encoder_streambuf.h"
    "streams/streambuffs/crc32_streambuf.h"
    "streams/streambuffs/mem_streambuf.h"
    "streams/streambuffs/null_streambuf.h"
    "streams/streambuffs/sub_streambuf.h"
    "streams/streambuffs/tee_streambuff.h"
    "streams/streambuffs/zip_crypto_streambuf.h"
)
source_group("Header Files\\streams\\streambuffs" FILES ${Header_Files__streams__streambuffs})

set(Header_Files__utils
    "utils/enum_utils.h"
    "utils/stream_utils.h"
    "utils/time_utils.h"
)
source_group("Header Files\\utils" FILES ${Header_Files__utils})

set(Source_Files
    "ZipArchive.cpp"
    "ZipArchiveEntry.cpp"
    "ZipFile.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(Source_Files__detail
    "detail/EndOfCentralDirectoryBlock.cpp"
    "detail/ZipCentralDirectoryFileHeader.cpp"
    "detail/ZipGenericExtraField.cpp"
    "detail/ZipLocalFileHeader.cpp"
)
source_group("Source Files\\detail" FILES ${Source_Files__detail})

set(ALL_FILES
    ${Header_Files}
    ${Header_Files__compression}
    ${Header_Files__compression__bzip2}
    ${Header_Files__compression__deflate}
    ${Header_Files__compression__lzma}
    ${Header_Files__compression__lzma__detail}
    ${Header_Files__compression__store}
    ${Header_Files__detail}
    ${Header_Files__methods}
    ${Header_Files__streams}
    ${Header_Files__streams__streambuffs}
    ${Header_Files__utils}
    ${Source_Files}
    ${Source_Files__detail}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE ZipLib)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
################################################################################
# Output directory
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/Bin/$ENV{PlatformShortName}/$<CONFIG>/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Bin/$ENV{PlatformShortName}/$<CONFIG>/"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/Bin/$ENV{PlatformShortName}/$<CONFIG>/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Bin/$ENV{PlatformShortName}/$<CONFIG>/"
    )
endif()
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
endif()
################################################################################
# Compile definitions
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "WIN32;"
        "_LIB;"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "WIN32;"
        "_LIB;"
        "UNICODE;"
        "_UNICODE"
    )
endif()

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /Od
            >
            $<$<CONFIG:Release>:
                /O2;
                /Oi;
                /Gy
            >
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /wd4996;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Y-
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /Od
            >
            $<$<CONFIG:Release>:
                /O2;
                /Oi;
                /Gy
            >
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /wd4996;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Y-
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF
            >
            /SUBSYSTEM:WINDOWS
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF
            >
            /SUBSYSTEM:WINDOWS
        )
    endif()
endif()

################################################################################
# Dependencies
################################################################################
# Link with other targets.
target_link_libraries(${PROJECT_NAME} PUBLIC
    bzip2
    lzma
    zlib
)

