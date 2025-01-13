#pragma once
#include "shared_pch.h"
#if !defined(DEDICATED) && !defined(PLUGINSDK)
#include <d3d11.h>
#endif // !DEDICATED && !PLUGINSDK

// Must be included before any third party lib!
// this header replaces the standard new/delete
// operators with our own, along with the standard
// malloc/free functions.
#include "tier0/memstd.h"

// Thirdparty includes.
#include "thirdparty/detours/include/detours.h"
#include "thirdparty/detours/include/idetour.h"

#include "thirdparty/lzham/include/lzham_assert.h"
#include "thirdparty/lzham/include/lzham_types.h"
#include "thirdparty/lzham/include/lzham.h"

#include "thirdparty/zstd/zstd.h"
#include "thirdparty/zstd/decompress/zstd_decompress_internal.h"

#include "thirdparty/lz4/lz4.h"

#include "thirdparty/curl/include/curl/curl.h"

// RapidJSON uses 32 bit size types. Size types are
// 64 bit wide on our target. Override it with ours.
// this must be done before the rapidjson.h include.
#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson { typedef ::std::size_t SizeType; }

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

#if !defined(DEDICATED) && !defined(PLUGINSDK)
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/imgui/misc/imgui_logger.h"
#include "thirdparty/imgui/misc/imgui_editor.h"
#include "thirdparty/imgui/misc/imgui_utility.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imgui/backends/imgui_impl_dx11.h"
#include "thirdparty/imgui/backends/imgui_impl_win32.h"

#include "thirdparty/nvapi/pclstats.h"
#include "thirdparty/nvapi/nvapi.h"
#include "thirdparty/nvapi/nvapi_lite_common.h"

#include "thirdparty/fidelityfx/ffx_antilag2_dx11.h"
#endif // !DEDICATED && !PLUGINSDK


#pragma warning(push)
#pragma warning( disable : 4505 )
#include "thirdparty/spdlog/spdlog.h"
#include "thirdparty/spdlog/async.h"
#include "thirdparty/spdlog/sinks/ostream_sink.h"
#include "thirdparty/spdlog/sinks/basic_file_sink.h"
#include "thirdparty/spdlog/sinks/stdout_sinks.h"
#include "thirdparty/spdlog/sinks/stdout_color_sinks.h"
#include "thirdparty/spdlog/sinks/ansicolor_sink.h"
#include "thirdparty/spdlog/sinks/rotating_file_sink.h"
#pragma warning(pop)

// Tier0 includes.
#include "tier0/basetypes.h"
#include "tier0/wchartypes.h"
#include "tier0/memaddr.h"
#include "tier0/utility.h"
#include "tier0/module.h"
#include "tier0/platform.h"
#include "tier0/platwindow.h"
#include "tier0/annotations.h"
#include "tier0/commonmacros.h"
#include "tier0/memalloc.h"
#include "tier0/tier0_iface.h"
#include "tier0/dbg.h"

// Tier1 includes.
#include "tier1/tier1.h"
#include "tier1/cvar.h"
#include "tier1/cmd.h"
#include "common/global.h"
