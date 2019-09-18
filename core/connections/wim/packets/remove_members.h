#ifndef __WIM_REMOVE_MEMBERS_H_
#define __WIM_REMOVE_MEMBERS_H_

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
        class remove_members : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string		aimid_;
            std::string		m_chat_members_to_remove;

        public:

            remove_members(
                wim_packet_params _params,
                const std::string& _aimid,
                const std::string& _m_chat_members_to_remove);

            virtual ~remove_members();
        };

    }

}


#endif// __WIM_REMOVE_MEMBERS_H_
