#include "stdafx.h"
#include "attach_uin.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../utils.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

std::string get_attach_uin_host()
{
    std::stringstream ss_host;
    ss_host << urls::get_url(urls::url_type::smsreg_host) << std::string_view("/replaceAccount.php");

    return ss_host.str();
}

std::string get_replace_account_host()
{
    std::stringstream ss_host;
    ss_host << urls::get_url(urls::url_type::account_operations_host) << std::string_view("/replaceAccount");

    return ss_host.str();
}

attach_uin::attach_uin(
    wim_packet_params _from_params, wim_packet_params _to_params)
    : wim_packet(std::move(_to_params))
    , from_params_(std::move(_from_params))
{
}

attach_uin::~attach_uin()
{
}

int32_t attach_uin::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (from_params_.a_token_.empty())
        return wpie_invalid_login;

    auto from_uin_signed_url = std::string();
    {
        core::http_request_simple request_from(_request->get_user_proxy(), utils::get_user_agent(params_.aimid_));
        const std::string host = get_replace_account_host();
        request_from.set_url(host);
        request_from.push_post_parameter("a", escape_symbols(from_params_.a_token_));
        request_from.push_post_parameter("k", escape_symbols(from_params_.dev_id_));

        time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - from_params_.time_offset_;
        request_from.push_post_parameter("ts", tools::from_int64(ts));

        request_from.push_post_parameter("sig_sha256", escape_symbols(get_url_sign(host, request_from.get_post_parameters(), from_params_, true)));
        from_uin_signed_url = request_from.get_post_url();
    }

    {
        const std::string host = get_attach_uin_host();
        _request->set_url(host);
        _request->set_normalized_url("replaceAccount");

        _request->push_post_parameter("a", escape_symbols(params_.a_token_));
        _request->push_post_parameter("k", escape_symbols(params_.dev_id_));
        _request->push_post_parameter("f", "json");
        _request->push_post_parameter("r", core::tools::system::generate_guid());

        time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
        _request->push_post_parameter("ts", tools::from_int64(ts));

        _request->push_post_parameter("fromUinSignedUrl", escape_symbols(from_uin_signed_url));

        _request->push_post_parameter("sig_sha256", escape_symbols(get_url_sign(host, _request->get_post_parameters(), params_, true)));
    }
    return 0;
}

int32_t attach_uin::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t attach_uin::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

int32_t attach_uin::on_empty_data()
{
    return 0;
}
