#pragma once

// ----------------------------------------------
// fix for msvc++/clang compartibility

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#if defined(DEBUG) && !defined(_DEBUG)
#define _DEBUG
#endif

// ----------------------------------------------

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

    constexpr bool is_pkg_msi() noexcept
    {
#if defined(BUILD_PKG_MSI)
        return true;
#else
        return false;
#endif
    }

    constexpr bool is_pkg_rpm() noexcept
    {
#if defined(BUILD_PKG_RPM)
        return true;
#else
        return false;
#endif
    }

    constexpr bool has_webengine() noexcept
    {
#if defined(HAS_WEB_ENGINE)
        return true;
#else
        return false;
#endif
    }

} // namespace build

namespace environment
{
    inline namespace impl
    {
        constexpr std::string_view get() noexcept
        {
#if defined(APP_ENVIRONMENT)
            return std::string_view(APP_ENVIRONMENT);
#else
            return {};
#endif
        }
    } // namespace impl

    constexpr bool is_alpha() noexcept
    {
        return impl::get() == "ALPHA";
    }

    constexpr bool is_beta() noexcept
    {
        return impl::get() == "BETA";
    }

    constexpr bool is_release() noexcept
    {
        return impl::get() == "RELEASE";
    }

    constexpr bool is_develop() noexcept
    {
        return impl::get().empty() || get() == "DEVELOP";
    }
} // namespace environment

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
} // namespace platform
