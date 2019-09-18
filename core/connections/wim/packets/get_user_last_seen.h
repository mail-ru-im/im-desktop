#pragma once

#include "../robusto_packet.h"
#include "../chat_info.h"

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
        class get_user_last_seen: public robusto_packet
        {

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;

            std::map<std::string, time_t> result_;
            std::vector<std::string> aimids_;

        public:
            get_user_last_seen(wim_packet_params _params, const std::vector<std::string>& _aimids);

            virtual ~get_user_last_seen();

            const std::map<std::string, time_t>& get_result() const;
        };

    }

}