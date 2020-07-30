#include "stdafx.h"
#include <lm.h>

#include "../system.h"
#include "dll.h"
#include "../../profiling/profiler.h"
#include "../common.shared/common_defs.h"

namespace fs = boost::filesystem;

CORE_TOOLS_SYSTEM_NS_BEGIN

namespace
{
    std::wstring get_user_downloads_dir_xp();

    std::wstring get_user_downloads_dir_vista();
}

unsigned long get_current_thread_id()
{
    return ::GetCurrentThreadId();
}

bool is_dir_writable(const std::wstring &_dir_path_str)
{
    const fs::wpath dir_path(_dir_path_str);

    boost::system::error_code error;
    const auto is_dir = fs::is_directory(dir_path, error);
    assert(is_dir);

    if (!is_dir)
    {
        return false;
    }

    const auto test_path = (dir_path / generate_guid());

    {
        std::ofstream out(test_path.wstring());
        if (out.fail())
        {
            return false;
        }
    }

    fs::remove(test_path, error);

    return true;
}

bool core::tools::system::delete_file(const std::wstring& _file_name)
{
    boost::system::error_code error;
    boost::filesystem::remove(_file_name, error);
    return !error;
}

bool move_file(const std::wstring& _old_file, const std::wstring& _new_file)
{
    return !!::MoveFileEx(_old_file.c_str(), _new_file.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
}

bool copy_file(const std::wstring& _old_file, const std::wstring& _new_file)
{
    return !!::CopyFile(_old_file.c_str(), _new_file.c_str(), false);
}

bool compare_dirs(const std::wstring& _dir1, const std::wstring& _dir2)
{
    if (_dir1.empty() || _dir2.empty())
        return false;

    boost::system::error_code error;
    return fs::equivalent(fs::path(_dir1), fs::path(_dir2), error);
}

std::wstring get_file_directory(const std::wstring& file)
{
    fs::wpath p(file);
    return p.parent_path().wstring();
}

std::wstring get_file_name(const std::wstring& file)
{
    fs::wpath p(file);
    return p.filename().wstring();
}

std::wstring get_temp_directory()
{
    wchar_t path[MAX_PATH + 1];
    if (::GetTempPath(MAX_PATH, path) != 0)
        return path;

    return std::wstring();
}

std::wstring get_user_downloads_dir()
{
    static std::wstring cached_path;

    if (!cached_path.empty())
    {
        return cached_path;
    }

    cached_path = get_user_downloads_dir_vista();

    if (!cached_path.empty())
    {
        return cached_path;
    }

    cached_path = get_user_downloads_dir_xp();

    return cached_path;
}

std::string to_upper(std::string_view str)
{
    return boost::locale::to_upper(str.data(), str.data() + str.size());
}

std::string to_lower(std::string_view str)
{
    return boost::locale::to_lower(str.data(), str.data() + str.size());
}

namespace
{
    std::wstring get_user_downloads_dir_xp()
    {
        WCHAR path[MAX_PATH] = { 0 };

        if (const auto error = SHGetFolderPath(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, 0, Out path); FAILED(error))
            return std::wstring();

        boost::system::error_code e;
        assert(fs::is_directory(path, e));
        return path;
    }

    std::wstring get_user_downloads_dir_vista()
    {
        PWSTR path = nullptr;

        if (const auto error = SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, Out & path); FAILED(error))
            return std::wstring();

        std::wstring result(path);
        boost::system::error_code e;
        assert(fs::is_directory(result, e));

        ::CoTaskMemFree(path);

        return result;
    }
}

std::string get_os_version_string()
{
    return common::get_win_os_version_string();
}

std::string get_short_file_name(const std::wstring& _file_name)
{
    auto length = GetShortPathName(_file_name.c_str(), NULL, 0);

    std::string result;
    if (length == 0)
        return result;

    std::vector<wchar_t> wide_buffer(length);

    length = GetShortPathName(_file_name.c_str(), wide_buffer.data(), length);
    if (length == 0)
        return result;

    length = WideCharToMultiByte(CP_ACP, 0, wide_buffer.data(), length, NULL, 0, NULL, NULL);
    if (length == 0)
        return result;

    std::vector<char> buffer(length);

    length = WideCharToMultiByte(CP_ACP, 0, wide_buffer.data(), length, buffer.data(), length, NULL, NULL);
    if (length == 0)
        return result;

    result.assign(buffer.begin(), buffer.end());
    return result;
}

bool is_do_not_dirturb_on()
{
    return false;
}

CORE_TOOLS_SYSTEM_NS_END