#include "stdafx.h"
#include "updater.h"

#include "../core.h"
#include "../async_task.h"
#include "../http_request.h"
#include "../utils.h"
#include "../../common.shared/version_info.h"
#include "../../common.shared/config/config.h"
#include "../../common.shared/string_utils.h"
#include "../../common.shared/omicron_keys.h"
#include "openssl/md5.h"
#include "../configuration/app_config.h"
#include "../configuration/host_config.h"
#include "../tools/json_helper.h"
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
                return su::concat(config::hosts::get_host_url(config::hosts::host_url_type::app_update), '/');
            }
        }

        std::string get_update_version_url(const update_params& _params)
        {
            return su::concat(get_update_server(_params), "version.json?login=",  _params.login_);
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
            assert(response);

            if (!response->available())
                return error::network_error;

            response->write((char) 0);

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

        error run_installer(const tools::binary_stream& _data);

        error do_update(std::string_view _file, std::string_view _md5, const update_params& _params, const proxy_settings& _proxy)
        {
            http_request_simple request(_proxy, utils::get_user_agent(_params.login_), default_priority(), _params.must_stop_);
            request.set_url(get_update_server(_params) + std::string(_file));
            request.set_normalized_url("https___mra_mail_ru_update");
            request.set_timeout(std::chrono::minutes(15));
            request.set_need_log(false);
            if (request.get() != curl_easy::completion_code::success)
                return error::network_error;

            int32_t http_code = (uint32_t)request.get_response_code();
            if (http_code != 200)
                return error::network_error;

            auto response = dynamic_cast<tools::binary_stream*>(request.get_response().get());
            assert(response);

            if (!response->available())
                return error::network_error;

            int32_t size = response->available();

            MD5_CTX md5handler;
            unsigned char md5digest[MD5_DIGEST_LENGTH];

            MD5_Init(&md5handler);
            MD5_Update(&md5handler, response->read(size), size);
            MD5_Final(md5digest,&md5handler);

            char buffer[100];
            std::string md5;

            for (size_t i = 0; i < MD5_DIGEST_LENGTH; ++i)
            {
                sprintf(buffer, "%02x", md5digest[i]);
                md5 += buffer;
            }

            if (md5 != _md5)
                return error::up_to_date;

            response->reset_out();

            return run_installer(*response);
        }

        error run_installer(const tools::binary_stream& _data)
        {
#ifdef _WIN32
            wchar_t temp_path[1024];
            if (!::GetTempPath(1024, temp_path))
                return error::up_to_date;

            wchar_t temp_file_name[1024];
            auto installer_win = config::get().string(config::values::installer_exe_win);
            installer_win = installer_win.substr(0, installer_win.size() - std::string_view(".exe").size());
            if (!::GetTempFileName(temp_path, tools::from_utf8(installer_win).c_str(), 0, temp_file_name))
                return error::up_to_date;

            _data.save_2_file(temp_file_name);

            PROCESS_INFORMATION pi = {0};
            STARTUPINFO si = {0};
            si.cb = sizeof(si);

            std::wstring command = su::wconcat(L'"', temp_file_name, L'"', L" -update -waitchild");

            if (!::CreateProcess(0, command.data(), 0, 0, 0, 0, 0, 0, &si, &pi))
                return error::up_to_date;

            const auto res = ::WaitForSingleObject(pi.hProcess, std::chrono::milliseconds(std::chrono::hours(1)).count()); // it's enough to execute installer
            ::CloseHandle(pi.hProcess);
            ::CloseHandle(pi.hThread);

            if (res != WAIT_OBJECT_0)
                return error::up_to_date;

#elif defined __linux__
            std::string path;
            char buff[PATH_MAX];
            ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
            if (len != -1)
            {
                buff[len] = '\0';
                path = buff;
            }
            else
            {
                return error::up_to_date;
            }

            auto pos = path.rfind("/");
            if (pos == std::string::npos)
                return error::up_to_date;

            path = su::concat(std::string_view(path).substr(0, pos + 1), "updater");
            std::string sys_path = path;
            boost::replace_all(sys_path, " ", "\\ ");
            std::string rm = su::concat("rm -f ", sys_path);
            system(rm.c_str());
            _data.save_2_file(core::tools::from_utf8(path));
            std::string chmod = su::concat("chmod 755 ", sys_path);
            system(chmod.c_str());
            system(sys_path.c_str());
#endif //_WIN32
            return error::update_ready;
        }

        void clean_prev_instalations()
        {
#ifdef _WIN32
            wchar_t exe_name[1025];
            if (::GetModuleFileName(0, exe_name, 1024))
            {
                boost::filesystem::wpath file_path(exe_name);
                boost::filesystem::wpath current_path = file_path.parent_path();
                boost::filesystem::wpath parent_path = file_path.parent_path().parent_path();

                std::wstring current_leaf = current_path.leaf().wstring();

                if (!tools::is_number(tools::from_utf16(current_leaf)))
                    return;

                try
                {
                    boost::system::error_code error;
                    boost::filesystem::directory_iterator end_iter;
                    for (boost::filesystem::directory_iterator dir_iter(parent_path, error); dir_iter != end_iter && !error; dir_iter.increment(error))
                    {
                        if (!boost::filesystem::is_directory(dir_iter->status()))
                            continue;

                        if (current_path == dir_iter->path())
                            continue;

                        std::wstring leaf = dir_iter->path().leaf().wstring();

                        if (!tools::is_number(tools::from_utf16(leaf)))
                            continue;

                        int32_t current_num = std::stoi(current_leaf);
                        int32_t num = std::stoi(leaf);

                        if (current_num < num)
                            continue;

                        boost::filesystem::remove_all(dir_iter->path(), error);
                    }
                }
                catch (const std::exception&)
                {

                }
            }

#endif //_WIN32
        }
    }
}
