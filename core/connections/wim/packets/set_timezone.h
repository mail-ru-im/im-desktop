#ifndef __WIM_SET_TIMEZONE_H_
#define __WIM_SET_TIMEZONE_H_

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
        class set_timezone : public wim_packet
        {
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t on_empty_data() override;

        public:

            explicit set_timezone(wim_packet_params _params);
            virtual ~set_timezone();
        };

    }

}


#endif// __WIM_SET_TIMEZONE_H_