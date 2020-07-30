#pragma once

#include "packets/fetch.h"
#include "../../archive/local_history.h"

namespace core
{
    namespace wim
    {
        void write_offset_in_log(time_t offset);
        void write_ignore_download_holes(std::string_view contact, std::string_view reason);
        class fetch;
        void write_time_offests_in_log(const std::shared_ptr<fetch>& packet);
        void write_files_error_in_log(std::string_view _func, int32_t _error);
        void write_files_error_in_log(std::string_view _func, const archive::error_vector& _errors);
        void write_add_opened_dialog(std::string_view contact);
        void write_remove_opened_dialog(std::string_view contact);
        void write_log_for_contact(std::string_view contact, std::string_view log);
    }
}