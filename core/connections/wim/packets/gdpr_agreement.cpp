#include "stdafx.h"

#include "gdpr_agreement.h"

#include "../../../http_request.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

std::string get_gdpr_agreement_host()
{
    std::stringstream ss_host;
    ss_host << urls::get_url(urls::url_type::wapi_host) << std::string_view("/agreement/save");

    return ss_host.str();
}

gdpr_agreement::gdpr_agreement(wim_packet_params _params, const accept_agreement_info &_info)
    : wim_packet(std::move(_params)),
      accept_info_(_info)
{
}

int32_t gdpr_agreement::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::string host = get_gdpr_agreement_host();

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    std::map<std::string, std::string> params;
    params["a"] = escape_symbols(params_.a_token_);
    params["k"] = params_.dev_id_;
    params["ts"] = std::to_string((int64_t) ts);
    params["sig_sha256"] = escape_symbols(get_url_sign(host, params, params_, true));

    std::stringstream ss_url;
    ss_url << host << '?' << format_get_params(params);

    _request->push_post_parameter("f", "json");

    std::string gdpr_param_value;

    switch (accept_info_.action_)
    {
    case accept_agreement_info::agreement_action::accept:
        gdpr_param_value = "accept";
        break;
    case accept_agreement_info::agreement_action::ignore:
        gdpr_param_value = "ignore";
        break;
    default:
        assert(!"unhandled agreement action, fix");
        break;
    }

    _request->push_post_parameter("gdpr_pp", std::move(gdpr_param_value));

    if (accept_info_.reset_)
        _request->push_post_parameter("reset", "1");

    _request->set_url(ss_url.str());
    _request->set_normalized_url("agreementSave");
    _request->set_priority(high_priority());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t gdpr_agreement::on_response_error_code()
{
    return wpie_login_unknown_error;
}

int32_t gdpr_agreement::parse_response_data(const rapidjson::Value& _data)
{
    UNUSED_ARG(_data);
    return 0;
}

void gdpr_agreement::parse_response_data_on_error(const rapidjson::Value &_data)
{
    UNUSED_ARG(_data);
}

int32_t gdpr_agreement::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t) request->get_response_code();
    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

int32_t gdpr_agreement::on_empty_data()
{
    return 0;
}
