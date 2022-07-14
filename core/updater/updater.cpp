#include "stdafx.h"
#include "updater.h"

#include "../core.h"
#include "../async_task.h"
#include "../http_request.h"
#include "../utils.h"
#include "../tools/md5.h"
#include "../../common.shared/version_info.h"
#include "../../common.shared/config/config.h"
#include "../../common.shared/string_utils.h"
#include "../../common.shared/omicron_keys.h"
#include "../../common.shared/json_helper.h"
#include "../configuration/app_config.h"
#include "../configuration/host_config.h"
#include "../tools/features.h"
#include "../libomicron/include/omicron/omicron.h"

#if defined _WIN32 || defined __linux__  // xcode 10 fix
#include "../../../corelib/core_face.h"
#include "../../../corelib/collection_helper.h"
#endif
namespace
{
    std::chrono::seconds check_period()
    {
        return std::chrono::seconds(core::configuration::get_app_config().app_update_interval_secs());
    }

#ifdef _WIN32
    std::wstring installer_temp_filename()
    {
        wchar_t path[1024];
        if (!::GetTempPath(1024, path))
            return {};

        wchar_t filename[1024];
        auto installer_win = config::get().string(config::values::installer_exe_win);
        installer_win = installer_win.substr(0, installer_win.size() - std::string_view(".exe").size());
        if (!::GetTempFileName(path, core::tools::from_utf8(installer_win).c_str(), 0, filename))
            return {};

        return filename;
    }
#elif defined __linux__
    std::string installer_temp_filename()
    {
        std::string path;
        char buff[PATH_MAX];
        ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff) - 1);
        if (len != -1)
        {
            buff[len] = '\0';
            path = buff;
        }
        else
        {
            return {};
        }

        auto pos = path.rfind("/");
        if (pos == std::string::npos)
            return {};

        boost::replace_all(path, " ", "\\ ");
        return su::concat(std::string_view(path).substr(0, pos + 1), "updater");
    }
#else
    std::string installer_temp_filename() { return std::string{}; }
#endif
}

namespace core
{
    namespace update
    {
        struct update_params
        {
            std::string login_;
            std::string custom_url_;
            std::function<bool()> must_stop_;
        };

        enum class error
        {
            up_to_date = -1,
            update_ready = 0,
            network_error = 1
        };


        std::string get_update_server(const update_params& _params)
        {
            if (!features::is_update_from_backend_enabled())
            {
                const core::tools::version_info infoCurrent;
                const std::string current_build_version = infoCurrent.get_version();
                std::string updateble_build_version;
                //updateble_build_version = /10.0.1234/
                if constexpr (environment::is_alpha())
                    updateble_build_version = su::concat('/', omicronlib::_o(omicron::keys::update_alpha_version, current_build_version), '/');
                else if (g_core->get_install_beta_updates())
                    updateble_build_version = su::concat('/', omicronlib::_o(omicron::keys::update_beta_version, current_build_version), '/');
                else
                    updateble_build_version = su::concat('/', omicronlib::_o(omicron::keys::update_release_version, current_build_version), '/');

                if (!_params.custom_url_.empty())
                    return _params.custom_url_;

                return core::configuration::get_app_config().get_update_url(updateble_build_version);
            }
            else
            {
                // resolve https://jira.vk.team/browse/IMDESKTOP-18085
                // note: it's safe to simply append "https://" since internal
                // cache inside config::hosts already hold schemeless URLs
                return su::concat("https://", config::hosts::get_host_url(config::hosts::host_url_type::app_update), '/');
            }
        }

        std::string get_update_version_url(const update_params& _params)
        {
            return su::concat(get_update_server(_params), "version.json?login=", _params.login_);
        }

        updater::updater()
            : stop_(false)
            , thread_(nullptr)
        {
            timer_id_ = g_core->add_timer({ [this]()
            {
                check_if_need();

            } }, (build::is_debug() ? std::chrono::seconds(10) : std::chrono::minutes(10)));

            last_check_time_ = std::chrono::steady_clock::now() - check_period();
        }


        updater::~updater()
        {
            stop_ = true;

            g_core->stop_timer(timer_id_);
        }

        bool updater::must_stop() const
        {
            return stop_;
        }

        error run(const update_params& _params, const proxy_settings& _proxy);


        void updater::check_if_need()
        {
            if constexpr (build::is_debug())
                return;

            if constexpr (platform::is_apple())
                return;

            if constexpr (build::is_store())
                return;

            if constexpr (build::is_pkg_msi())
                return;

            if (!core::configuration::get_app_config().is_updateble())
                return;

            if ((std::chrono::steady_clock::now() - last_check_time_) > check_period())
                check({}, {});
        }

        void updater::check(std::optional<int64_t> _seq, std::optional<std::string> _custom_url)
        {
            if (!thread_)
                thread_ = std::make_unique<core::async_executer>("updater");

            auto make_params = [this](auto _url)
            {
                update_params params;
                params.login_ = g_core->get_root_login();
                params.must_stop_ = std::bind(&updater::must_stop, this);
                if (_url)
                    params.custom_url_ = *(std::move(_url));
                return params;
            };

            const auto update_from_backend = features::is_update_from_backend_enabled();
            const auto app_update_url = update_from_backend ? config::hosts::get_host_url(config::hosts::host_url_type::app_update) : std::string();
            if (!update_from_backend || (update_from_backend && !app_update_url.empty()))
            {
                thread_->run_async_function([params = make_params(std::move(_custom_url)), proxy = g_core->get_proxy_settings(), _seq = std::move(_seq)]
                    {
                        const auto ret = run(params, proxy);
                        if (ret == error::update_ready)
                        {
                            g_core->post_message_to_gui("update_ready", 0, nullptr);
                        }
                        else
                        {
                            if (_seq)
                            {
    #if defined _WIN32 || defined __linux__  // xcode 10 fix
                                coll_helper coll(g_core->create_collection(), true);
                                coll.set_value_as_bool("is_network_error", ret == error::network_error);
                                g_core->post_message_to_gui("up_to_date", *_seq, coll.get());
    #endif
                            }
                        }

                        return int(ret);
                    });
            }

            last_check_time_ = std::chrono::steady_clock::now();
        }

        error do_update(std::string_view _file, std::string_view _md5, const update_params& _params, const proxy_settings& _proxy);

        error run(const update_params& _params, const proxy_settings& _proxy)
        {
            http_request_simple request(_proxy, utils::get_user_agent(_params.login_), default_priority(), _params.must_stop_);
            request.set_url(get_update_version_url(_params));
            request.set_normalized_url("update");

            if (request.get() != curl_easy::completion_code::success)
                return error::network_error;

            int32_t http_code = (int32_t)request.get_response_code();
            if (http_code != 200)
                return error::network_error;

            auto response = dynamic_cast<tools::binary_stream*>(request.get_response().get());
            im_assert(response);

            if (!response->available())
                return error::network_error;

            response->write((char)0);

            //{"info":{"version":{"major":10, "minor":0, "buildnumber":10008},"file":"icqsetup.exe","md5":"afda633be58e18a0aaeb3e88481058e9"}}

            rapidjson::Document doc;
            if (doc.ParseInsitu(response->read(response->available())).HasParseError())
                return error::up_to_date;

            auto iter_info = doc.FindMember("info");
            if (iter_info == doc.MemberEnd())
                return error::up_to_date;

            auto iter_version = iter_info->value.FindMember("version");
            if (iter_version == iter_info->value.MemberEnd())
                return error::up_to_date;

            int32_t major, minor, build;
            std::string file;
            std::string md5;

            const auto parse_res = {
                tools::unserialize_value(iter_version->value, "major", major),
                tools::unserialize_value(iter_version->value, "minor", minor),
                tools::unserialize_value(iter_version->value, "buildnumber", build),
                tools::unserialize_value(iter_info->value, "file", file),
                tools::unserialize_value(iter_info->value, "md5", md5),
            };

            if (!std::all_of(parse_res.begin(), parse_res.end(), [](auto x) { return x; }))
                return error::up_to_date;

            tools::version_info local_version_info = tools::version_info();
            tools::version_info server_version_info = tools::version_info(major, minor, build);

            if (local_version_info < server_version_info)
                return do_update(file, md5, _params, _proxy);

            return error::up_to_date;
        }


#ifdef _WIN32
        error run_installer(const std::wstring& _filename)
        {
            auto error = error::up_to_date;

            PROCESS_INFORMATION pi = { 0 };
            STARTUPINFO si = { 0 };
            si.cb = sizeof(si);

            std::wstring command = su::wconcat(L'"', _filename, L'"', L" -update -waitchild");

            if (!::CreateProcess(0, command.data(), 0, 0, 0, 0, 0, 0, &si, &pi))
                return error;

            const auto res = ::WaitForSingleObject(pi.hProcess, std::chrono::milliseconds(std::chrono::hours(1)).count()); // it's enough to execute installer
            ::CloseHandle(pi.hProcess);
            ::CloseHandle(pi.hThread);

            if (res == WAIT_OBJECT_0)
                error = error::update_ready;

            return error;

        }
#elif defined __linux__
        error run_installer(const std::string& _filename)
        {
            auto error = error::update_ready;

            std::string sys_path = _filename;

            std::string chmod = su::concat("chmod 755 ", sys_path);
            system(chmod.c_str());
            system(sys_path.c_str());

            return error;
        }
#else
        error run_installer(const std::string&) { return error::update_ready; }
#endif


        error do_update(std::string_view _file, std::string_view _md5, const update_params& _params, const proxy_settings& _proxy)
        {
            auto tmp_file_flags = std::ios::binary | std::ios::out;
            auto tmp_file_name = installer_temp_filename();
            std::ofstream tmp_file(tmp_file_name, tmp_file_flags);
            if (!tmp_file.good())
                return error::up_to_date;

            http_request_simple request(_proxy, utils::get_user_agent(_params.login_), default_priority(), _params.must_stop_);
            request.set_url(get_update_server(_params) + std::string(_file));
            request.set_normalized_url("https___mra_mail_ru_update");
            request.set_timeout(std::chrono::minutes(15));
            request.set_need_log(false);
            request.set_use_curl_decompresion(true);
            request.set_output_stream(std::make_shared<tools::file_output_stream>(std::move(tmp_file)));
            if (request.get() != curl_easy::completion_code::success)
                return error::network_error;

            request.get_response()->close();
            const int32_t http_code = (uint32_t)request.get_response_code();
            if (http_code != 200)
                return error::network_error;

            auto ifile = std::ifstream(tmp_file_name, std::ios::binary|std::ios::in);
            if (!ifile)
                return error::network_error;

            if (core::tools::md5(ifile) != _md5)
                return error::up_to_date;

            return run_installer(tmp_file_name);
        }
    }
}
