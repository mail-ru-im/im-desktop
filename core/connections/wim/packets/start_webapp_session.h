#pragma once

#include "../robusto_packet.h"

namespace core::wim
{
    class start_webapp_session : public robusto_packet
    {
    public:
        start_webapp_session(wim_packet_params _params, std::string_view _miniapp_id);
        std::string_view get_method() const override;

        const std::string& get_miniapp_id() const noexcept { return id_; }
        const std::string& get_miniapp_aimsid() const noexcept { return aimsid_; }

    private:
        int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
        int32_t parse_results(const rapidjson::Value& _node_results) override;

    private:
        std::string id_;
        std::string aimsid_;
    };
}