#ifndef __WIM_SET_STATE_H_
#define __WIM_SET_STATE_H_

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
        class set_state : public wim_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response_data(const rapidjson::Value& _data) override;

            const profile_state	state_;

        public:
            set_state(wim_packet_params _params, const profile_state _state);
            virtual ~set_state();

            priority_t get_priority() const override;

            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };

    }

}

#endif // __WIM_SET_STATE_H_
