cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "libzip" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "compression"
	"src/compression/compression_interface.h"
)

add_sources( SOURCE_GROUP "Compression/Bzip2"
	"src/compression/bzip2/bzip2_decoder.h"
	"src/compression/bzip2/bzip2_decoder_properties.h"
	"src/compression/bzip2/bzip2_encoder.h"
	"src/compression/bzip2/bzip2_encoder_properties.h"
)

add_sources( SOURCE_GROUP "Compression/Deflate"
	"src/compression/deflate/deflate_decoder.h"
	"src/compression/deflate/deflate_decoder_properties.h"
	"src/compression/deflate/deflate_encoder.h"
	"src/compression/deflate/deflate_encoder_properties.h"
)

add_sources( SOURCE_GROUP "Compression/LZMA/Detail"
	"src/compression/lzma/detail/lzma_alloc.h"
	"src/compression/lzma/detail/lzma_handle.h"
	"src/compression/lzma/detail/lzma_header.h"
	"src/compression/lzma/detail/lzma_in_stream.h"
	"src/compression/lzma/detail/lzma_out_stream.h"
)
add_sources( SOURCE_GROUP "Compression/LZMA"
	"src/compression/lzma/lzma_decoder.h"
	"src/compression/lzma/lzma_decoder_properties.h"
	"src/compression/lzma/lzma_encoder.h"
	"src/compression/lzma/lzma_encoder_properties.h"
)
add_sources( SOURCE_GROUP "Compression/Store"
	"src/compression/store/store_decoder.h"
	"src/compression/store/store_decoder_properties.h"
	"src/compression/store/store_encoder.h"
	"src/compression/store/store_encoder_properties.h"
)
add_sources( SOURCE_GROUP "Detail"
	"src/detail/EndOfCentralDirectoryBlock.cpp"
	"src/detail/EndOfCentralDirectoryBlock.h"
	"src/detail/ZipCentralDirectoryFileHeader.cpp"
	"src/detail/ZipCentralDirectoryFileHeader.h"
	"src/detail/ZipGenericExtraField.cpp"
	"src/detail/ZipGenericExtraField.h"
	"src/detail/ZipLocalFileHeader.cpp"
	"src/detail/ZipLocalFileHeader.h"
)
add_sources( SOURCE_GROUP "Methods"
	"src/methods/Bzip2Method.h"
	"src/methods/DeflateMethod.h"
	"src/methods/ICompressionMethod.h"
	"src/methods/LzmaMethod.h"
	"src/methods/StoreMethod.h"
	"src/methods/ZipMethodResolver.h"
)
add_sources( SOURCE_GROUP "Streams"
	"src/streams/compression_decoder_stream.h"
	"src/streams/compression_encoder_stream.h"
	"src/streams/crc32stream.h"
	"src/streams/memstream.h"
	"src/streams/nullstream.h"
	"src/streams/serialization.h"
	"src/streams/substream.h"
	"src/streams/teestream.h"
	"src/streams/zip_cryptostream.h"
)
add_sources( SOURCE_GROUP "Utils"
	"src/utils/enum_utils.h"
	"src/utils/stream_utils.h"
	"src/utils/time_utils.h"
)

add_sources( SOURCE_GROUP "Utils"
	"src/ZipArchive.cpp"
	"src/ZipArchive.h"
	"src/ZipArchiveEntry.cpp"
	"src/ZipArchiveEntry.h"
	"src/ZipFile.cpp"
	"src/ZipFile.h"
)

end_sources()

whole_program_optimization()
thirdparty_suppress_warnings()

target_compile_definitions( ${PROJECT_NAME} PRIVATE
	"WIN32"
)
