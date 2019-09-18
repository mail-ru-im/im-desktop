#include "stdafx.h"
#include "add_members.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

add_members::add_members(
    wim_packet_params _params,
    const std::string& _aimid,
    const std::string & _members_to_add)
    :    wim_packet(std::move(_params)),
    aimid_(_aimid),
    members_to_add_(_members_to_add)
{
}

add_members::~add_members()
{
}

int32_t add_members::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;


    ss_url << urls::get_url(urls::url_type::wim_host) << "mchat/AddChat" <<
        "?f=json" <<
        "&chat_id=" <<  aimid_ <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&r=" <<  core::tools::system::generate_guid() <<
        "&members=" << members_to_add_;

    _request->set_url(ss_url.str());
    _request->set_normalized_url("addMembers");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t add_members::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t core::wim::add_members::on_response_error_code()
{
    if (status_code_ == 453)
        return wpie_error_cannot_add_member;
    return wpie_http_error;
}

int32_t core::wim::add_members::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->get())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
    {
        status_code_ = http_code_;
        return wpie_http_error;
    }

    return 0;
}
