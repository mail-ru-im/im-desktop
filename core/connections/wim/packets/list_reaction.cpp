#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"
#include "archive/history_message.h"
#include "../log_replace_functor.h"

#include "list_reaction.h"


namespace core::wim
{

int32_t reactions_list::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "exhausted", exhausted_);
    tools::unserialize_value(_node, "newest", newest_);
    tools::unserialize_value(_node, "oldest", oldest_);

    const auto iter_reactions = _node.FindMember("reactions");

    if (iter_reactions != _node.MemberEnd() && iter_reactions->value.IsArray())
    {
        reactions_.reserve(iter_reactions->value.Size());
        for (const auto& member : iter_reactions->value.GetArray())
        {
            reaction_item reaction;
            tools::unserialize_value(member, "user", reaction.user_);
            tools::unserialize_value(member, "reaction", reaction.reaction_);
            tools::unserialize_value(member, "time", reaction.time_);
            reactions_.push_back(std::move(reaction));
        }
    }

    persons_ = std::make_shared<core::archive::persons_map>();
    *persons_ = parse_persons(_node);

    return 0;
}

void reactions_list::serialize(coll_helper _coll) const
{
    _coll.set_value_as_bool("exhausted", exhausted_);
    _coll.set_value_as_string("newest", newest_);
    _coll.set_value_as_string("oldest", oldest_);

    ifptr<iarray> reactions_array(_coll->create_array());
    if (!reactions_.empty())
    {
        reactions_array->reserve((int32_t)reactions_.size());
        for (const auto& reaction : reactions_)
        {
            coll_helper _coll_reaction(_coll->create_collection(), true);
            _coll_reaction.set_value_as_string("user", reaction.user_);
            _coll_reaction.set_value_as_string("reaction", reaction.reaction_);
            _coll_reaction.set_value_as_int64("time", reaction.time_);
            ifptr<ivalue> val(_coll->create_value());
            val->set_as_collection(_coll_reaction.get());
            reactions_array->push_back(val.get());
        }
    }
    _coll.set_value_as_array("reactions", reactions_array.get());
}

list_reaction::list_reaction(wim_packet_params _params,
                             int64_t _msg_id,
                             const std::string& _chat_id,
                             const std::string& _reaction,
                             const std::string& _newer_than,
                             const std::string& _older_than,
                             int64_t _limit)
    : robusto_packet(std::move(_params)),
      msg_id_(_msg_id),
      limit_(_limit),
      chat_id_(_chat_id),
      reaction_(_reaction),
      newer_than_(_newer_than),
      older_than_(_older_than)
{

}

std::string_view list_reaction::get_method() const
{
    return "reaction/list";
}

int list_reaction::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t list_reaction::init_request(const std::shared_ptr<http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("msgId", msg_id_, a);
    node_params.AddMember("chatId", chat_id_, a);
    node_params.AddMember("reaction", reaction_, a);
    if (!newer_than_.empty())
        node_params.AddMember("newerThan", newer_than_, a);
    if (!older_than_.empty())
        node_params.AddMember("olderThan", older_than_, a);
    node_params.AddMember("limit", limit_, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t list_reaction::parse_results(const rapidjson::Value& _node_results)
{
    if (result_.unserialize(_node_results) != 0)
        return wpie_http_parse_response;

   return 0;
}

}
