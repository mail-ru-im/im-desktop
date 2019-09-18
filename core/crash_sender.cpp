#include "stdafx.h"

#ifdef _WIN32

#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

#include "crash_sender.h"
#include "log/log.h"
#include "utils.h"
#include "core.h"
#include "http_request.h"
#include "async_task.h"
#include "../common.shared/keys.h"
#include "tools/system.h"

namespace core
{
    namespace dump
    {
        constexpr auto max_crash_stat_interval = 24;

        const std::string& get_hockeyapp_url()
        {
            const auto& app_id = build::get_product_variant(hockey_app_id, agent_hockey_app_id, biz_hokey_app_id, dit_hokey_app_id);
            static std::string hockeyapp_url = "https://rink.hockeyapp.net/api/2/apps/" + app_id + "/crashes/upload";
            return hockeyapp_url;
        }

        bool report_sender::send_to_hockey_app(const std::string& _login, const proxy_settings& _proxy)
        {
            // http://support.hockeyapp.net/kb/api/api-crashes
            core::http_request_simple post_request(_proxy, utils::get_user_agent());
            post_request.set_url(get_hockeyapp_url());
            post_request.set_normalized_url("https___rink_hockeyapp_net_api_2_apps_crashes_upload");
            post_request.set_post_form(true);
            post_request.push_post_form_parameter("contact", _login);
            post_request.push_post_form_filedata(L"log", utils::get_report_log_path());
            post_request.set_need_log(false);
            post_request.set_send_im_stats(false);

            auto dump_name = utils::get_report_mini_dump_path();
            try
            {
                boost::system::error_code e;
                auto size = boost::filesystem::file_size(dump_name, e);
                if (size < 10 * 1024 * 1024) // 10 mb
                    post_request.push_post_form_filedata(L"attachment0", dump_name);
                return post_request.post();
            }
            catch (const std::exception& /*exp*/)
            {
                // if error - delete folder
                return true;
            };
        }

        void report_sender::insert_imstat_event()
        {
            boost::system::error_code e;
            auto last_modified = boost::filesystem::last_write_time(utils::get_report_path(), e);
            auto crash_interval = std::min<time_t>(core::stats::round_to_hours(std::time(nullptr) - last_modified), max_crash_stat_interval);

            core::stats::event_props_type props;
            props.emplace_back("time", std::to_string(crash_interval));

            g_core->insert_im_stats_event(core::stats::im_stat_event_names::crash, std::move(props));
        }

        bool report_sender::is_report_existed()
        {
            return tools::system::is_exist(utils::get_report_path());
        }

        void report_sender::clear_report_folder()
        {
            boost::system::error_code e;
            boost::filesystem::remove_all(utils::get_report_path(), e);
        }

        report_sender::report_sender(const std::string& _login)
            : login_(_login)
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

                    if (ptr_this->send_to_hockey_app(ptr_this->login_, user_proxy))
                        ptr_this->clear_report_folder();
                    return 0;
                });
            }
        }
    }
}

#endif // _WIN32
