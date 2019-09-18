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
        class resolve_pending : public robusto_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string aimid_;
            std::vector<std::string> contacts_;
            bool approve_;

        public:

            resolve_pending(wim_packet_params _params, const std::string& _aimId, const std::vector<std::string>& _contacts, bool _approve);
            virtual ~resolve_pending();
        };

    }

}
