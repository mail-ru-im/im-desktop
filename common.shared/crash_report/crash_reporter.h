#pragma once

#include "../common_defs.h"
#include "../config/config.h"
#include "../version_info.h"

namespace google_breakpad
{
    class ExceptionHandler;
}

namespace crashpad
{
    class CrashpadClient;
}

#ifdef  _WIN32
namespace core::dump
{
    class crash_handler;
}
#endif

namespace crash_system
{
    class reporter
    {
    public:
#ifdef  _WIN32
        using string_type = std::wstring;
#else
        using string_type = std::string;
#endif
        ~reporter();

#ifdef  _WIN32
        static LONG WINAPI seh_handler(PEXCEPTION_POINTERS pExceptionPtrs);
        void set_process_exception_handlers();
        void set_thread_exception_handlers();
        void set_sys_handler_enabled(bool _sys_handler_enabled);
#endif

        void uninstall();

        static reporter& instance();

        static std::string submit_url(std::string_view _login)
        {
            std::string url;

            url += "https://ub.icq.net/upload-crash/desktop/";
            url += common::get_dev_id();
            url += '/';
            if (_login.empty())
                url += "XXXXXXXX";
            else
                url += _login;

            url += '/';
            url += core::tools::version_info().get_version();
            url += "/";

            return url;
        }

#ifdef __APPLE__
        void init(std::string_view _dump_path, std::string_view _login, std::string_view _bundle_path);
#endif

    private:
        static reporter make();

    private:
        reporter(const string_type& _dump_path);
        reporter();
#ifdef  _WIN32
        std::unique_ptr<core::dump::crash_handler> crash_handler_;
#elif __APPLE__
#if defined(BUILD_FOR_STORE)
        std::unique_ptr<google_breakpad::ExceptionHandler> breakpad_;
#else
        std::unique_ptr<crashpad::CrashpadClient> crashpad_;
#endif
#elif __linux__
        std::unique_ptr<google_breakpad::ExceptionHandler> breakpad_;
#endif
    };
}
