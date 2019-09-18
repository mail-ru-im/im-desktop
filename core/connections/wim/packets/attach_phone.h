#ifndef __WIM_ATTACH_PHONE_H_
#define __WIM_ATTACH_PHONE_H_

#pragma once

#include "../../login_info.h"
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
        class attach_phone : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t on_response_error_code() override;
            int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;
            virtual int32_t on_empty_data() override;

            phone_info      phone_info_;
        public:

            attach_phone(wim_packet_params _params, const phone_info& _info);
            int32_t get_response_error_code();
            virtual ~attach_phone();
        };

    }

}


#endif// __WIM_ATTACH_PHONE_H_
