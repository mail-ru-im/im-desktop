#pragma once

#include <boost/filesystem.hpp>

#define CORE_TOOLS_SYSTEM_NS_BEGIN namespace core { namespace tools { namespace system {
#define CORE_TOOLS_SYSTEM_NS_END } } }

CORE_TOOLS_SYSTEM_NS_BEGIN

bool is_dir_writable(const std::wstring &_dir_path_str);

bool delete_file(const std::wstring& _file_name);

bool move_file(const std::wstring& _old_file, const std::wstring& _new_file);

bool copy_file(const std::wstring& _old_file, const std::wstring& _new_file);

bool compare_dirs(const std::wstring& _dir1, const std::wstring& _dir2);

std::wstring get_file_directory(const std::wstring& file);

std::wstring get_file_name(const std::wstring& file);

std::wstring create_temp_file_path(std::wstring_view _filename = {});

std::wstring get_temp_directory();

#ifdef _WIN32
    std::string get_short_file_name(const std::wstring& _file_name);
#else
    std::string get_home_directory();
    std::wstring get_user_profile();
#endif // WIN32

std::string generate_guid();

std::string generate_internal_id();

unsigned long get_current_thread_id();

std::wstring get_user_downloads_dir();

std::string to_upper(std::string_view str);

std::string to_lower(std::string_view str);

size_t get_memory_size_mb();

bool is_exist(std::wstring_view path);

bool is_empty(std::wstring_view path);

inline bool is_exist(const std::wstring& path)
{
    return is_exist(std::wstring_view(path));
}

bool is_exist(const boost::filesystem::wpath & path);

bool create_directory(const std::wstring& path);

bool create_directory(const boost::filesystem::wpath& path);

bool create_directory_if_not_exists(const boost::filesystem::wpath& _path);

bool delete_directory(const std::wstring& path);

bool delete_directory(const boost::filesystem::wpath& path);

bool create_empty_file(const std::wstring &_path);

std::string get_os_version_string();

bool read_file(const boost::filesystem::wpath& _path, std::string& _result);

bool unzip(const boost::filesystem::path& _archive, const boost::filesystem::path& _target_dir);

bool clean_directory(const boost::filesystem::path& _dir);

std::ifstream open_file_for_read(std::string_view _file_name, std::ios_base::openmode _mode = std::ios_base::in);
std::ifstream open_file_for_read(std::wstring_view _file_name, std::ios_base::openmode _mode = std::ios_base::in);

std::ofstream open_file_for_write(std::string_view _file_name, std::ios_base::openmode _mode = std::ios_base::out);
std::ofstream open_file_for_write(std::wstring_view _file_name, std::ios_base::openmode _mode = std::ios_base::out);

size_t get_file_size(std::string_view _file_name);
size_t get_file_size(std::wstring_view _file_name);

bool is_do_not_dirturb_on();

CORE_TOOLS_SYSTEM_NS_END
