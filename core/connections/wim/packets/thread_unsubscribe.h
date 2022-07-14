#pragma once

#include "../robusto_packet.h"

namespace core::wim
{
    class thread_unsubscribe : public robusto_packet
    {
    public:
        thread_unsubscribe(wim_packet_params _params, std::string_view _thread_id);
        std::string_view get_method() const override;
        int minimal_supported_api_version() const override;

        const std::string& get_thread_id() const { return thread_id_; }

    private:
        int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
        int32_t parse_results(const rapidjson::Value& _node_results) override;

    private:
        std::string thread_id_;
    };
}
