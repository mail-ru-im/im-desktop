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
        class get_chat_home: public robusto_packet
        {

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;
            std::string new_tag_;

        public:
            std::vector<chat_info> result_;
            std::string result_tag_;
            bool need_restart_;
            bool finished_;

            get_chat_home(wim_packet_params _params, const std::string& _new_tag = std::string());

            virtual ~get_chat_home();
        };

    }

}