#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace wim
    {
        class misc_support : public robusto_packet
        {
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

            std::string aimid_;
            std::string message_;
            std::string attachment_file_id_;
            std::vector<std::string> screenshot_file_id_list_;

        public:
            misc_support(wim_packet_params _params, std::string_view _aimid, std::string_view _message, std::string_view _attachment_file_id, std::vector<std::string> _screenshot_file_id_list);
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };
    }
}
