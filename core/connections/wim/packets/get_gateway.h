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
        class get_gateway : public wim_packet
        {
            std::string		file_name_;
            int64_t			file_size_;
            std::string		host_;
            std::string		url_;
            std::optional<int64_t> duration_;
            std::optional<file_sharing_base_content_type> base_content_type_;
            std::optional<std::string> locale_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;
            int32_t on_http_client_error() override;

        public:

            get_gateway(const wim_packet_params& _params,
                const std::string& _file_name,
                int64_t _file_size,
                const std::optional<int64_t>& _duration,
                const std::optional<file_sharing_base_content_type>& _base_content_type,
                const std::optional<std::string>& _locale);
            virtual ~get_gateway();

            const std::string& get_host() const noexcept { return host_; }
            const std::string& get_url() const noexcept { return url_; }
            std::string_view get_method() const override;

            int minimal_supported_api_version() const override;
        };
    }
}
