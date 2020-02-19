#include "stdafx.h"

#include "config/config.h"

#ifdef _WIN32
#include <Windows.h>
#include <shlobj.h>
#include <lm.h>
#include "common_defs.h"
#pragma comment(lib, "netapi32.lib")
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

    namespace
    {
        std::wstringstream& operator<<(std::wstringstream& os, REFGUID guid) {

            os << std::uppercase;
            os.width(8);
            os << std::hex << guid.Data1 << '-';

            os.width(4);
            os << std::hex << guid.Data2 << '-';

            os.width(4);
            os << std::hex << guid.Data3 << '-';

            os.width(2);
            os << std::hex
                << static_cast<short>(guid.Data4[0])
                << static_cast<short>(guid.Data4[1])
                << '-'
                << static_cast<short>(guid.Data4[2])
                << static_cast<short>(guid.Data4[3])
                << static_cast<short>(guid.Data4[4])
                << static_cast<short>(guid.Data4[5])
                << static_cast<short>(guid.Data4[6])
                << static_cast<short>(guid.Data4[7]);
            os << std::nouppercase;
            return os;
        }
    }

    std::wstring get_guid()
    {
        GUID guid;
        if (CoCreateGuid(&guid) != S_OK)
            return {};

        std::wstringstream ss;
        ss << guid;
        return ss.str();
    }

    namespace
    {
        bool GetWinMajorMinorVersion(DWORD& major, DWORD& minor)
        {
            bool bRetCode = false;
            LPBYTE pinfoRawData = 0;
            if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData))
            {
                WKSTA_INFO_100* pworkstationInfo = (WKSTA_INFO_100*)pinfoRawData;
                major = pworkstationInfo->wki100_ver_major;
                minor = pworkstationInfo->wki100_ver_minor;
                ::NetApiBufferFree(pinfoRawData);
                bRetCode = true;
            }
            return bRetCode;
        }
    }

    std::string get_win_os_version_string()
    {
        std::string     winver;
        OSVERSIONINFOEX osver;
        SYSTEM_INFO     sysInfo;
        typedef void(__stdcall *GETSYSTEMINFO) (LPSYSTEM_INFO);

        __pragma(warning(push))
            __pragma(warning(disable:4996))
            memset(&osver, 0, sizeof(osver));
        osver.dwOSVersionInfoSize = sizeof(osver);
        auto getResult = GetVersionEx((LPOSVERSIONINFO)&osver);
        __pragma(warning(pop))
            if (getResult == 0)
                return "unknown";

        DWORD major = 0;
        DWORD minor = 0;
        if (GetWinMajorMinorVersion(major, minor))
        {
            osver.dwMajorVersion = major;
            osver.dwMinorVersion = minor;
        }
        else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2)
        {
            OSVERSIONINFOEXW osvi;
            ULONGLONG cm = 0;
            cm = VerSetConditionMask(cm, VER_MINORVERSION, VER_EQUAL);
            ZeroMemory(&osvi, sizeof(osvi));
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            osvi.dwMinorVersion = 3;
            if (VerifyVersionInfoW(&osvi, VER_MINORVERSION, cm))
            {
                osver.dwMinorVersion = 3;
            }
        }

        GETSYSTEMINFO getSysInfo = (GETSYSTEMINFO)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo");
        if (getSysInfo == NULL)
            getSysInfo = ::GetSystemInfo;
        getSysInfo(&sysInfo);

        if (osver.dwMajorVersion == 10 && osver.dwMinorVersion >= 0 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows 10 Server";
        if (osver.dwMajorVersion == 10 && osver.dwMinorVersion >= 0 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 10";
        if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012 R2";
        if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8.1";
        if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012";
        if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8";
        if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008 R2";
        if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 7";
        if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008";
        if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows Vista";
        if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION
            && sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)  winver = "Windows XP x64";
        if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2)   winver = "Windows Server 2003";
        if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1)   winver = "Windows XP";
        if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0)   winver = "Windows 2000";
        if (osver.dwMajorVersion < 5)   winver = "unknown";

        if (osver.wServicePackMajor != 0)
        {
            std::string sp;
            char buf[128] = { 0 };
            sp = " Service Pack ";
            sprintf_s(buf, sizeof(buf), "%d", osver.wServicePackMajor);
            sp.append(buf);
            winver += sp;
        }

        return winver;
    }

    namespace
    {
        uint16_t getVolumeHash()
        {
            DWORD serialNum = 0;

            // Determine if this volume uses an NTFS file system.
            GetVolumeInformationA("c:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0);
            uint16_t hash = (uint16_t)((serialNum + (serialNum >> 16)) & 0xFFFF);

            return hash;
        }

        uint16_t getCpuHash()
        {
            int cpuinfo[4] = { 0, 0, 0, 0 };
            __cpuid(cpuinfo, 0);
            uint16_t hash = 0;
            uint16_t* ptr = (uint16_t*)(&cpuinfo[0]);
            for (size_t i = 0; i < 8; ++i)
                hash += ptr[i];

            return hash;
        }

        const char* getMachineName()
        {
            static char computerName[1024];
            DWORD size = 1024;
            GetComputerNameA(computerName, &size);
            return &(computerName[0]);
        }
    }

    std::string get_win_device_id()
    {
        std::string device_id;
        device_id += std::to_string(getVolumeHash());
        device_id += std::to_string(getCpuHash());
        device_id += getMachineName();

        return device_id;
    }
}
#endif // _WIN32

namespace common
{
    uint32_t get_limit_search_results()
    {
        return 100;
    }

    std::string_view get_dev_id()
    {
        constexpr auto key = platform::is_apple()
            ? config::values::dev_id_mac : (platform::is_windows() ? config::values::dev_id_win : config::values::dev_id_linux);
        return config::get().string(key);
    }
}
