#pragma once

#include "../robusto_packet.h"
#include "archive/reactions.h"

namespace core::wim
{

using reactions_vector = std::vector<archive::reactions_data>;
using reactions_vector_sptr = std::shared_ptr<reactions_vector>;

class add_reaction : public robusto_packet
{
public:
    add_reaction(wim_packet_params _params, int64_t _msg_id, const std::string& _chat_id, const std::string& _reaction, const std::vector<std::string>& _reactions);

    const archive::reactions_data& get_result() const;
    bool is_reactions_for_message_disabled() const;

    std::string_view get_method() const override;
    int minimal_supported_api_version() const override;

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
    int32_t parse_results(const rapidjson::Value& _node_results) override;

    int64_t msg_id_;
    std::string chat_id_;
    std::string reaction_;
    std::vector<std::string> reactions_list_;
    archive::reactions_data reactions_;
};

}

