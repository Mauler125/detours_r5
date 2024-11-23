add_module( "lib" "libbzip2" "" ${FOLDER_CONTEXT} TRUE TRUE )

add_sources( SOURCE_GROUP "Include"
    "bzlib.h"
    "bzlib_private.h"
)

add_sources( SOURCE_GROUP "Source"
    "blocksort.c"
    "bzerror.c"
    "bzlib.c"
    "compress.c"
    "crctable.c"
    "decompress.c"
    "huffman.c"
    "randtable.c"
)

end_sources()

whole_program_optimization()
thirdparty_suppress_warnings()
