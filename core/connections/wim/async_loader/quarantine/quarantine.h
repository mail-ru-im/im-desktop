#pragma once

namespace core::wim::quarantine
{
    struct quarantine_params
    {
        std::wstring_view file_name;
        std::string_view source_url;
        std::string_view referrer_url;
    };

    enum class quarantine_result
    {
        ok,             // Success.
        access_denied,  // Access to the file was denied. The safety of the file could
        // not be determined.
        blocked_by_policy,  // Downloads from |source_url| are not allowed by policy.
        // The file has been deleted.
        annotation_failed,  // Unable to write the mark-of-the-web or otherwise
        // annotate the file as being downloaded from
        // |source_url|.
        file_missing,       // |file| does not name a valid file.
        security_check_failed,  // An unknown error occurred while checking |file|.
        // The file may have been deleted.
        virus_infected  // |file| was found to be infected by a virus and was deleted.
    };

    quarantine_result quarantine_file(const quarantine_params& _params);
}