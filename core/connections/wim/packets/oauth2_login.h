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
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t init_oauth2_request(const std::shared_ptr<core::http_request_simple>& _request);
            int32_t init_keycloak_request(const std::shared_ptr<core::http_request_simple>& _request);
            void mask_request(const std::shared_ptr<core::http_request_simple>& _request, const std::vector<std::string_view>& _keys);
            int32_t on_response_error_code() override;
            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& response) override;
            int32_t on_http_client_error() override;

        public:
            enum class oauth2_type
            {
                oauth2,
                keycloak
            };
            oauth2_login(wim_packet_params _params, std::string_view _authcode, oauth2_type _type);
            oauth2_login(wim_packet_params _params);
            virtual ~oauth2_login();

            static oauth2_type get_auth_type(std::string_view _type);

            const std::string& get_o2auth_token() const { return o2auth_token_; }
            const std::string& get_o2refresh_token() const { return o2refresh_token_; }
            const uint32_t get_expired_in() const { return expired_in_; }
            bool is_valid() const override { return true; }
            bool is_post() const override { return true; }
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;

        private:
            std::string authcode_;
            std::string o2auth_token_;
            std::string o2refresh_token_;
            uint32_t expired_in_;
            oauth2_type auth_type_;
        };
    }
}
