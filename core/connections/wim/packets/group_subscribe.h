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
        class group_subscribe : public robusto_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;

            std::string stamp_;
            int32_t resubscribe_in_;

        public:
            group_subscribe(wim_packet_params _params, const std::string& _stamp);
            int32_t get_resubscribe_in() const;
        };
    }
}
