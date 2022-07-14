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
        class set_attention_attribute : public wim_packet
        {
        public:
            set_attention_attribute(
                wim_packet_params _params,
                std::string _contact, bool _value);
            ~set_attention_attribute();

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;

        private:
            const std::string contact_;
            const bool value_;
        };
    }
}
