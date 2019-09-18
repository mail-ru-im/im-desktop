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
        class block_chat_member : public robusto_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string aimid_;
            std::string contact_;
            bool block_;

        public:

            block_chat_member(wim_packet_params _params, const std::string& _aimId, const std::string& _contact, bool _block);
            virtual ~block_chat_member();
        };

    }

}
