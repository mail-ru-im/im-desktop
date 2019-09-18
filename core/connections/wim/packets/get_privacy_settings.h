#pragma once

#include "../robusto_packet.h"
#include "../privacy_settings.h"

namespace core::wim
{
    class get_privacy_settings : public robusto_packet
    {
    public:
        get_privacy_settings(wim_packet_params _params);
        const privacy_settings& get_settings() const;

    private:
        int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
        int32_t parse_results(const rapidjson::Value& _data) override;

        privacy_settings settings_;
    };
}