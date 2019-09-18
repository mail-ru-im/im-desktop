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

            int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;

        private:
            const std::string contact_;
            const bool value_;
        };
    }
}
