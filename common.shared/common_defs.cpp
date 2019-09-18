#include "stdafx.h"

#ifdef _WIN32
#include <Windows.h>
#include <shlobj.h>
#endif // _WIN32

#ifdef _WIN32
namespace common
{
    std::wstring get_user_profile()
    {
        wchar_t fullPath[MAX_PATH + 1];

        const auto error = ::SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, fullPath);
        if (FAILED(error))
        {
            return std::wstring();
        }

        wchar_t shortPath[MAX_PATH + 1];
        GetShortPathNameW(fullPath, shortPath, MAX_PATH);

        return shortPath;
    }
}
#endif // _WIN32

namespace common
{
    uint32_t get_limit_search_results()
    {
        return 100;
    }
}
