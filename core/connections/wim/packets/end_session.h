#ifndef __WIM_END_SESSION_H_
#define __WIM_END_SESSION_H_

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
    enum class profile_state;

    namespace wim
    {
        class end_session : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;
        public:

            end_session(wim_packet_params _params);
            virtual ~end_session();
        };

    }

}

#endif // __WIM_END_SESSION_H_
