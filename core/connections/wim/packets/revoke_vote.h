#pragma once

#include "../robusto_packet.h"

namespace core::wim
{

class revoke_vote : public robusto_packet
{
public:
    revoke_vote(wim_packet_params _params, const std::string& _poll_id);
    virtual std::string_view get_method() const override;

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

    std::string poll_id_;
};

}
