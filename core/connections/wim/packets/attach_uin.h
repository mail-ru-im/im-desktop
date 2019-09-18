#ifndef __WIM_ATTACH_UIN_H_
#define __WIM_ATTACH_UIN_H_

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
        class attach_uin : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;
            virtual int32_t on_empty_data() override;

            wim_packet_params from_params_;

        public:

            attach_uin(wim_packet_params _from_params, wim_packet_params _to_params);

            virtual ~attach_uin();
        };

    }

}


#endif// __WIM_ATTACH_UIN_H_
