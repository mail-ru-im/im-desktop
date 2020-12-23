#pragma once

#include "../../../namespaces.h"

#include "../robusto_packet.h"

CORE_WIM_NS_BEGIN

class del_message_batch : public robusto_packet
{
public:
    del_message_batch(
        wim_packet_params _params,
        std::vector<int64_t>&& _message_ids,
        const std::string &_contact_aimid,
        const bool _for_all,
        const bool _silent
    );

    virtual priority_t get_priority() const override { return priority_protocol(); }

    virtual bool support_self_resending() const override { return true; }

    virtual std::string_view get_method() const override;

    bool is_silent() const { return silent_responce_; }

private:
    std::vector<int64_t> message_ids_;

    const std::string contact_aimid_;

    const bool for_all_;

    const bool silent_;

    bool silent_responce_;

    virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

    virtual int32_t parse_results(const rapidjson::Value& _data) override;

    virtual int32_t on_response_error_code() override;
};

CORE_WIM_NS_END