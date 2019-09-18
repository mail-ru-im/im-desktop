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
        class check_stickers_new final : public wim_packet
        {
        private:

            std::shared_ptr<core::tools::binary_stream> response_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;

        public:

            bool support_async_execution() const override;

            check_stickers_new(wim_packet_params _params);
            virtual ~check_stickers_new();

            std::shared_ptr<core::tools::binary_stream> get_response();
        };
    }
}