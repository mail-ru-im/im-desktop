#pragma once

#include "../robusto_packet.h"
#include "../persons_packet.h"
#include "../search_contacts_response.h"

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
        class search_contacts: public robusto_packet, public persons_packet
        {
            std::string keyword_;
            std::string phone_;

            int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;

        public:
            search_contacts_response response_;

            search_contacts(wim_packet_params _packet_params, const std::string_view _keyword, const std::string_view _phone);

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override;

            bool support_async_execution() const override { return true; }
        };
    }
}
