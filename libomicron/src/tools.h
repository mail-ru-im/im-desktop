#pragma once

namespace omicronlib
{
    namespace tools
    {
        void set_this_thread_name(const std::string& _name);

        std::string from_utf16(const std::wstring& _source_16);
        std::wstring from_utf8(const std::string& _source_8);

        std::ifstream open_file_for_read(const std::wstring& _file_name, std::ios_base::openmode _mode = std::ios_base::in);
        std::ofstream open_file_for_write(const std::wstring& _file_name, std::ios_base::openmode _mode = std::ios_base::out);

        bool is_exist(const std::wstring& _path);

        bool create_parent_directories_for_file(const std::wstring& _file_name);

        bool rename_file(const std::wstring& _path_from, const std::wstring& _path_to);
    }
}
