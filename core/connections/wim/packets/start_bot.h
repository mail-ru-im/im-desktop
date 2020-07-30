#pragma once

#include "../robusto_packet.h"

namespace core::wim
{
    class start_bot : public robusto_packet
    {
    public:
        start_bot(wim_packet_params _params, std::string_view _nick, std::string_view _payload);

    private:
        int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

    private:
        std::string nick_;
        std::string payload_;
    };
}