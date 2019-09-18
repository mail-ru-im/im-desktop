#include "stdafx.h"
#include "updater.h"

#include "../core.h"
#include "../async_task.h"
#include "../http_request.h"
#include "../utils.h"
#include "../../common.shared/version_info.h"
#include "openssl/md5.h"
#include "../configuration/app_config.h"
#include "../tools/json_helper.h"

namespace core
{
    namespace update
    {
        constexpr auto check_period = std::chrono::hours(24); // one day

        struct update_params
        {
            std::string login_;
            std::string custom_url_;
            std::function<bool()> must_stop_;
        };

        std::string get_update_server(const update_params& _params)
        {
            if (!_params.custom_url_.empty())
                return _params.custom_url_;

            if constexpr (platform::is_windows())
            {
                if constexpr (build::is_alpha()) {
                    return build::get_product_variant("https://im-desktop.mail.msk/update/windows/icq/",
                                                      "https://im-desktop.mail.msk/update/windows/agent/",
                                                      "https://myteam-www.hb.bizmrg.com/alpha-version/win/x32/0.0.0/",
                                                      "https://messenger-www.hb.bizmrg.com/alpha-version/win/x32/0.0.0/");
                }
                else
                {
                    return build::get_product_variant("https://mra.mail.ru/icq10_win_update/",
                                                      "https://mra.mail.ru/mra10_win_update/",
                                                      "https://myteam-www.hb.bizmrg.com/win/x32/0.0.0/",
                                                      "https://messenger-www.hb.bizmrg.com/win/x32/0.0.0/");
                }
            }
            else
            {
                if constexpr (platform::is_x86_64())
                {
                    if (build::is_alpha())
                    {
                        return build::get_product_variant("https://im-desktop.mail.msk/update/linux/icq/64/",
                                                          "https://im-desktop.mail.msk/update/linux/agent/64/",
                                                          "https://myteam-www.hb.bizmrg.com/alpha-version/linux/x64/0.0.0/",
                                                          "https://messenger-www.hb.bizmrg.com/alpha-version/linux/x64/0.0.0/");
                    }
                    else
                    {
                        return build::get_product_variant("https://mra.mail.ru/icq10_linux_update_x64/",
                                                          "https://mra.mail.ru/mra10_linux_update_x64/",
                                                          "https://myteam-www.hb.bizmrg.com/linux/x64/0.0.0/",
                                                          "https://messenger-www.hb.bizmrg.com/linux/x64/0.0.0/");
                    }
                }
                else
                {
                    if (build::is_alpha())
                    {
                        return build::get_product_variant("https://im-desktop.mail.msk/update/linux/icq/32/",
                                                          "https://im-desktop.mail.msk/update/linux/agent/32/",
                                                          "https://myteam-www.hb.bizmrg.com/alpha-version/linux/x32/0.0.0/",
                                                          "https://messenger-www.hb.bizmrg.com/alpha-version/linux/x32/0.0.0/");
                    }
                    else
                    {
                    return build::get_product_variant("https://mra.mail.ru/icq10_linux_update/",
                                                      "https://mra.mail.ru/mra10_linux_update/",
                                                      "https://myteam-www.hb.bizmrg.com/linux/x32/0.0.0/",
                                                      "https://messenger-www.hb.bizmrg.com/linux/x32/0.0.0/");
                    }
                }
            }

        }

        std::string get_update_version_url(const update_params& _params)
        {
            return get_update_server(_params) + "version.js?login=" + _params.login_;
        }

        updater::updater()
            : stop_(false)
            , thread_(nullptr)
        {
            timer_id_ = g_core->add_timer([this]()
            {
                check_if_need();

            }, (build::is_debug() ? std::chrono::seconds(10) : std::chrono::minutes(10)));

            last_check_time_ = std::chrono::system_clock::now() - check_period;
        }


        updater::~updater()
        {
            stop_ = true;

            g_core->stop_timer(timer_id_);
        }

        bool updater::must_stop()
        {
            return stop_;
        }

        int32_t run(const update_params& _params, const proxy_settings& _proxy);


        void updater::check_if_need()
        {
            if constexpr (build::is_debug())
                return;

            if constexpr (platform::is_apple())
                return;

            if constexpr (build::is_store())
                return;

            if (!core::configuration::get_app_config().is_updateble())
                return;

            if ((std::chrono::system_clock::now() - last_check_time_) > check_period)
                check();
        }

        void updater::check(const std::string& _custom_url)
        {
            update_params params;
            params.login_ = g_core->get_root_login();
            params.must_stop_ = std::bind(&updater::must_stop, this);
            params.custom_url_ = _custom_url;

            if (!thread_)
                thread_ = std::make_unique<core::async_executer>("updater");

            thread_->run_async_function(
                [params, proxy = g_core->get_proxy_settings()]
                {
                    const auto ret = run(params, proxy);
                    if (ret == 0)
                        g_core->post_message_to_gui("update_ready", 0, nullptr);

                    return ret;
                });

            last_check_time_ = std::chrono::system_clock::now();
        }

        int32_t do_update(std::string_view _file, std::string_view _md5, const update_params& _params, const proxy_settings& _proxy);

        int32_t run(const update_params& _params, const proxy_settings& _proxy)
        {
            http_request_simple request(_proxy, utils::get_user_agent(_params.login_), _params.must_stop_);
            request.set_url(get_update_version_url(_params));
            request.set_normalized_url("update");

            if (!request.get())
                return -1;

            int32_t http_code = (int32_t)request.get_response_code();
            if (http_code != 200)
                return -1;

            auto response = dynamic_cast<tools::binary_stream*>(request.get_response().get());
            assert(response);

            if (!response->available())
                return -1;

            response->write((char) 0);

            //{"info":{"version":{"major":10, "minor":0, "buildnumber":10008},"file":"icqsetup.exe","md5":"afda633be58e18a0aaeb3e88481058e9"}}

            rapidjson::Document doc;
            if (doc.ParseInsitu(response->read(response->available())).HasParseError())
                return -1;

            auto iter_info = doc.FindMember("info");
            if (iter_info == doc.MemberEnd())
                return -1;

            auto iter_version = iter_info->value.FindMember("version");
            if (iter_version == iter_info->value.MemberEnd())
                return -1;

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

            if (std::find(parse_res.begin(), parse_res.end(), false) != parse_res.end())
                return -1;

            tools::version_info local_version_info = tools::version_info();
            tools::version_info server_version_info = tools::version_info(major, minor, build);

            if (local_version_info < server_version_info)
                return do_update(file, md5, _params, _proxy);

            return -1;
        }

        int32_t run_installer(const tools::binary_stream& _data);

        int32_t do_update(std::string_view _file, std::string_view _md5, const update_params& _params, const proxy_settings& _proxy)
        {
            http_request_simple request(_proxy, utils::get_user_agent(_params.login_), _params.must_stop_);
            request.set_url(get_update_server(_params) + std::string(_file));
            request.set_normalized_url("https___mra_mail_ru_update");
            request.set_timeout(std::chrono::minutes(15));
            request.set_need_log(false);
            if (!request.get())
                return -1;

            int32_t http_code = (uint32_t)request.get_response_code();
            if (http_code != 200)
                return -1;

            auto response = dynamic_cast<tools::binary_stream*>(request.get_response().get());
            assert(response);

            if (!response->available())
                return -1;

            int32_t size = response->available();

            MD5_CTX md5handler;
            unsigned char md5digest[MD5_DIGEST_LENGTH];

            MD5_Init(&md5handler);
            MD5_Update(&md5handler, response->read(size), size);
            MD5_Final(md5digest,&md5handler);

            char buffer[100];
            std::string md5;

            for (int32_t i = 0; i < MD5_DIGEST_LENGTH; i++)
            {
                sprintf(buffer, "%02x", md5digest[i]);
                md5 += buffer;
            }

            if (md5 != _md5)
                return -1;

            response->reset_out();

            return run_installer(*response);
        }

        int32_t run_installer(const tools::binary_stream& _data)
        {
#ifdef _WIN32
            wchar_t temp_path[1024];
            if (!::GetTempPath(1024, temp_path))
                return -1;

            wchar_t temp_file_name[1024];
            if (!::GetTempFileName(temp_path, build::get_product_variant(L"icqsetup", L"magentsetup", L"myteamsetup", L"ditsetup"), 0, temp_file_name))
                return -1;

            _data.save_2_file(temp_file_name);

            PROCESS_INFORMATION pi = {0};
            STARTUPINFO si = {0};
            si.cb = sizeof(si);

            wchar_t command[1024];
            swprintf_s(command, 1023, L"\"%s\" -update", temp_file_name);

            if (!::CreateProcess(0, command, 0, 0, 0, 0, 0, 0, &si, &pi))
                return -1;

            ::CloseHandle( pi.hProcess );
            ::CloseHandle( pi.hThread );
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
                return -1;
            }

            auto pos = path.rfind("/");
            if (pos == std::string::npos)
                return - 1;

            path = path.substr(0, pos + 1);
            path += "updater";
            std::string sys_path = path;
            boost::replace_all(sys_path, " ", "\\ ");
            std::string rm = "rm -f ";
            rm += sys_path;
            system(rm.c_str());
            _data.save_2_file(core::tools::from_utf8(path));
            std::string chmod = "chmod 755 ";
            chmod += sys_path;
            system(chmod.c_str());
            system(sys_path.c_str());
#endif //_WIN32
            return  0;
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
