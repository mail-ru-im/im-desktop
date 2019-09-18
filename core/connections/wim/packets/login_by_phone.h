#ifndef __LOGIN_BY_PHONE_H_
#define __LOGIN_BY_PHONE_H_

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
    class phone_info;

    namespace wim
    {
        class phone_login : public wim_packet
        {
            std::shared_ptr<phone_info>		info_;
            std::string						session_key_;
            std::string						a_token_;
            uint32_t						expired_in_;
            uint32_t						host_time_;
            int64_t							time_offset_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t on_response_error_code() override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t on_http_client_error() override;

        public:

            phone_login(
                wim_packet_params params,
                phone_info _info);

            virtual ~phone_login();

            const std::string& get_session_key() const { return session_key_; }
            const std::string& get_a_token() const { return a_token_; }
            const uint32_t get_expired_in() const { return expired_in_; }
            const uint32_t get_host_time() const { return host_time_; }
            const int64_t	get_time_offset() const { return time_offset_; }
        };
    }
}


#endif //__LOGIN_BY_PHONE_H_