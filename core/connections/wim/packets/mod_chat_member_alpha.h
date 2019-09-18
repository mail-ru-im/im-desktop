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
        class mod_chat_member_alpha : public robusto_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string aimid_;
            std::string contact_;
            std::string role_;

        public:

            mod_chat_member_alpha(wim_packet_params _params, const std::string& _aimId, const std::string& _contact, const std::string& _role);
            virtual ~mod_chat_member_alpha();
        };

    }

}
