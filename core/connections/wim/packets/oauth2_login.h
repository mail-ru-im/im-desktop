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
        class oauth2_login : public wim_packet
        {
            std::string authcode_;
            std::string o2auth_token_;
            std::string o2refresh_token_;
            uint32_t expired_in_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t on_response_error_code() override;
            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& response) override;
            int32_t on_http_client_error() override;

        public:
            oauth2_login(wim_packet_params _params, std::string_view _authcode);
            oauth2_login(wim_packet_params _params);
            virtual ~oauth2_login();

            const std::string& get_o2auth_token() const { return o2auth_token_; }
            const std::string& get_o2refresh_token() const { return o2refresh_token_; }
            const uint32_t get_expired_in() const { return expired_in_; }
            bool is_valid() const override { return true; }
            bool is_post() const override { return true; }
            std::string_view get_method() const override;
        };
    }
}
