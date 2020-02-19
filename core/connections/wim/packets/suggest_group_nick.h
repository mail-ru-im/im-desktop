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
        class suggest_group_nick : public robusto_packet
        {
        private:
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string	aimid_;
            std::string nick_;
            bool public_;

        public:
            suggest_group_nick(wim_packet_params _params, const std::string& _aimId, bool _public);
            virtual ~suggest_group_nick();

            std::string get_nick() const;
        };

    }

}
