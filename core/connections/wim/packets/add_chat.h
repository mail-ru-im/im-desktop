#ifndef __WIM_ADD_CHAT_H_
#define __WIM_ADD_CHAT_H_

#pragma once

#include "../wim_packet.h"

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
        class add_chat : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string m_chat_name;
            std::vector<std::string> m_chat_members;

        public:

            add_chat(
                wim_packet_params _params,
                const std::string& _m_chat_name,
                std::vector<std::string> _m_chat_members);

            int32_t get_members_count() const;

            virtual ~add_chat();
        };

    }

}


#endif// __WIM_ADD_CHAT_H_
