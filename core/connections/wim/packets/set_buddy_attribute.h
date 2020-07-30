#ifndef __WIM_SET_BUDDY_ATTRIBUTE_H_
#define __WIM_SET_BUDDY_ATTRIBUTE_H_

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
        class set_buddy_attribute : public wim_packet
        {
            const std::string friendly_;
            const std::string aimid_;

            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

        public:

            set_buddy_attribute(
                wim_packet_params _params,
                const std::string& _aimid,
                const std::string& _friendly);

            virtual ~set_buddy_attribute();
        };

    }

}


#endif// __WIM_SET_BUDDY_ATTRIBUTE_H_
