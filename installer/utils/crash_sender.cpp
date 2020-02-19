#include "stdafx.h"

#ifdef _WIN32

#include "../../common.shared/win32/common_crash_sender.h"

#include "../../external/curl/include/curl.h"
#include "../../common.shared/config/config.h"
#include "../../common.shared/crash_report/crash_reporter.h"
#include "../logic/tools.h"

#include "curl_utils.h"

using namespace installer;

namespace common_crash_sender
{
    static bool send_dump(const std::string& _path)
    {
        CURLcode res;
        long response_code = 0;

        curl_global_init(CURL_GLOBAL_ALL);

        auto curl = curl_easy_init();
        if (curl)
        {
            struct curl_httppost *formpost = nullptr;
            struct curl_httppost *lastptr = nullptr;

            curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, "upload_file_minidump",
                CURLFORM_FILE, _path.c_str(),
                CURLFORM_END);

            curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, curl_utils::ssl_ctx_callback_impl);

            curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

            curl_easy_setopt(curl, CURLOPT_URL, crash_system::reporter::submit_url({}).c_str());

            res = curl_easy_perform(curl);
            if (res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

            curl_easy_cleanup(curl);

            curl_formfree(formpost);
        }
        curl_global_cleanup();
        return res == CURLE_OK && response_code == 200;
    }

    void post_dump_and_delete()
    {
        const auto report_path = QString(logic::get_product_folder() % ql1s("/reports")).toStdWString();

        std::error_code ec;
        auto it = std::filesystem::recursive_directory_iterator(report_path, ec);
        auto end = std::filesystem::recursive_directory_iterator();
        for (; it != end && !ec; it.increment(ec))
        {
            if (it->status().type() != std::filesystem::file_type::regular)
                continue;

            if (const auto& path = it->path(); path.extension().wstring() == L".dmp" && path.filename().wstring().find(L"installer") != std::wstring::npos)
            {
                std::error_code e;
                try
                {
                    const auto size = std::filesystem::file_size(path, e);
                    if (size < 10 * 1024 * 1024) // 10 mb
                    {
                        if (send_dump(path.u8string()))
                        {
                            std::filesystem::remove(path, e);
                            continue;
                        }
                    }
                    else
                    {
                        std::filesystem::remove(path, e);
                    }
                }
                catch (const std::exception&)
                {
                    std::filesystem::remove(path, e);
                };
            }
        }
    }

    void start_sending_process()
    {
        PROCESS_INFORMATION pi = {0};
        STARTUPINFO si = {0};
        si.cb = sizeof(si);

        wchar_t buffer[1025];
        ::GetModuleFileName(0, buffer, 1024);

        wchar_t command[1024];
        swprintf_s(command, 1023, L"\"%s\" %s", buffer, QString(send_dump_arg).toStdWString().c_str());

        if (::CreateProcess(0, command, 0, 0, 0, 0, 0, 0, &si, &pi))
        {
            ::CloseHandle( pi.hProcess );
            ::CloseHandle( pi.hThread );
        }
    }
}

#endif // _WIN32
