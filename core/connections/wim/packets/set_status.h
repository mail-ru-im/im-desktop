#pragma once

#include "../robusto_packet.h"

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
        class set_status : public robusto_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

            std::string status_;
            std::string description_;
            std::chrono::seconds duration_;

        public:
            set_status(wim_packet_params _params, std::string_view _status, std::chrono::seconds _duration, std::string_view _description);
            virtual ~set_status() = default;

            virtual std::string_view get_method() const override;
        };
    }
}
