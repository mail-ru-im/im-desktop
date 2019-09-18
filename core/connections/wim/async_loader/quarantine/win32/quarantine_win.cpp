#include "stdafx.h"

#include "../quarantine.h"

#include <Windows.h>

#include "tools/system.h"

namespace
{
    constexpr std::wstring_view zone_identifier_stream_suffix() noexcept
    {
        return L":Zone.Identifier";
    }

    constexpr bool is_enable_extended_info() noexcept
    {
        return true;
    }
}

namespace core::wim::quarantine
{
    quarantine_result quarantine_file(const quarantine_params& _params)
    {
        std::wstring path;
        path += _params.file_name;
        if (!tools::system::is_exist(path))
            return quarantine_result::file_missing;

        path += zone_identifier_stream_suffix();
        const DWORD k_share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

        const auto h_result = ::CreateFile(path.c_str(), GENERIC_WRITE, k_share, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h_result == INVALID_HANDLE_VALUE)
            return quarantine_result::annotation_failed;

        std::string identifier;
        identifier += "[ZoneTransfer]\r\nZoneId=3\r\n";

        if constexpr (is_enable_extended_info())
        {
            if (!_params.referrer_url.empty())
            {
                identifier += "ReferrerUrl=";
                identifier += _params.referrer_url;
                identifier += "\r\n";
            }

            identifier += "HostUrl=";
            identifier += _params.source_url.empty() ? "about:internet" : _params.source_url;
            identifier += "\r\n";
        }

        DWORD written = 0;
        const BOOL write_result = ::WriteFile(h_result, identifier.data(),
            DWORD(identifier.size()), &written, nullptr);
        const BOOL flush_result = ::FlushFileBuffers(h_result);

        if (h_result)
            CloseHandle(h_result);

        return write_result && flush_result && written == DWORD(identifier.size())
            ? quarantine_result::ok
            : quarantine_result::annotation_failed;
    }
}