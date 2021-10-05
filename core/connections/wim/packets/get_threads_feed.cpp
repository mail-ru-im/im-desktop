#include "stdafx.h"
#include "http_request.h"
#include "get_threads_feed.h"
#include "archive/history_message.h"
#include "archive/thread_parent_topic.h"
#include "../../../../corelib/collection_helper.h"
#include "../wim_history.h"
#include "../../../../common.shared/json_helper.h"

namespace core::wim
{

void thread_feed_data::unserialize(const rapidjson::Value& _thread_node, const archive::persons_map& _persons)
{
    if (auto it = _thread_node.FindMember("parentTopic");  it != _thread_node.MemberEnd())
        parent_topic_.unserialize(it->value);

    tools::unserialize_value(_thread_node, "threadId", thread_id_);
    tools::unserialize_value(_thread_node, "olderMsgId", older_msg_id_);
    tools::unserialize_value(_thread_node, "repliesCount", replies_count_);
    tools::unserialize_value(_thread_node, "unreadCnt", unread_count_);
    archive::history_block pinned;
    parse_history_messages_json(_thread_node, -1, thread_id_, messages_, _persons, message_order::direct);
    parse_history_messages_json(_thread_node, -1, thread_id_, pinned, _persons, message_order::reverse, "pinned");
    if (!pinned.empty())
    {
        parent_message_ = *pinned.front();
        parent_message_.set_thread_id(thread_id_);
    }
}

void thread_feed_data::serialize(coll_helper& _coll, int _time_offset) const
{
    coll_helper msg_coll(g_core->create_collection(), true);
    parent_message_.serialize(msg_coll.get(), _time_offset);
    parent_topic_.serialize(_coll);
    _coll.set_value_as_collection("parent_message", msg_coll.get());
    _coll.set_value_as_string("thread_id", thread_id_);
    _coll.set_value_as_int64("older_msg_id", older_msg_id_);
    _coll.set_value_as_int64("replies_count", replies_count_);
    _coll.set_value_as_int64("unread_count", unread_count_);

    ifptr<iarray> messages_array(_coll->create_array());
    for (const auto& msg : messages_)
    {
        coll_helper coll_message(_coll->create_collection(), true);
        msg->serialize(coll_message.get(), _time_offset);
        ifptr<ivalue> val(_coll->create_value());
        val->set_as_collection(coll_message.get());
        messages_array->push_back(val.get());
    }
    _coll.set_value_as_array("messages", messages_array.get());
}

get_threads_feed::get_threads_feed(wim_packet_params _params, const std::string& _cursor)
    : robusto_packet(std::move(_params))
    , cursor_(_cursor)
    , persons_(std::make_shared<core::archive::persons_map>())
{

}

std::string_view get_threads_feed::get_method() const
{
    return "thread/feed/get";
}

const std::vector<thread_feed_data>& get_threads_feed::get_data() const
{
    return data_;
}

const std::string& get_threads_feed::get_cursor() const
{
    return cursor_;
}

const std::vector<archive::thread_update>& get_threads_feed::get_updates() const
{
    return updates_;
}

bool get_threads_feed::get_reset_page() const
{
    return reset_page_;
}

int32_t get_threads_feed::init_request(const std::shared_ptr<http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("pageSize", 10, a);
    node_params.AddMember("cursor", cursor_, a);

    rapidjson::Value node_filter(rapidjson::Type::kObjectType);
    node_filter.AddMember("type", "all", a);
    node_params.AddMember("filter", std::move(node_filter), a);

    doc.AddMember("params", std::move(node_params), a);
    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_message_markers();
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_threads_feed::parse_results(const rapidjson::Value& _node_results)
{
    if (get_status_code() == robusto_protocol_error::reset_page)
        reset_page_ = true;

    *persons_ = parse_persons(_node_results);
    const auto it_threads = _node_results.FindMember("threads");
    if (it_threads != _node_results.MemberEnd())
    {
        data_.reserve(it_threads->value.GetArray().Size());
        for (const auto& thread_item : it_threads->value.GetArray())
        {
            thread_feed_data item;
            item.unserialize(thread_item, *persons_);
            data_.push_back(std::move(item));

            archive::thread_update update;
            update.unserialize(thread_item);
            updates_.push_back(std::move(update));
        }
    }
    cursor_.clear();
    tools::unserialize_value(_node_results, "cursor", cursor_);

    return 0;
}

}
