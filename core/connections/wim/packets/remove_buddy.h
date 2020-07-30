#ifndef __WIM_REMOVE_BUDDY_H_
#define __WIM_REMOVE_BUDDY_H_

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
        class remove_buddy : public wim_packet
        {
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string		aimid_;

        public:

            remove_buddy(
                wim_packet_params _params,
                const std::string& _aimid);

            virtual ~remove_buddy();
        };

    }

}


#endif// __WIM_REMOVE_BUDDY_H_