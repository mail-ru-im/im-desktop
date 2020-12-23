#ifndef __REQUEST_AVATAR_H_
#define __REQUEST_AVATAR_H_

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
        class request_avatar : public wim_packet
        {
            const std::string_view avatar_type_;
            const std::string contact_;
            std::shared_ptr<core::tools::binary_stream> data_;
            time_t write_time_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t execute_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;

        public:

            const std::shared_ptr<core::tools::binary_stream>& get_data() const;

            request_avatar(
                wim_packet_params params,
                const std::string& _contact,
                std::string_view _avatar_type,
                time_t _write_time = 0);
            virtual ~request_avatar();

            virtual priority_t get_priority() const override;
            virtual std::string_view get_method() const override;
        };
    }
}


#endif //__REQUEST_AVATAR_H_
