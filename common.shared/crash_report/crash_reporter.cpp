#include "stdafx.h"

#include "crash_reporter.h"

#ifdef  _WIN32
#include "../common.shared/win32/crash_handler.h"
#include <filesystem>
#elif __APPLE__
#if defined(BUILD_FOR_STORE)
#include "client/mac/handler/exception_handler.h"
#else
#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#endif //BUILD_FOR_STORE
#elif __linux__
#define __STDC_FORMAT_MACROS
#include "client/linux/handler/exception_handler.h"
#include <filesystem>
#endif

#include "../common.shared/string_utils.h"
#include "../common.shared/common_defs.h"

#include "../core/tools/strings.h"
#include "../utils.h"

/************************************************************************/
/* DumpCallback                                                         */
/************************************************************************/
#ifdef __linux__
static bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool success)
{
    return success;
}
#endif

#if defined(__APPLE__) && defined(BUILD_FOR_STORE)
static bool FilterCallback(void *context)
{
    return true;
}

// A callback function to run after the minidump has been written.
// |minidump_id| is a unique id for the dump, so the minidump
// file is <dump_dir>/<minidump_id>.dmp.
// |context| is the value passed into the constructor.
// |succeeded| indicates whether a minidump file was successfully written.
// Return true if the exception was fully handled and breakpad should exit.
// Return false to allow any other exception handlers to process the
// exception.
bool MinidumpCallback(const char *dump_dir, const char *minidump_id, void *context, bool succeeded)
{
    return succeeded;
}
#endif

namespace crash_system
{
    reporter::reporter(const string_type& _dump_path)
    {
#ifdef  _WIN32
        //breakpad_ = std::make_unique<google_breakpad::ExceptionHandler>(_dump_path, nullptr, nullptr, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpNormal, L"", nullptr);
        crash_handler_ = std::make_unique<core::dump::crash_handler>(config::get().string(config::values::product_name_short), _dump_path, false);
#elif __APPLE__ //  _WIN32
        if constexpr (!core::dump::is_crash_handle_enabled())
            return;
#if defined(BUILD_FOR_STORE)
        boost::system::error_code e;
        boost::filesystem::create_directories(_dump_path, e);
        breakpad_ = std::make_unique<google_breakpad::ExceptionHandler>(_dump_path, FilterCallback, MinidumpCallback, nullptr, true, nullptr);
#endif
#elif __linux__ //  _WIN32
        if constexpr (!core::dump::is_crash_handle_enabled())
            return;
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(_dump_path), ec);
        google_breakpad::MinidumpDescriptor descriptor(_dump_path);
        breakpad_ = std::make_unique<google_breakpad::ExceptionHandler>(descriptor, nullptr, DumpCallback, nullptr, true, -1);
#endif
    }

    void reporter::uninstall()
    {
#ifdef  _WIN32
        crash_handler_ = {};
#elif __APPLE__ //  _WIN32
#if defined(BUILD_FOR_STORE)
        breakpad_ = {};
#else
        crashpad_ = {};
#endif
#elif __linux__ //  _WIN32
        breakpad_ = {};
#endif
    }

    reporter::reporter() = default;

    reporter::~reporter() = default;

#ifdef  _WIN32
    LONG WINAPI reporter::seh_handler(PEXCEPTION_POINTERS pExceptionPtrs)
    {
        return core::dump::crash_handler::seh_handler(pExceptionPtrs);
    }

    void reporter::set_process_exception_handlers()
    {
        if (crash_handler_)
            crash_handler_->set_process_exception_handlers();
    }

    void reporter::set_thread_exception_handlers()
    {
        if (crash_handler_)
            crash_handler_->set_thread_exception_handlers();
    }

    void reporter::set_sys_handler_enabled(bool _sys_handler_enabled)
    {
        if (crash_handler_)
            crash_handler_->set_is_sys_handler_enabled(_sys_handler_enabled);
    }
#endif

    reporter& reporter::instance()
    {
        static auto r = make();
        return r;
    }

    reporter reporter::make()
    {
#ifdef _WIN32
        return reporter(core::utils::get_product_data_path());
#else
        return reporter(su::concat(core::tools::from_utf16(core::utils::get_product_data_path()), "/reports"));
#endif
    }

#ifdef __APPLE__
    void reporter::init(std::string_view _dump_path, std::string_view _base_url, std::string_view _login, std::string_view _bundle_path)
    {
#ifndef BUILD_FOR_STORE
        if constexpr (!core::dump::is_crash_handle_enabled())
            return;

        using namespace crashpad;
        // Cache directory that will store crashpad information and minidumps
        const auto path  = std::string(_dump_path);
        base::FilePath database(path);
        // Path to the out-of-process handler executable
        base::FilePath handler(su::concat(_bundle_path, "/Contents/Helpers/crashpad_handler"));
        // URL used to submit minidumps to
        std::string url = submit_url(_base_url, _login);
        // Optional annotations passed via --annotations to the handler
        std::map<std::string, std::string> annotations;
        // Optional arguments to pass to the handler
        std::vector<std::string> arguments;

        arguments.push_back("--no-rate-limit");

        //arguments.push_back("--no-upload-gzip");

        crashpad_ = std::make_unique<CrashpadClient>();

        std::unique_ptr<CrashReportDatabase> db = CrashReportDatabase::Initialize(database);

        if (db == nullptr || db->GetSettings() == nullptr)
            return;

        /* Enable automated uploads. */
        db->GetSettings()->SetUploadsEnabled(true);

        const bool success = crashpad_->StartHandler(
        handler,
        database,
        database,
        url,
        annotations,
        arguments,
        /* restartable */ true,
        /* asynchronous_start */ false
        );
        (void)success;
#endif // BUILD_FOR_STORE
    }
#endif
}
