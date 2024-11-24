cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "vgui" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "vgui_baseui_interface.cpp"
    "vgui_baseui_interface.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/vgui/vgui.h"
    "${ENGINE_SOURCE_DIR}/public/vgui/ienginevgui.h"
)

add_sources( SOURCE_GROUP "Debug"
    "vgui_debugpanel.cpp"
    "vgui_debugpanel.h"
    "vgui_fpspanel.cpp"
    "vgui_fpspanel.h"
)

add_sources( SOURCE_GROUP "Controls"
    "vgui_controls/RichText.cpp"
    "vgui_controls/RichText.h"
)

end_sources()
