#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../../common.shared/json_helper.h"
#include "../../../archive/history_message.h"
#include "../../../../common.shared/smartreply/smartreply_types.h"
#include "../../urls_cache.h"
#include "send_message.h"

#include "../common.shared/string_utils.h"

namespace
{
    rapidjson::Value create_poll_params(const core::archive::poll& _poll, rapidjson_allocator& _a)
    {
        rapidjson::Value poll_params(rapidjson::Type::kObjectType);

        if (_poll->id_.empty())
        {
            poll_params.AddMember("type", "anon", _a);

            rapidjson::Value responses_array(rapidjson::Type::kArrayType);
            responses_array.Reserve(_poll->answers_.size(), _a);
            for (const auto& response : _poll->answers_)
                responses_array.PushBack(core::tools::make_string_ref(response.text_), _a);

            poll_params.AddMember("responses", std::move(responses_array), _a);
        }
        else
        {
            poll_params.AddMember("id", _poll->id_, _a);
        }

        return poll_params;
    }

    rapidjson::Value create_task_params(const core::tasks::task& _task, rapidjson_allocator& _a)
    {
        rapidjson::Value text_params(rapidjson::Type::kObjectType);
        text_params.AddMember("mediaType", "text", _a);
        text_params.AddMember("text", _task->params_.title_, _a);

        rapidjson::Value task_params(rapidjson::Type::kObjectType);
        task_params.AddMember("title", _task->params_.title_, _a);
        task_params.AddMember("endTime", _task->end_time_to_seconds(), _a);
        task_params.AddMember("assignee", _task->params_.assignee_, _a);

        text_params.AddMember("task", task_params, _a);
        return text_params;
    }
}

using namespace core;
using namespace wim;

send_message::send_message(wim_packet_params _params,
    const int64_t _updated_id,
    const message_type _type,
    const std::string& _internal_id,
    const std::string& _aimid,
    const std::string& _message_text,
    const core::data::format& _message_format,
    const core::archive::quotes_vec& _quotes,
    const core::archive::mentions_map& _mentions,
    const std::string& _description,
    const core::data::format& _description_format,
    const std::string& _url,
    const archive::shared_contact& _shared_contact,
    const core::archive::geo& _geo,
    const core::archive::poll& _poll,
    const core::tasks::task& _task,
    const smartreply::marker_opt& _smartreply_marker,
    const std::optional<int64_t>& _draft_delete_time)
    :
        wim_packet(std::move(_params)),
        updated_id_(_updated_id),
        type_(_type),
        aimid_(_aimid),
        message_text_(_message_text),
        message_format_(_message_format),
        sms_error_(0),
        sms_count_(0),
        internal_id_(_internal_id),
        hist_msg_id_(-1),
        before_hist_msg_id(-1),
        duplicate_(false),
        quotes_(_quotes),
        mentions_(_mentions),
        description_(_description),
        description_format_(_description_format),
        url_(_url),
        shared_contact_(_shared_contact),
        geo_(_geo),
        poll_(_poll),
        task_(_task),
        smartreply_marker_(_smartreply_marker),
        draft_delete_time_(_draft_delete_time)
{
    im_assert(!internal_id_.empty());
}

send_message::~send_message() = default;

int32_t send_message::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    const auto is_sticker = (type_ == message_type::sticker);
    const auto is_sms = (type_ == message_type::sms);

    _request->set_url(su::concat(urls::get_url(urls::url_type::wim_host), "im/", get_method()));
    _request->set_normalized_url(get_method());
    _request->set_keep_alive();
    _request->set_compression_auto();
    _request->push_post_parameter("f", "json");
    _request->push_post_parameter("aimsid", escape_symbols(get_params().aimsid_));
    _request->push_post_parameter("t", escape_symbols(aimid_));
    _request->push_post_parameter("r", internal_id_);

    if (updated_id_ > 0)
        _request->push_post_parameter("updateMsgId", std::to_string(updated_id_));

    if (!quotes_.empty())
    {
        rapidjson::Document doc(rapidjson::Type::kArrayType);
        auto& a = doc.GetAllocator();
        doc.Reserve(quotes_.size() + 1, a);

        archive::serialize_message_quotes(quotes_, doc, a);

        if (description_.empty())
        {
            if (!message_text_.empty())
            {
                rapidjson::Value text_params(rapidjson::Type::kObjectType);
                if (is_sticker)
                {
                    text_params.AddMember("mediaType", "sticker", a);
                    text_params.AddMember("stickerId", message_text_, a);
                }
                else
                {
                    text_params.AddMember("mediaType", "text", a);
                    text_params.AddMember("text", message_text_, a);
                    if (!message_format_.empty())
                        text_params.AddMember("format", message_format_.serialize(a), a);

                    if (poll_)
                        text_params.AddMember("poll", create_poll_params(poll_, a), a);
                }
                doc.PushBack(std::move(text_params), a);
            }

            if (task_)
                doc.PushBack(create_task_params(task_, a), a);

            if (shared_contact_)
            {
                rapidjson::Value text_params(rapidjson::Type::kObjectType);
                text_params.AddMember("mediaType", "text", a);
                text_params.AddMember("text", "", a);

                rapidjson::Value contact_params(rapidjson::Type::kObjectType);
                contact_params.AddMember("name", shared_contact_->name_, a);
                contact_params.AddMember("phone", shared_contact_->phone_, a);
                if (!shared_contact_->sn_.empty())
                    contact_params.AddMember("sn", shared_contact_->sn_, a);
                text_params.AddMember("contact", std::move(contact_params), a);
                doc.PushBack(std::move(text_params), a);
            }
        }
        else
        {
            rapidjson::Value text_params(rapidjson::Type::kObjectType);
            text_params.AddMember("mediaType", "text", a);
            text_params.AddMember("text", "", a);

            rapidjson::Value caption_params(rapidjson::Type::kObjectType);
            caption_params.AddMember("caption", description_, a);
            if (!description_format_.empty())
                caption_params.AddMember("format", description_format_.serialize(a), a);
            caption_params.AddMember("url", url_, a);
            text_params.AddMember("captionedContent", std::move(caption_params), a);
            doc.PushBack(std::move(text_params), a);
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        _request->push_post_parameter("parts", escape_symbols(rapidjson_get_string_view(buffer)));
    }
    else if (poll_)
    {
        rapidjson::Document doc(rapidjson::Type::kArrayType);
        auto& a = doc.GetAllocator();

        rapidjson::Value text_params(rapidjson::Type::kObjectType);
        text_params.AddMember("mediaType", "text", a);
        text_params.AddMember("text", message_text_, a);

        text_params.AddMember("poll", create_poll_params(poll_, a), a);
        doc.PushBack(std::move(text_params), a);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        _request->push_post_parameter("parts", escape_symbols(rapidjson_get_string_view(buffer)));
    }
    else if (task_)
    {
        rapidjson::Document doc(rapidjson::Type::kArrayType);
        auto& a = doc.GetAllocator();

        doc.PushBack(create_task_params(task_, a), a);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        _request->push_post_parameter("parts", escape_symbols(rapidjson_get_string_view(buffer)));
    }

    if (!mentions_.empty())
    {
        std::string str;
        for (const auto& [uin, _] : mentions_)
        {
            str += uin;
            str += ',';
        }
        str.pop_back();
        _request->push_post_parameter("mentions", std::move(str));
    }

    if (quotes_.empty() && !geo_ && !poll_ && !task_)
    {
        if (description_.empty())
        {
            if (message_format_.empty())
            {
                std::string message_text = is_sticker ? message_text_ : escape_symbols(message_text_);
                _request->push_post_parameter((is_sticker ? "stickerId" : "message"), std::move(message_text));
            }
            else
            {
                im_assert(!is_sticker);
                rapidjson::Document doc(rapidjson::Type::kArrayType);
                auto& a = doc.GetAllocator();

                rapidjson::Value text_params(rapidjson::Type::kObjectType);
                text_params.AddMember("mediaType", "text", a);
                text_params.AddMember("text", message_text_, a);

                rapidjson::Value format_params(rapidjson::Type::kObjectType);
                text_params.AddMember("format", message_format_.serialize(a), a);
                doc.PushBack(std::move(text_params), a);

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);

                _request->push_post_parameter("parts", escape_symbols(rapidjson_get_string_view(buffer)));
            }
        }
        else
        {
            rapidjson::Document doc(rapidjson::Type::kArrayType);
            auto& a = doc.GetAllocator();

            rapidjson::Value text_params(rapidjson::Type::kObjectType);
            text_params.AddMember("mediaType", "text", a);
            text_params.AddMember("text", "", a);

            rapidjson::Value caption_params(rapidjson::Type::kObjectType);
            caption_params.AddMember("caption", description_, a);
            if (!description_format_.empty())
                caption_params.AddMember("format", description_format_.serialize(a), a);
            caption_params.AddMember("url", url_, a);
            text_params.AddMember("captionedContent", std::move(caption_params), a);
            doc.PushBack(std::move(text_params), a);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            _request->push_post_parameter("parts", escape_symbols(rapidjson_get_string_view(buffer)));
        }

        if (shared_contact_)
        {
            rapidjson::Document doc(rapidjson::Type::kArrayType);
            auto& a = doc.GetAllocator();

            rapidjson::Value text_params(rapidjson::Type::kObjectType);
            text_params.AddMember("mediaType", "text", a);
            text_params.AddMember("text", "", a);

            rapidjson::Value contact_params(rapidjson::Type::kObjectType);
            contact_params.AddMember("name", shared_contact_->name_, a);
            contact_params.AddMember("phone", shared_contact_->phone_, a);
            if (!shared_contact_->sn_.empty())
                contact_params.AddMember("sn", shared_contact_->sn_, a);
            text_params.AddMember("contact", std::move(contact_params), a);
            doc.PushBack(std::move(text_params), a);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            _request->push_post_parameter("parts", escape_symbols(rapidjson_get_string_view(buffer)));
        }
    }
    if (geo_)
    {
        rapidjson::Document doc(rapidjson::Type::kArrayType);
        auto& a = doc.GetAllocator();

        rapidjson::Value text_params(rapidjson::Type::kObjectType);
        text_params.AddMember("mediaType", "text", a);
        text_params.AddMember("text", message_text_, a);

        rapidjson::Value geo_params(rapidjson::Type::kObjectType);
        geo_params.AddMember("name", geo_->name_, a);
        geo_params.AddMember("lat", geo_->lat_, a);
        geo_params.AddMember("long", geo_->long_, a);
        text_params.AddMember("geo", std::move(geo_params), a);
        doc.PushBack(std::move(text_params), a);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        _request->push_post_parameter("parts", escape_symbols(rapidjson_get_string_view(buffer)));
    }

    if (smartreply_marker_)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        doc.AddMember("messageId", smartreply_marker_->msgid_, a);
        doc.AddMember("type", std::string(smartreply::type_2_string(smartreply_marker_->type_)), a);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        _request->push_post_parameter("smartReplyMeta", escape_symbols(rapidjson_get_string_view(buffer)));
    }

    if (is_sms)
    {
        _request->push_post_parameter("displaySMSSegmentData", "true");
    }
    else
    {
        _request->push_post_parameter("offlineIM", "1");
        _request->push_post_parameter("notifyDelivery", "true");
    }

    if (draft_delete_time_)
        _request->push_post_parameter("draftDeleteTime", std::to_string(*draft_delete_time_));

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_message_markers();
        f.add_marker("aimsid", aimsid_range_evaluator());
        f.add_marker("message");
        f.add_marker("text");
        f.add_json_array_marker("responses", poll_responses_ranges_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t send_message::parse_response_data(const rapidjson::Value& _data)
{
    tools::unserialize_value(_data, "histMsgId", hist_msg_id_);
    tools::unserialize_value(_data, "beforeHistMsgId", before_hist_msg_id);
    tools::unserialize_value(_data, "pollId", poll_id_);
    tools::unserialize_value(_data, "taskId", task_id_);

    if (std::string_view state; tools::unserialize_value(_data, "state", state) && state == "duplicate")
        duplicate_ = true;

    return 0;
}

int32_t send_message::execute_request(const std::shared_ptr<core::http_request_simple>& request)
{
    url_ = request->get_url();

    if (auto error_code = get_error(request->post()))
        return *error_code;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

priority_t send_message::get_priority() const
{
    return priority_send_message();
}

std::string_view send_message::get_method() const
{
    return type_ == message_type::sticker && quotes_.empty() ? std::string_view("sendSticker") : std::string_view("sendIM");
}
