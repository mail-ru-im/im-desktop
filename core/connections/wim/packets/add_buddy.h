#ifndef __WIM_ADD_BUDDY_H_
#define __WIM_ADD_BUDDY_H_

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
        class add_buddy : public wim_packet
        {
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string		aimid_;
            std::string		group_;
            std::string		auth_message_;

        public:

            add_buddy(
                wim_packet_params _params,
                const std::string& _aimid,
                const std::string& _group,
                const std::string& _auth_message);

            virtual ~add_buddy();

            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };

    }

}


#endif// __WIM_ADD_BUDDY_H_