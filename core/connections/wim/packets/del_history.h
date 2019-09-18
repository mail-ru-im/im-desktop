#pragma once

#include "../../../namespaces.h"

#include "../robusto_packet.h"

CORE_WIM_NS_BEGIN

class del_history: public robusto_packet
{
public:
    del_history(
        wim_packet_params _params,
        const int64_t _up_to_id,
        const std::string &_contact_aimid
    );

private:
    const int64_t up_to_id_;

    const std::string contact_aimid_;

    virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

    virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

    virtual int32_t on_response_error_code() override;

};

CORE_WIM_NS_END