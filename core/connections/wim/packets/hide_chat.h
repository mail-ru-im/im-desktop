#ifndef __WIM_HIDE_CHAT_H_
#define __WIM_HIDE_CHAT_H_

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
        class hide_chat : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t on_empty_data() override;

            std::string		aimid_;
            int64_t			last_msg_id_;

        public:

            hide_chat(wim_packet_params _params, const std::string& _aimid, int64_t _last_msg_id);
            virtual ~hide_chat();
        };

    }

}


#endif// __WIM_HIDE_CHAT_H_