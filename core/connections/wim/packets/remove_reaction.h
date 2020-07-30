#pragma once

#include "../robusto_packet.h"
#include "archive/reactions.h"

namespace core::wim
{

class remove_reaction : public robusto_packet
{
public:
    remove_reaction(wim_packet_params _params, int64_t _msg_id, const std::string& _chat_id);

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

    int64_t msg_id_;
    std::string chat_id_;
};

}

