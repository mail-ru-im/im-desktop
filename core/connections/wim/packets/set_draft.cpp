#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../common.shared/json_helper.h"
#include "archive/history_message.h"
#include "tools/features.h"

#include "set_draft.h"

namespace
{

std::string_view limit_str_size(std::string_view _str, size_t _max_size) // find a valid position to cut a string
{
    if (_str.size() <= _max_size)
        return _str;

    auto cut_index = _max_size;
    while (((_str[cut_index] & 0xC0) == 0x80) && cut_index > 0) // intermediate unicode bytes start with 10 (0x80 is a mask), we can not cut at intermediate bytes
        cut_index--;

    return _str.substr(0, cut_index);
}

}

namespace core::wim
{

set_draft::set_draft(wim_packet_params _params, std::string_view _contact, const archive::draft& _draft)
    : robusto_packet(std::move(_params))
    , contact_(_contact)
    , draft_(_draft)
{

}

std::string_view set_draft::get_method() const
{
    return "draft/set";
}

int32_t set_draft::init_request(const std::shared_ptr<http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("sn", contact_, a);
    node_params.AddMember("time", draft_.timestamp_, a);

    auto& message = draft_.message_;

    rapidjson::Document parts_doc(rapidjson::Type::kArrayType);
    auto& parts_a = parts_doc.GetAllocator();

    if (message)
    {
        parts_doc.Reserve(message->get_quotes().size() + 1, parts_a);
        archive::serialize_message_quotes(message->get_quotes(), parts_doc, parts_a);

        if (!message->get_text().empty())
        {
            rapidjson::Value text_params(rapidjson::Type::kObjectType);

            text_params.AddMember("mediaType", "text", parts_a);
            text_params.AddMember("text", tools::make_string_ref(limit_str_size(std::string_view(message->get_text()), features::draft_maximum_length())), parts_a);
            const auto format = message->get_format();
            if (!format.empty())
                text_params.AddMember("format", format.serialize(a), a);

            parts_doc.PushBack(std::move(text_params), parts_a);
        }
    }

    node_params.AddMember("parts", parts_doc, a);

    if (message && !message->get_mentions().empty())
    {
        rapidjson::Value ment_params(rapidjson::Type::kArrayType);
        ment_params.Reserve(message->get_mentions().size(), a);
        for (const auto& [uin, _] : message->get_mentions())
            ment_params.PushBack(tools::make_string_ref(uin), a);
        node_params.AddMember("mentions", ment_params, a);
    }

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

int32_t set_draft::parse_results(const rapidjson::Value& _node_results)
{
    return 0;
}

}
