#pragma once

namespace core
{
    namespace wim
    {
        struct downloaded_file_info
        {
            explicit downloaded_file_info(std::string _url, std::wstring _local_path = std::wstring())
                : url_(std::move(_url))
                , local_path_(std::move(_local_path))
            {
            }

            std::string url_;
            std::wstring local_path_;
        };
    }
}
