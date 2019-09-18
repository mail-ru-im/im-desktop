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
        class check_nick : public robusto_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t on_response_error_code() override;

            std::string nickname_;
            bool set_nick_;

        public:
            check_nick(wim_packet_params _params, const std::string& _nick, bool _set_nick);
        };
    }
}
