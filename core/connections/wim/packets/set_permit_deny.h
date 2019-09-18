#ifndef __WIM_SET_PERMIT_DENY_H_
#define __WIM_SET_PERMIT_DENY_H_

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
        class set_permit_deny : public wim_packet
        {
        public:
            enum class operation
            {
                invalid,
                min,

                ignore,
                ignore_remove,

                max
            };

        private:

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            const std::string aimid_;
            const set_permit_deny::operation op_;

        public:

            set_permit_deny(
                wim_packet_params _params,
                const std::string& _aimid,
                const set_permit_deny::operation _op);

            virtual ~set_permit_deny();
        };

    }

}


#endif// __WIM_SET_PERMIT_DENY_H_