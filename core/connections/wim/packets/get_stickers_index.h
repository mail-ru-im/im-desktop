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
    namespace tools
    {
        class binary_stream;
    }

    namespace wim
    {
        class get_stickers_index : public wim_packet
        {
            std::string request_etag_;
            std::string response_etag_;
            std::shared_ptr<core::tools::binary_stream> response_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;

        public:

            get_stickers_index(wim_packet_params _params, std::string_view _etag);
            virtual ~get_stickers_index();

            const std::shared_ptr<core::tools::binary_stream>& get_response() const noexcept { return response_; }
            const std::string& get_response_etag() const noexcept { return response_etag_; }
            const std::string& get_request_etag() const noexcept { return request_etag_; }

            priority_t get_priority() const override;
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
            bool support_self_resending() const override { return true; }
        };
    }
}
