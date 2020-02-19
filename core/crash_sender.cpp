#include "stdafx.h"

#if defined _WIN32 || defined __linux__

#include "crash_sender.h"
#include "utils.h"
#include "core.h"
#include "http_request.h"
#include "async_task.h"
#include "../common.shared/config/config.h"
#include "../common.shared/crash_report/crash_reporter.h"
#include "tools/system.h"
#include "proxy_settings.h"

namespace core
{
    namespace dump
    {
        constexpr auto max_crash_stat_interval = 24;

        bool report_sender::send(std::string_view _login, const proxy_settings& _proxy)
        {
            core::http_request_simple post_request(_proxy, utils::get_user_agent());
            post_request.set_url(crash_system::reporter::submit_url(_login));
            post_request.set_post_form(true);
            post_request.set_need_log(true);
            post_request.set_send_im_stats(false);

            std::error_code ec;
            auto it = std::filesystem::recursive_directory_iterator(utils::get_report_path(), ec);
            auto end = std::filesystem::recursive_directory_iterator();
            for (; it != end && !ec; it.increment(ec))
            {
                if (it->status().type() != std::filesystem::file_type::regular)
                    continue;

                if (const auto& path = it->path(); path.extension().wstring() == L".dmp")
                {
                    try
                    {
                        std::error_code e;
                        const auto size = std::filesystem::file_size(path, e);
                        if (size < 10 * 1024 * 1024) // 10 mb
                            post_request.push_post_form_filedata(L"upload_file_minidump", path.wstring());
                        if (post_request.post() && post_request.get_response_code() == 200)
                        {
                            std::filesystem::remove(path, e);
                            continue;
                        }
                        return false;
                    }
                    catch (const std::exception& /*exp*/)
                    {
                        // if error - delete folder
                        return true;
                    };
                }
            }

            return true;
        }

        void report_sender::insert_imstat_event()
        {
            std::error_code e;
            const auto last_modified = std::filesystem::last_write_time(utils::get_report_path(), e);
            const auto diff = std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::file_time_type::clock::now() - last_modified);
            auto crash_interval = std::min<time_t>(core::stats::round_to_hours(diff.count()), max_crash_stat_interval);

            core::stats::event_props_type props;
            props.emplace_back("time", std::to_string(crash_interval));

            g_core->insert_im_stats_event(core::stats::im_stat_event_names::crash, std::move(props));
        }

        bool report_sender::is_report_existed() const
        {
            const auto p = utils::get_report_path();
            return tools::system::is_exist(p) && !tools::system::is_empty(p);
        }

        void report_sender::clear_report_folder()
        {
            tools::system::clean_directory(utils::get_report_path());
        }

        report_sender::report_sender(std::string _login)
            : login_(std::move(_login))
        {
        }

        report_sender::~report_sender()
        {
            send_thread_.reset();
        }

        void report_sender::send_report()
        {
            if (core::dump::report_sender::is_report_existed())
            {
                insert_imstat_event();

                send_thread_ = std::make_unique<async_executer>("report_sender");

                send_thread_->run_async_function([wr_this = weak_from_this(), user_proxy = g_core->get_proxy_settings()]
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return 0;

                    if (ptr_this->send(ptr_this->login_, user_proxy))
                        ptr_this->clear_report_folder();
                    return 0;
                });
            }
        }
    }
}

#endif // defined _WIN32 || defined __linux__
