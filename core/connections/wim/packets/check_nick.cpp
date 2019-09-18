#include "stdafx.h"
#include "check_nick.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

check_nick::check_nick(wim_packet_params _params, const std::string& _nick, bool _set_nick)
    : robusto_packet(std::move(_params))
    , nickname_(_nick)
    , set_nick_(_set_nick)
{
}

int32_t check_nick::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    auto method = set_nick_ ? std::string("setNick") : std::string("checkNick");

    _request->set_gzip(false);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("nick", nickname_, a);

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

int32_t check_nick::on_response_error_code()
{
    if (status_code_ == 40000)
        return wpie_error_nickname_bad_value;
    else if (status_code_ == 40600)
        return wpie_error_nickname_already_used;
    else if (status_code_ == 40100)
        return wpie_error_nickname_not_allowed;

    return wpie_error_message_unknown;
}
