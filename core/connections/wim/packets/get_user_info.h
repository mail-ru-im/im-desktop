#pragma once

#include "../../../namespaces.h"

#include "../robusto_packet.h"
#include "../persons_packet.h"
#include "../user_info.h"

CORE_WIM_NS_BEGIN

class get_user_info: public robusto_packet, public persons_packet
{
public:
    get_user_info(wim_packet_params _params, const std::string& _contact_aimid);
    user_info get_info() const;
    const std::shared_ptr<core::archive::persons_map>& get_persons() const override;

private:
    virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
    virtual int32_t parse_results(const rapidjson::Value& _data) override;

private:
    const std::string contact_aimid_;
    user_info info_;
    std::shared_ptr<core::archive::persons_map> persons_;
};

CORE_WIM_NS_END