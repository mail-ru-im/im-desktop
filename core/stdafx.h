#pragma once

#ifdef _WIN32
#pragma warning( disable : 35038)
#include <WinSDKVer.h>
#define WINVER 0x0501
#define _WIN32_WINDOWS 0x0501
#define _WIN32_WINNT 0x0600

#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_LEAN_AND_MEAN
#define _ATL_XP_TARGETING

#include "win32/targetver.h"
#include <boost/asio.hpp>
#include <Windows.h>
#include <Shlobj.h>
#include <atlbase.h>
#include <atlstr.h>
#include <Psapi.h>
#pragma warning( error : 35038)
#endif //WIN32

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED

#include <algorithm>
#include <atomic>
#include <cassert>
#include <ctime>
#include <codecvt>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iterator>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <optional>
#include <type_traits>
#include <assert.h>

#ifndef _WIN32
#include <boost/asio.hpp>
#endif //_WIN32
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/locale.hpp>
#include <boost/locale/generator.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/config.hpp>
#include <boost/thread/thread.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/thread.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/stacktrace.hpp>

#include "product.h"
#include "../common.shared/common.h"
#include "../common.shared/typedefs.h"

#include "tools/tlv.h"
#include "tools/binary_stream.h"
#include "tools/binary_stream_reader.h"
#include "tools/scope.h"
#include "tools/strings.h"

#undef min
#undef max
#undef small

#ifndef _WIN32
#define assert(e) { if (!(e)) puts("ASSERT: " #e); }
#define RAPIDJSON_ASSERT(e) assert(e)
#endif // _WIN32

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "../common.shared/constants.h"

using rapidjson_allocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

#ifndef _RAPIDJSON_GET_STRING_
#define _RAPIDJSON_GET_STRING_
namespace core
{
    template <typename T>
    inline std::string rapidjson_get_string(const T& value) { return std::string(value.GetString(), value.GetStringLength()); }

    template <>
    inline std::string rapidjson_get_string<rapidjson::StringBuffer>(const rapidjson::StringBuffer& value) { return std::string(value.GetString(), value.GetSize()); }

    template <typename T>
    inline std::string_view rapidjson_get_string_view(const T& value) { return std::string_view(value.GetString(), value.GetStringLength()); }

    template <>
    inline std::string_view rapidjson_get_string_view<rapidjson::StringBuffer>(const rapidjson::StringBuffer& value) { return std::string_view(value.GetString(), value.GetSize()); }
}
#endif // _RAPIDJSON_GET_STRING_


#ifdef __APPLE__
#   if defined(DEBUG) || defined(_DEBUG)
//#       define DEBUG__OUTPUT_NET_PACKETS
#   endif // defined(DEBUG) || defined(_DEBUG)
#endif // __APPLE__

namespace core
{
    using milliseconds_t = long;

    using hash_t = size_t;

    using priority_t = int; // the lower number is the higher priority

    constexpr inline priority_t increase_priority() noexcept { return -5; }
    constexpr inline priority_t decrease_priority() noexcept { return 5; }

    constexpr inline priority_t priority_fetch() noexcept { return 0; }
    constexpr inline priority_t priority_send_message() noexcept { return 1; }
    constexpr inline priority_t priority_protocol() noexcept { return 2; }

    constexpr inline priority_t top_priority() noexcept { return 3; }
    constexpr inline priority_t highest_priority() noexcept { return 50; }
    constexpr inline priority_t high_priority() noexcept { return 100; }
    constexpr inline priority_t default_priority() noexcept { return 150; }
    constexpr inline priority_t low_priority() noexcept { return 200; }
    constexpr inline priority_t lowest_priority() noexcept { return 250; }
}
