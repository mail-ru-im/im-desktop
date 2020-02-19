#include "stdafx.h"

#include "crash_reporter.h"

#ifdef  _WIN32
#include "../common.shared/win32/crash_handler.h"
#include <filesystem>
#elif __APPLE__
#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#elif __linux__
#define __STDC_FORMAT_MACROS
#include "client/linux/handler/exception_handler.h"
#include <filesystem>
#endif

#include "../core/tools/strings.h"

/************************************************************************/
/* DumpCallback                                                         */
/************************************************************************/
#ifdef __linux__
bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool success)
{
    return success;
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
#elif __linux__ //  _WIN32
        if constexpr (!core::dump::is_crash_handle_enabled())
            return;
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(_dump_path), ec);
        google_breakpad::MinidumpDescriptor descriptor(_dump_path);
        breakpad_ = std::make_unique<google_breakpad::ExceptionHandler>(descriptor, nullptr, DumpCallback, nullptr, true, -1);
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
        crash_handler_->set_process_exception_handlers();
    }

    void reporter::set_thread_exception_handlers()
    {
        crash_handler_->set_thread_exception_handlers();
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
        return reporter(common::get_user_profile() + L'\\' + core::tools::from_utf8(config::get().string(config::values::product_path)));
#elif __APPLE__
        return reporter();
#elif __linux__
        return reporter(getenv("HOME") + std::string("/.config/") + std::string(config::get().string(config::values::product_path)) + "/reports");
#endif
    }

#ifdef __APPLE__
    void reporter::init(std::string_view _dump_path, std::string_view _login, std::string_view _bundle_path)
    {
        if constexpr (!core::dump::is_crash_handle_enabled())
            return;
        const auto bundle_path = std::string(_bundle_path);
        using namespace crashpad;
        // Cache directory that will store crashpad information and minidumps
        const auto path  = std::string(_dump_path);
        base::FilePath database(path);
        // Path to the out-of-process handler executable
        base::FilePath handler(std::string(_bundle_path) + "/Contents/Helpers/crashpad_handler");
        // URL used to submit minidumps to
        std::string url = submit_url(_login);
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
    }
#endif
}
