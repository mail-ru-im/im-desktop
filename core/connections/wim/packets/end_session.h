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
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response_data(const rapidjson::Value& _data) override;
            int32_t execute_request(const std::shared_ptr<core::http_request_simple>& request) override;
        public:

            end_session(wim_packet_params _params);
            bool is_valid() const override { return true; }
            virtual ~end_session();
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
            bool support_self_resending() const override { return true; }
        };

    }

}

#endif // __WIM_END_SESSION_H_
