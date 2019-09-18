#ifndef __WIM_MODIFY_CHAT_H_
#define __WIM_MODIFY_CHAT_H_

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
        class modify_chat : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string	m_chat_name;
            std::string	aimid_;

        public:

            modify_chat(
                wim_packet_params _params,
                const std::string& _aimid_,
                const std::string& _m_chat_name);

            virtual ~modify_chat();
        };

    }

}


#endif// __WIM_MODIFY_CHAT_H_
