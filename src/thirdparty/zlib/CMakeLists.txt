add_module( "lib" "libzlib" "" ${FOLDER_CONTEXT} TRUE TRUE )

add_sources( SOURCE_GROUP "Include"
    "crc32.h"
    "deflate.h"
    "gzguts.h"
    "inffast.h"
    "inffixed.h"
    "inflate.h"
    "inftrees.h"
    "trees.h"
    "zconf.h"
    "zlib.h"
    "zutil.h"
)

add_sources( SOURCE_GROUP "Source"
    "adler32.c"
    "compress.c"
    "crc32.c"
    "deflate.c"
    "infback.c"
    "inffast.c"
    "inflate.c"
    "inftrees.c"
    "trees.c"
    "uncompr.c"
    "zutil.c"
)

end_sources()

whole_program_optimization()
thirdparty_suppress_warnings()
