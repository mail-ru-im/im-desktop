#pragma once

#include "../robusto_packet.h"
#include "archive/reactions.h"
#include "../corelib/collection_helper.h"
#include "../persons.h"

namespace core::wim
{

using msgids_list = std::vector<int64_t>;
using reactions_vector = std::vector<archive::reactions_data>;
using reactions_vector_sptr = std::shared_ptr<reactions_vector>;

struct reactions_list
{
    struct reaction_item
    {
        std::string user_;
        std::string reaction_;
        int64_t time_;
    };

    int32_t unserialize(const rapidjson::Value& _node);
    void serialize(core::coll_helper _coll) const;

    const auto& get_persons() const { return persons_; }

    bool exhausted_ = false;
    std::string newest_;
    std::string oldest_;
    std::vector<reaction_item> reactions_;
    std::shared_ptr<core::archive::persons_map> persons_;
};

class list_reaction : public robusto_packet
{
public:
    list_reaction(wim_packet_params _params,
                  int64_t _msg_id,
                  const std::string& _chat_id,
                  const std::string& _reaction,
                  const std::string& _newer_than,
                  const std::string& _older_than,
                  int64_t _limit);

    const auto& get_result() const { return result_; }

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
    int32_t parse_results(const rapidjson::Value& _node_results) override;

    int64_t msg_id_;
    int64_t limit_;
    std::string chat_id_;
    std::string reaction_;
    std::string newer_than_;
    std::string older_than_;
    reactions_list result_;
};

}

