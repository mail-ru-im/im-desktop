#pragma once

#include "../robusto_packet.h"
#include "archive/draft_storage.h"

namespace core::wim
{

class set_draft : public robusto_packet
{
public:
    set_draft(wim_packet_params _params, std::string_view _contact, const archive::draft& _draft);

    std::string_view get_method() const override;
    int minimal_supported_api_version() const override;
    bool auto_resend_on_fail() const override { return true; }

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
    int32_t parse_results(const rapidjson::Value& _node_results) override;

    std::string contact_;
    archive::draft draft_;
};

}

