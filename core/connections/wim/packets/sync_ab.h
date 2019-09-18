#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace wim
    {
        class sync_ab : public robusto_packet
        {
            std::string keyword_;
            std::vector<std::string> phone_;
            int32_t status_code_;

            int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;
            int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;

        public:
            sync_ab(wim_packet_params _packet_params, const std::string& _keyword, const std::vector<std::string>& _phone);
            int32_t get_status_code();
        };
    }
}
