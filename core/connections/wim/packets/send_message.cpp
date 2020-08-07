#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../tools/json_helper.h"
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
}

using namespace core;
using namespace wim;

send_message::send_message(wim_packet_params _params,
    const int64_t _updated_id,
    const message_type _type,
    const std::string& _internal_id,
    const std::string& _aimid,
    const std::string& _message_text,
    const core::archive::quotes_vec& _quotes,
    const core::archive::mentions_map& _mentions,
    const std::string& _description,
    const std::string& _url,
    const archive::shared_contact& _shared_contact,
    const core::archive::geo& _geo,
    const core::archive::poll& _poll,
    const smartreply::marker_opt& _smartreply_marker)
    :
        wim_packet(std::move(_params)),
        updated_id_(_updated_id),
        type_(_type),
        aimid_(_aimid),
        message_text_(_message_text),
        sms_error_(0),
        sms_count_(0),
        internal_id_(_internal_id),
        hist_msg_id_(-1),
        before_hist_msg_id(-1),
        duplicate_(false),
        quotes_(_quotes),
        mentions_(_mentions),
        description_(_description),
        url_(_url),
        shared_contact_(_shared_contact),
        geo_(_geo),
        poll_(_poll),
        smartreply_marker_(_smartreply_marker)
{
    assert(!internal_id_.empty());
}

send_message::~send_message() = default;

int32_t send_message::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    const auto is_sticker = (type_ == message_type::sticker);
    const auto is_sms = (type_ == message_type::sms);

    const auto method = is_sticker && quotes_.empty() ? std::string_view("sendSticker") : std::string_view("sendIM");

    _request->set_url(su::concat(urls::get_url(urls::url_type::wim_host), "im/", method));
    _request->set_normalized_url(method);
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

        for (const auto& quote : quotes_)
        {
            rapidjson::Value quote_params(rapidjson::Type::kObjectType);
            quote_params.AddMember("mediaType", quote.get_type(), a);

            if (!quote.get_description().empty())
            {
                quote_params.AddMember("text", "", a);
                rapidjson::Value caption_params(rapidjson::Type::kObjectType);
                caption_params.AddMember("caption", quote.get_description(), a);
                caption_params.AddMember("url", quote.get_url(), a);
                quote_params.AddMember("captionedContent", std::move(caption_params), a);
            }
            else if (quote.get_shared_contact())
            {
                quote_params.AddMember("text", "", a);
                rapidjson::Value contact_params(rapidjson::Type::kObjectType);
                contact_params.AddMember("name", quote.get_shared_contact()->name_, a);
                contact_params.AddMember("phone", quote.get_shared_contact()->phone_, a);
                if (!quote.get_shared_contact()->sn_.empty())
                    contact_params.AddMember("sn", quote.get_shared_contact()->sn_, a);
                quote_params.AddMember("contact", std::move(contact_params), a);
            }
            else if (quote.get_geo())
            {
                quote_params.AddMember("text", quote.get_text(), a);
                rapidjson::Value geo_params(rapidjson::Type::kObjectType);
                geo_params.AddMember("name", quote.get_geo()->name_, a);
                geo_params.AddMember("lat", quote.get_geo()->lat_, a);
                geo_params.AddMember("long", quote.get_geo()->long_, a);
                quote_params.AddMember("geo", std::move(geo_params), a);
            }
            else if (quote.get_poll())
            {
                quote_params.AddMember("text", quote.get_text(), a);
                rapidjson::Value poll_params(rapidjson::Type::kObjectType);
                poll_params.AddMember("id", quote.get_poll()->id_, a);
                quote_params.AddMember("poll", std::move(poll_params), a);
            }
            else
            {
                if (auto sticker = quote.get_sticker(); !sticker.empty())
                    quote_params.AddMember("stickerId", std::move(sticker), a);
                else
                    quote_params.AddMember("text", quote.get_text(), a);
            }
            quote_params.AddMember("sn", quote.get_sender(), a);
            quote_params.AddMember("msgId", quote.get_msg_id(), a);
            quote_params.AddMember("time", quote.get_time(), a);

            if (const auto& chat_sn = quote.get_chat(); chat_sn.find("@chat.agent") != chat_sn.npos)
            {
                rapidjson::Value chat_params(rapidjson::Type::kObjectType);
                chat_params.AddMember("sn", chat_sn, a);
                if (const auto& chat_stamp = quote.get_stamp(); !chat_stamp.empty())
                    chat_params.AddMember("stamp", chat_stamp, a);
                quote_params.AddMember("chat", std::move(chat_params), a);
            }

            doc.PushBack(std::move(quote_params), a);
        }

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

                    if (poll_)
                        text_params.AddMember("poll", create_poll_params(poll_, a), a);
                }
                doc.PushBack(std::move(text_params), a);
            }

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

    if (quotes_.empty() && !geo_ && !poll_)
    {
        if (description_.empty())
        {
            std::string message_text = is_sticker ? message_text_ : escape_symbols(message_text_);
            _request->push_post_parameter((is_sticker ? "stickerId" : "message"), std::move(message_text));
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

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_message_markers();
        f.add_marker("aimsid", aimsid_range_evaluator());
        f.add_marker("message");
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

    if (std::string_view state; tools::unserialize_value(_data, "state", state) && state == "duplicate")
        duplicate_ = true;

    return 0;
}

int32_t send_message::execute_request(const std::shared_ptr<core::http_request_simple>& request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

priority_t send_message::get_priority() const
{
    return priority_send_message();
}
