#pragma once

#include "../robusto_packet.h"
#include "../privacy_settings.h"

namespace core::wim
{
    class set_privacy_settings : public robusto_packet
    {
    public:
        set_privacy_settings(wim_packet_params _params, privacy_settings _settings);

    private:
        int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

        privacy_settings settings_;
    };
}