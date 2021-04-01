#include "stdafx.h"

#include "log_helpers.h"

#include "../../core.h"
#include "archive/archive_index.h"

namespace core
{
    namespace wim
    {
        void write_offset_in_log(time_t offset)
        {
            tools::binary_stream bs;
            bs.write<std::string_view>("new offset is ");
            bs.write<std::string_view>(std::to_string(offset));
            bs.write<std::string_view>("\r\n");

            g_core->write_data_to_network_log(std::move(bs));
        }

        void write_ignore_download_holes(std::string_view contact, std::string_view reason, std::optional<archive::archive_hole_error> _error, const std::optional<holes::request>& _request)
        {
            std::stringstream s;
            s << "ignore download holes for " << contact << ": " << reason << ";";
            if (_error)
                s << " error: " << int(*_error) << ";";
            if (_request)
                s << " from: " << _request->get_from() << "; depth: " << _request->get_depth() << "; " << "recursion: " << _request->get_recursion();
            s << "\r\n";

            tools::binary_stream bs;
            bs.write<std::string_view>(s.str());

            g_core->write_data_to_network_log(std::move(bs));
        }

        void write_time_offests_in_log(const std::shared_ptr<fetch>& packet)
        {
            std::stringstream s;
            s << "fetch packet: start execute: " << packet->get_start_execute_time()
                << "; now: " << packet->now() << "; request_time: " << packet->get_request_time()
                << "; timezone_offset: " << packet->get_timezone_offset()
                << "; time_offset: " << packet->get_time_offset()
                << "; time_offset_local: " << packet->get_time_offset_local();
            s << "\r\n";

            tools::binary_stream bs;
            bs.write<std::string_view>(s.str());

            g_core->write_data_to_network_log(std::move(bs));
        }

        void write_files_error_in_log(std::string_view _func, int32_t _error)
        {
            tools::binary_stream bs;
            bs.write<std::string_view>(_func);
            std::stringstream s;
            s << " error is ";
            s << _error;
            bs.write<std::string_view>(s.str());
            bs.write<std::string_view>("\r\n");

            g_core->write_data_to_network_log(std::move(bs));
        }

        void write_files_error_in_log(std::string_view _func, const archive::error_vector& _errors)
        {
            tools::binary_stream bs;
            bs.write<std::string_view>(_func);
            bs.write<std::string_view>(" errors: ");
            std::stringstream s;
            for (auto [id, error] : _errors)
                s << "id: " << id << " error: " << error << "; ";

            bs.write<std::string_view>(s.str());
            bs.write<std::string_view>("\r\n");

            g_core->write_data_to_network_log(std::move(bs));
        }

        void write_add_opened_dialog(std::string_view contact)
        {
            tools::binary_stream bs;
            bs.write<std::string_view>(contact);
            bs.write<std::string_view>(" has been added to opened dialogs");
            bs.write<std::string_view>("\r\n");

            g_core->write_data_to_network_log(std::move(bs));
        }

        void write_remove_opened_dialog(std::string_view contact)
        {
            tools::binary_stream bs;
            bs.write<std::string_view>(contact);
            bs.write<std::string_view>(" has been removed from opened dialogs");
            bs.write<std::string_view>("\r\n");

            g_core->write_data_to_network_log(std::move(bs));
        }

        void write_log_for_contact(std::string_view contact, std::string_view log)
        {
            tools::binary_stream bs;
            bs.write<std::string_view>(contact);
            bs.write<std::string_view>(": ");
            bs.write<std::string_view>(log);
            bs.write<std::string_view>("\r\n");

            g_core->write_data_to_network_log(std::move(bs));
        }
    }
}