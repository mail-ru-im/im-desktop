#include "stdafx.h"
#include "search_chat_threads.h"

#include "../../../archive/history_message.h"
#include "../../../archive/thread_parent_topic.h"
#include "../../../http_request.h"
#include "../log_replace_functor.h"

#include "../../../../common.shared/json_helper.h"

namespace
{
    constexpr auto results_per_page = 50;
}

namespace core::wim
{
    search_chat_threads::search_chat_threads(wim_packet_params _packet_params, const std::string_view _keyword, const std::string_view _contact, const std::string_view _cursor)
        : robusto_packet(std::move(_packet_params))
        , keyword_(_keyword)
        , contact_(_contact)
        , cursor_(_cursor)
        , persons_(std::make_shared<core::archive::persons_map>())
    {
    }

    const std::shared_ptr<core::archive::persons_map>& search_chat_threads::get_persons() const
    {
        return persons_;
    }

    std::string_view search_chat_threads::get_method() const
    {
        return "searchChatThreads";
    }

    int search_chat_threads::minimal_supported_api_version() const
    {
        return thread_search_minimal_supported_api_version();
    }

    void search_chat_threads::set_dates_range(const std::string_view _start_date, const std::string_view _end_date)
    {
        start_date_ = _start_date;
        end_date_ = _end_date;
    }

    void search_chat_threads::reset_dates()
    {
        set_dates_range(std::string_view(), std::string_view());
    }

    int32_t search_chat_threads::init_request(const std::shared_ptr<core::http_request_simple>& _request)
    {
        if (keyword_.empty() && cursor_.empty())
            return -1;

        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        rapidjson::Value node_params(rapidjson::Type::kObjectType);

        if (!cursor_.empty())
        {
            node_params.AddMember("cursorAll", cursor_, a);
        }
        else
        {
            node_params.AddMember("sn", contact_, a);

            rapidjson::Value node_filter(rapidjson::Type::kObjectType);
            node_filter.AddMember("keyword", keyword_, a);

            if (!start_date_.empty() || !end_date_.empty())
            {
                rapidjson::Value node_date(rapidjson::Type::kObjectType);
                if (!start_date_.empty())
                    node_date.AddMember("after", start_date_, a);

                if (!end_date_.empty())
                    node_date.AddMember("before", end_date_, a);

                node_filter.AddMember("date", std::move(node_date), a);
            }

            node_params.AddMember("filter", std::move(node_filter), a);
        }

        node_params.AddMember("pagesize", results_per_page, a);

        doc.AddMember("params", std::move(node_params), a);

        setup_common_and_sign(doc, a, _request, get_method());

        if (!robusto_packet::params_.full_log_)
        {
            log_replace_functor f;
            f.add_json_marker("aimsid", aimsid_range_evaluator());
            f.add_json_array_marker("highlight");
            if (hide_keyword_)
                f.add_json_marker("keyword");

            f.add_message_markers();

            _request->set_replace_log_function(f);
        }

        return 0;
    }

    int32_t search_chat_threads::parse_results(const rapidjson::Value& _node_results)
    {
        bool no_more_results = false;
        tools::unserialize_value(_node_results, "exhausted", no_more_results);

        if (!no_more_results)
            tools::unserialize_value(_node_results, "cursorAll", results_.cursor_next_);

        tools::unserialize_value(_node_results, "totalEntryCount", results_.total_entry_count_);

        results_.keyword_ = keyword_;
        results_.contact_ = contact_;
        results_.unserialize_monthly_histogram(_node_results);

        const auto iter_dialogs = _node_results.FindMember("dialogs");
        if (iter_dialogs == _node_results.MemberEnd() || !iter_dialogs->value.IsArray())
            return wpie_http_parse_response;

        results_.messages_.reserve(iter_dialogs->value.Size());
        for (auto iter = iter_dialogs->value.Begin(), iter_end = iter_dialogs->value.End(); iter != iter_end; ++iter)
        {
            std::string contact;
            std::string cursor;
            int entry_count = 0;

            tools::unserialize_value(*iter, "sn", contact);
            tools::unserialize_value(*iter, "cursorOne", cursor);
            tools::unserialize_value(*iter, "peerEntryCount", entry_count);

            const auto iter_parent_topic = iter->FindMember("parentId");
            core::archive::thread_parent_topic parent_topic;
            if (iter_parent_topic != iter->MemberEnd() && iter_parent_topic->value.IsObject())
                parent_topic.unserialize(iter_parent_topic->value);

            if (!parse_entries(*iter, parent_topic, contact, cursor, entry_count))
                return wpie_http_parse_response;
        }

        *persons_ = parse_persons(_node_results);

        return 0;
    }

    bool search_chat_threads::parse_entries(const rapidjson::Value & _root_node, const core::archive::thread_parent_topic& _parent_topic, const std::string_view _contact, const std::string_view _cursor, const int _entry_count)
    {
        const auto iter_entries = _root_node.FindMember("entries");
        if (iter_entries == _root_node.MemberEnd() || !iter_entries->value.IsArray())
            return false;

        results_.messages_.reserve(iter_entries->value.Size());
        for (auto iter = iter_entries->value.Begin(), iter_end = iter_entries->value.End(); iter != iter_end; ++iter)
        {
            if (!parse_single_entry(*iter, _parent_topic, _contact, _cursor, _entry_count))
                return false;
        }

        return true;
    }

    bool search_chat_threads::parse_single_entry(const rapidjson::Value& _entry_node, const core::archive::thread_parent_topic& _parent_topic, const std::string_view _contact, const std::string_view _cursor, const int _entry_count)
    {
        const auto iter_hl = _entry_node.FindMember("highlight");
        if (iter_hl == _entry_node.MemberEnd() || !iter_hl->value.IsArray())
        {
            im_assert("entry with no highlights");
            return true; // skip messages without highlights
        }

        search::searched_message msg;
        msg.contact_ = _contact;
        msg.contact_cursor_ = _cursor;
        msg.contact_entry_count_ = _entry_count;
        if (_parent_topic.is_valid())
            msg.parent_topic_ = std::make_shared<core::archive::thread_parent_topic>(_parent_topic);

        msg.highlights_.reserve(iter_hl->value.Size());
        for (auto ithl = iter_hl->value.Begin(), ithl_end = iter_hl->value.End(); ithl != ithl_end; ++ithl)
            msg.highlights_.push_back(rapidjson_get_string(*ithl));

        const auto iter_message = _entry_node.FindMember("message");
        if (iter_message == _entry_node.MemberEnd() || !iter_message->value.IsObject())
            return false;

        bool outgoing = false;
        tools::unserialize_value(iter_message->value, "outgoing", outgoing);
        const auto& aimid = outgoing ? params_.aimid_ : msg.contact_;

        msg.message_ = std::make_shared<archive::history_message>();
        if (msg.message_->unserialize(iter_message->value, aimid) == 0)
            results_.messages_.emplace_back(std::move(msg));

        return true;
    }
}
