#pragma once

#include "../robusto_packet.h"
#include "archive/reactions.h"

namespace core::wim
{

using msgids_list = std::vector<int64_t>;
using reactions_vector = std::vector<archive::reactions_data>;
using reactions_vector_sptr = std::shared_ptr<reactions_vector>;

class get_reaction : public robusto_packet
{
public:
    get_reaction(wim_packet_params _params, const std::string& _chat_id, const msgids_list& _msg_ids);

    reactions_vector_sptr get_result() const;

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
    int32_t parse_results(const rapidjson::Value& _node_results) override;

    std::string chat_id_;
    msgids_list msg_ids_;
    reactions_vector_sptr reactions_;
};

}

