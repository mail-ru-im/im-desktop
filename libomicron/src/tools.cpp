#include "stdafx.h"

#include "tools.h"

namespace omicronlib { namespace tools {

#ifdef _WIN32 // https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2008/xcb2z8hs(v=vs.90)
    void SetThreadName(DWORD dwThreadID, const char* threadName)
    {
#ifndef __MINGW32__
        constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
        typedef struct tagTHREADNAME_INFO
        {
            DWORD dwType; // Must be 0x1000.
            LPCSTR szName; // Pointer to name (in user addr space).
            DWORD dwThreadID; // Thread ID (-1=caller thread).
            DWORD dwFlags; // Reserved for future use, must be zero.
        } THREADNAME_INFO;
#pragma pack(pop)

        if (IsDebuggerPresent())
        {
            THREADNAME_INFO info;
            info.dwType = 0x1000;
            info.szName = threadName;
            info.dwThreadID = dwThreadID;
            info.dwFlags = 0;

            __try
            {
                RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }
#endif
    }
#endif

    void set_this_thread_name(const std::string& _name)
    {
#if defined(_WIN32)
        SetThreadName(GetCurrentThreadId(), _name.c_str());
#elif defined(__linux__)
        std::array<char, 16> thread_name;

        std::snprintf(thread_name.data(), thread_name.size(), "%s", _name.c_str());
        thread_name.back() = '\0';

        pthread_setname_np(pthread_self(), thread_name.data());
#else
        pthread_setname_np(_name.c_str());
#endif
    }

    std::string from_utf16(const std::wstring& _source_16)
    {
#ifdef __linux__
        return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(_source_16.data(), _source_16.data() + _source_16.size());
#else
        return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(_source_16.data(), _source_16.data() + _source_16.size());
#endif
    }

    std::wstring from_utf8(const std::string& _source_8)
    {
#ifdef __linux__
        return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(_source_8.data(), _source_8.data() + _source_8.size());
#else
        return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().from_bytes(_source_8.data(), _source_8.data() + _source_8.size());
#endif
    }

    std::ifstream open_file_for_read(const std::wstring& _file_name, std::ios_base::openmode _mode)
    {
#ifdef _WIN32
        return std::ifstream(_file_name, _mode);
#else
        return std::ifstream(tools::from_utf16(_file_name), _mode);
#endif
    }

    std::ofstream open_file_for_write(const std::wstring& _file_name, std::ios_base::openmode _mode)
    {
#ifdef _WIN32
        return std::ofstream(_file_name, _mode);
#else
        return std::ofstream(tools::from_utf16(_file_name), _mode);
#endif
    }

    bool is_exist(const std::wstring& _path)
    {
#ifdef _WIN32
        return _waccess_s(_path.c_str(), 0) == 0;
#else
        return access(tools::from_utf16(_path).c_str(), 0) == 0;
#endif
    }

    bool create_parent_directories_for_file(const std::wstring& _file_name)
    {
        size_t pos, pre_pos = 0;
#ifdef _WIN32
        // skip disk lable if it exist 'C:\...'
        pos = _file_name.find(L':');
        if (pos != std::wstring::npos)
            pre_pos = pos + 2;
#endif
        while ((pos=_file_name.find_first_of(L"/\\", pre_pos)) != std::wstring::npos)
        {
            auto dir = _file_name.substr(0, pos);
            pre_pos = ++pos;
            if (dir.length() > 0 && !tools::is_exist(dir))
            {
#ifdef _WIN32
                auto result = _wmkdir(dir.c_str());
#else
                auto result = mkdir(tools::from_utf16(dir).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
                if (result != 0)
                    return false;
            }
        }

        return true;
    }

    bool rename_file(const std::wstring& _path_from, const std::wstring& _path_to)
    {
#ifndef _WIN32
        auto path_from = tools::from_utf16(_path_from);
        auto path_to = tools::from_utf16(_path_to);
#endif
        int result = 0;

        if (is_exist(_path_to))
#ifdef _WIN32
            result = _wremove(_path_to.c_str());
#else
            result = std::remove(path_to.c_str());
#endif

        if (result == 0)
        {
#ifdef _WIN32
            result = _wrename(_path_from.c_str(), _path_to.c_str());
#else
            result = std::rename(path_from.c_str(), path_to.c_str());
#endif
            if (result == 0)
                return true;
        }

        return false;
    }
}}
