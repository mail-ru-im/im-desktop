#ifndef __WIM_ADD_MEMBERS_H_
#define __WIM_ADD_MEMBERS_H_

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
        class add_members : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t on_response_error_code() override;
            int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;

            std::string		aimid_;
            std::string		members_to_add_;

        public:

            add_members(
                wim_packet_params _params,
                const std::string& _aimid,
                const std::string & _members_to_add);

            virtual ~add_members();
        };

    }

}


#endif// __WIM_ADD_MEMBERS_H_
