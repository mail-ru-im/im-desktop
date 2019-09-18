#include "stdafx.h"

#include "../../../log/log.h"
#include "../../../http_request.h"

#include "report_abuse.h"
#include "../../urls_cache.h"

namespace
{
    std::string get_reason_string(const core::wim::report_reason& _reason)
    {
        std::string result;
        switch (_reason)
        {
        case core::wim::report_reason::porn:
            result = "porno";
            break;

        case core::wim::report_reason::spam:
            result = "spam";
            break;

        case core::wim::report_reason::violation:
            result = "violation";
            break;

        default:
            result = "other";
            break;
        }

        return result;
    }
}

CORE_WIM_NS_BEGIN

report_abuse::report_abuse(wim_packet_params _params)
    : robusto_packet(std::move(_params))
{
}

int32_t report_abuse::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t report_abuse::on_response_error_code()
{
    return robusto_packet::on_response_error_code();
}

report_contact::report_contact(wim_packet_params _params, const std::string& _aimid, const report_reason& _reason)
    : report_abuse(_params)
    , aimId_(_aimid)
    , reason_(_reason)
{
}

int32_t report_contact::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    constexpr char method[] = "reportAbuse";

    _request->set_gzip(true);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimId_, a);
    node_params.AddMember("context", "profile", a);
    node_params.AddMember("reason", get_reason_string(reason_), a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

report_stickerpack::report_stickerpack(wim_packet_params _params, const int32_t _id, const report_reason& _reason)
    : report_abuse(_params)
    , id_(_id)
    , reason_(_reason)
{
}

int32_t report_stickerpack::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "reportAbuse", a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("packId", id_, a);
    node_params.AddMember("context", "stickerPack", a);
    node_params.AddMember("reason", get_reason_string(reason_), a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

report_sticker::report_sticker(wim_packet_params _params, const std::string& _id, const report_reason& _reason, const std::string& _aimId, const std::string& _chatId)
    : report_abuse(_params)
    , id_(_id)
    , reason_(_reason)
    , aimId_(_aimId)
    , chatId_(_chatId)
{
}

int32_t report_sticker::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "reportAbuse", a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("stickerId", id_, a);
    node_params.AddMember("context", "sticker", a);
    node_params.AddMember("sn", aimId_, a);
    if (!chatId_.empty())
        node_params.AddMember("chatSn", chatId_, a);
    node_params.AddMember("reason", get_reason_string(reason_), a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

report_message::report_message(wim_packet_params _params, const int64_t _id, const std::string& _text, const report_reason& _reason, const std::string& _aimId, const std::string& _chatId)
    : report_abuse(_params)
    , id_(_id)
    , text_(_text)
    , reason_(_reason)
    , aimId_(_aimId)
    , chatId_(_chatId)
{
}

int32_t report_message::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "reportAbuse", a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("msgId", id_, a);
    if (!text_.empty())
        node_params.AddMember("msgText", text_, a);
    node_params.AddMember("context", "message", a);
    node_params.AddMember("sn", aimId_, a);
    if (!chatId_.empty())
        node_params.AddMember("chatSn", chatId_, a);
    node_params.AddMember("reason", get_reason_string(reason_), a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("msgText");
        _request->set_replace_log_function(f);
    }

    return 0;
}

report_reason get_reason_by_string(const std::string& _reason)
{
    if (_reason == "spam")
        return report_reason::spam;

    if (_reason == "porn")
        return report_reason::porn;

    if (_reason == "violation")
        return report_reason::violation;

    return report_reason::other;
}

CORE_WIM_NS_END
