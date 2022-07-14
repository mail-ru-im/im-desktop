#pragma once

#include "../robusto_packet.h"
#include "../session_info.h"

namespace core::tools
{
    class http_request_simple;
}

namespace core::wim
{
    class get_sessions_list : public robusto_packet
    {
    public:
        get_sessions_list(wim_packet_params _params);

        const std::vector<session_info>& get_sessions() const noexcept { return sessions_; }
        std::string_view get_method() const override;
        int minimal_supported_api_version() const override;

    private:
        int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
        int32_t parse_results(const rapidjson::Value& _node_results) override;

        std::vector<session_info> sessions_;
    };
}
