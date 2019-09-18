#pragma once

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
        class stickers_migration : public wim_packet
        {
            std::vector<std::pair<int32_t, int32_t>> ids_;
            std::shared_ptr<core::tools::binary_stream> response_;

            int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;

        public:

            bool support_async_execution() const override;

            stickers_migration(wim_packet_params _params, std::vector<std::pair<int, int>> _ids);
            virtual ~stickers_migration();

            std::shared_ptr<core::tools::binary_stream> get_response() const;
        };
    }
}
