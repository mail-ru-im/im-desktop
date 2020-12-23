#pragma once

#include "../robusto_packet.h"
#include "archive/poll.h"

namespace core::wim
{

class get_poll : public robusto_packet
{
public:
    get_poll(wim_packet_params _params, const std::string& _poll_id);

    const archive::poll_data& get_result() const;

    virtual std::string_view get_method() const override;

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
    int32_t parse_results(const rapidjson::Value& _node_results) override;
    int32_t on_response_error_code() override;

    std::string poll_id_;
    archive::poll_data poll_;
};

}

