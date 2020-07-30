#pragma once

#if !defined(InOut)
#define InOut
#endif

#if !defined(Out)
#define Out
#endif

#if !defined(UNUSED_ARG)
#define UNUSED_ARG(arg) ((void)arg)
#endif

// ----------------------------------------------
// fix for msvc++/clang compartibility

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#if defined(DEBUG) && !defined(_DEBUG)
#define _DEBUG
#endif

// ----------------------------------------------

#if !defined(__FUNCTION__)
#define __FUNCTION__ ""
#endif

#define __DISABLE(x) {}

#define __STRA(x) _STRA(x)
#define _STRA(x) #x

#define __STRW(x) _STRW(x)
#define _STRW(x) L#x

#define __LINEA__ __STRA(__LINE__)
#define __LINEW__ __STRW(__LINE__)

#define __FUNCLINEA__ __FUNCTION__ "(#" __LINEA__ ")"
#define __FUNCLINEW__ __FUNCTIONW__ L"(#" __LINEW__ L")"

#define __FILELINEA__ __FILE__ "(" __LINEA__ ")"
#define __FILELINEW__ __FILEW__ L"(" __LINEW__ L")"

#define __TODOA__ __FILELINEA__ ": "
#define __TODOW__ __FILELINEW__ L": "

#ifdef __APPLE__
#   undef __TODOA__
#   define __TODOA__ ""
#   undef __TODOW__
#   define __TODOW__ ""
#   undef __FILELINEA__
#   define __FILELINEA__ ""
#   undef __FILELINEW__
#   define __FILELINEW__ ""
#   undef __FUNCLINEA__
#   define __FUNCLINEA__ ""
#   undef __FUNCLINEW__
#   define __FUNCLINEW__ ""
#endif

#ifndef _countof
#define _countof(array) (sizeof(array) / sizeof(array[0]))
#endif

namespace logutils
{

    constexpr const char* yn(const bool v) noexcept { return (v ? "yes" : "no"); }

    constexpr const char* tf(const bool v) noexcept { return (v ? "true" : "false"); }

}

namespace build
{
    constexpr bool is_debug() noexcept
    {
#if defined(_DEBUG) || defined(DEBUG)
        return true;
#else
        return false;
#endif
    }

    constexpr bool is_release() noexcept
    {
        return !is_debug();
    }

    constexpr bool is_testing() noexcept
    {
#if defined(IM_AUTO_TESTING)
        return true;
#else
        return false;
#endif
    }

    constexpr bool is_store() noexcept
    {
#if defined(BUILD_FOR_STORE)
        return true;
#else
        return false;
#endif
    }

}

namespace environment
{
    inline namespace impl
    {
        constexpr std::string_view get() noexcept
        {
#if defined(APP_ENVIRONMENT)
            return std::string_view(APP_ENVIRONMENT);
#else
            return std::string_view();
#endif
        }
    }

    constexpr bool is_alpha() noexcept
    {
        return get() == "ALPHA";
    }

    constexpr bool is_beta() noexcept
    {
        return get() == "BETA";
    }

    constexpr bool is_release() noexcept
    {
        return get() == "RELEASE";
    }

    constexpr bool is_develop() noexcept
    {
        return get().empty() || get() == "DEVELOP";
    }
}

namespace platform
{
    constexpr bool is_windows() noexcept
    {
#if defined(_WIN32)
        return true;
#else
        return false;
#endif
    }

    constexpr bool is_apple() noexcept
    {
#if defined(__APPLE__)
        return true;
#else
        return false;
#endif
    }

    constexpr bool is_linux() noexcept
    {
#if defined(__linux__)
        return true;
#else
        return false;
#endif
    }

    constexpr bool is_x86_64() noexcept
    {
#ifdef __x86_64__
        return true;
#else
        return false;
#endif
    }
}

namespace common::utils
{
    inline std::vector<std::string_view>
        splitSV(std::string_view strv, std::string_view delims)
    {
        std::vector<std::string_view> output;
        size_t first = 0;

        while (first < strv.size())
        {
            const auto second = strv.find_first_of(delims, first);

            if (first != second)
                output.emplace_back(strv.substr(first, second - first));

            if (second == std::string_view::npos)
                break;

            first = second + 1;
        }

        return output;
    }

    inline std::vector<std::string_view>
        splitSV(std::string_view strv, char delim)
    {
        return splitSV(strv, std::string_view(&delim, 1));
    }
}

namespace core
{
    constexpr unsigned long BYTE     = 1;
    constexpr unsigned long KILOBYTE = 1024 * BYTE;
    constexpr unsigned long MEGABYTE = 1024 * KILOBYTE;
    constexpr unsigned long GIGABYTE = 1024 * MEGABYTE;

    const std::string KILOBYTE_STR = "kb";
    const std::string MEGABYTE_STR = "mb";
    const std::string GIGABYTE_STR = "gb";
    const std::string MILLISECONDS_STR = "ms";

    namespace stats
    {
        using event_prop_key_type = std::string;
        using event_prop_val_type = std::string;
        using event_prop_kv_type = std::pair<event_prop_key_type, event_prop_val_type>;
        using event_props_type = std::vector<event_prop_kv_type>;

        constexpr int32_t msg_pending_delay_s = 5;

        inline std::string round_value(const int32_t _value, const int32_t _step, const int32_t _max_value)
        {
            assert(_step);
            if (!_step)
                return std::string();

            if (_value > _max_value)
                return (std::to_string(_max_value) + '+');

            const auto base = _value / _step;
            const auto over = (_value % _step) ? 1 : 0;
            return std::to_string((base + over) * _step);
        }

        inline time_t round_to_hours(time_t _value)
        {
            return (_value / 3600) * 3600;
        }

        inline std::string round_interval(const long long _min_val, const long long _value,
                                          const long long _step, const long long _max_value)
        {
            assert(_value >= _min_val && _value <= _max_value && _step);
            if ((_value < _min_val) || (_value > _max_value) || !_step)
                return std::string();

             auto steps = (_value - _min_val) / _step;
             auto start = _min_val + steps * _step;
             auto end = (start + _step > _max_value) ? _max_value : start + _step;

             return std::to_string(start) + '-' + std::to_string(end);
        }

        inline std::string memory_size_interval(size_t _bytes)
        {
            std::string interval;

            if (_bytes <= 100 * MEGABYTE)
                interval = round_interval(0, _bytes / MEGABYTE, 100, 100) + MEGABYTE_STR;
            else if (_bytes <= 500 * MEGABYTE)
                interval = round_interval(100, _bytes/MEGABYTE, 50, 500) + MEGABYTE_STR;
            else if (_bytes <= 1 * GIGABYTE)
                interval = round_interval(500, _bytes/MEGABYTE, 100, 1024) + MEGABYTE_STR;
            else
                interval = "more1" + GIGABYTE_STR;

            return interval;
        }

        inline std::string duration_interval(long long _ms)
        {
            std::string interval;

            if (_ms <= 250)
                interval = core::stats::round_interval(0, _ms, 50, 250);
            else if (_ms <= 2000)
                interval = core::stats::round_interval(250, _ms, 250, 2000);
            else if (_ms <= 5000)
                interval = core::stats::round_interval(2000, _ms, 1000, 5000);
            else
                interval = "more5000";

            interval += MILLISECONDS_STR;

            return interval;
        }

        inline std::string disk_space_interval(long long _bytes)
        {
            std::string interval;

            if (_bytes > 1500 * MEGABYTE)
                interval = "1500mb +";
            else if (_bytes < 100 * MEGABYTE)
                interval = "< 100mb";
            else
                interval = round_interval(100, _bytes / MEGABYTE, 100, 1500) + MEGABYTE_STR;

            return interval;
        }

        inline std::string traffic_size_interval(size_t _bytes)
        {
            std::string interval;

            if (_bytes <= 100 * KILOBYTE)
                interval = round_interval(0, _bytes / KILOBYTE, 100, 100) + KILOBYTE_STR;
            else if (_bytes <= 500 * KILOBYTE)
                interval = round_interval(100, _bytes / KILOBYTE, 400, 500) + KILOBYTE_STR;
            else if (_bytes <= MEGABYTE)
                interval = round_interval(500, _bytes / KILOBYTE, 500, 1024) + KILOBYTE_STR;
            else if (_bytes <= 5 * MEGABYTE)
                interval = round_interval(1, _bytes / MEGABYTE, 1, 5) + MEGABYTE_STR;
            else if (_bytes <= 20 * MEGABYTE)
                interval = round_interval(5, _bytes / MEGABYTE, 5, 20) + MEGABYTE_STR;
            else if (_bytes <= 50 * MEGABYTE)
                interval = round_interval(20, _bytes / MEGABYTE, 30, 50) + MEGABYTE_STR;
            else if (_bytes <= 500 * MEGABYTE)
                interval = round_interval(50, _bytes / MEGABYTE, 50, 500) + MEGABYTE_STR;
            else
                interval = "more500" + MEGABYTE_STR;

            return interval;
        }
    }
}

namespace ffmpeg
{
    constexpr bool is_enable_streaming() noexcept
    {
        return false;//platform::is_windows();
    }
}

namespace core
{
    namespace dump
    {
        constexpr bool is_crash_handle_enabled() noexcept
        {
            // XXX : don't change it
            // use /settings/dump_type.txt: 0 for not handle crashes
            //                              1 for make mini dump
            //                              2 for make full dump

            if constexpr (platform::is_apple())
                return environment::is_alpha() || environment::is_beta() || environment::is_release();
            else
                return !build::is_debug();
        }
    }
}

