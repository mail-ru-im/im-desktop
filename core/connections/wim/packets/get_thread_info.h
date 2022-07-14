#pragma once

#include "../robusto_packet.h"
#include "../persons_packet.h"
#include "../thread_info.h"

namespace core::wim
{

struct get_thread_info_params
{
    std::string thread_id_;
};

class get_thread_info : public robusto_packet, public persons_packet
{
public:
    get_thread_info(wim_packet_params _params, get_thread_info_params _thrad_info_params);
    ~get_thread_info() override = default;

    std::string_view get_method() const override;
    bool auto_resend_on_fail() const override { return true; }
    const auto& get_result() const { return result_; }
    int minimal_supported_api_version() const override;
    const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return result_.get_persons(); }

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
    int32_t parse_results(const rapidjson::Value& _node_results) override;

    get_thread_info_params params_;
    thread_info result_;
};

}

