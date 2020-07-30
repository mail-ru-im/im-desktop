#pragma once

#include "../robusto_packet.h"

namespace core::tools
{
    class http_request_simple;
}

namespace core::wim
{
    class reset_session : public robusto_packet
    {
    public:
        reset_session(wim_packet_params _params, std::string_view _hash);

        const std::string& get_hash() const { return hash_; }

    private:
        int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
        int32_t on_response_error_code() override;

        bool is_reset_all() const { return hash_.empty(); }

        std::string hash_;
    };
}