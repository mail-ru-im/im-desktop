#pragma once

#include "../../../namespaces.h"

#include "../robusto_packet.h"

CORE_WIM_NS_BEGIN

class del_message : public robusto_packet
{
public:
    del_message(
        wim_packet_params _params,
        const int64_t _message_id,
        const std::string &_contact_aimid,
        const bool _for_all
    );

private:
    const int64_t message_id_;

    const std::string contact_aimid_;

    const bool for_all_;

    virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

    virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

    virtual int32_t on_response_error_code() override;

};

CORE_WIM_NS_END