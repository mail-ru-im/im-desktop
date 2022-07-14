#pragma once

#include "../robusto_packet.h"
#include "../privacy_settings.h"

namespace core::wim
{
    class set_privacy_settings : public robusto_packet
    {
    public:
        set_privacy_settings(wim_packet_params _params, privacy_settings _settings);
        std::string_view get_method() const override;
        int minimal_supported_api_version() const override;

    private:
        int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        privacy_settings settings_;
    };
}