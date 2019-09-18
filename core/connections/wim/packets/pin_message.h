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
        class pin_message : public robusto_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string aimid_;
            int64_t msg_id_;
            bool unpin_;

        public:
            pin_message(wim_packet_params _params, const std::string& _aimId, const int64_t _message, const bool _is_unpin);
        };
    }
}
