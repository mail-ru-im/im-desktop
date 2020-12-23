#pragma once

#include "../robusto_packet.h"

namespace core
{
    struct lastseen;
    namespace tools
    {
        class http_request_simple;
    }

    namespace wim
    {
        class get_user_last_seen: public robusto_packet
        {

            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;

            std::map<std::string, lastseen> result_;
            std::vector<std::string> aimids_;

        public:
            get_user_last_seen(wim_packet_params _params, std::vector<std::string> _aimids);

            virtual ~get_user_last_seen();

            const std::map<std::string, lastseen>& get_result() const;

            virtual std::string_view get_method() const override;
        };
    }
}