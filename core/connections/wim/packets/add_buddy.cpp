#include "stdafx.h"
#include "add_buddy.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

#include "../../urls_cache.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

add_buddy::add_buddy(
    wim_packet_params _params,
    const std::string& _aimid,
    const std::string& _group,
    const std::string& _auth_message)

    :    wim_packet(std::move(_params)),
    aimid_(_aimid),
    group_(_group),
    auth_message_(_auth_message)
{
}

add_buddy::~add_buddy()
{
}

int32_t add_buddy::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::stringstream ss_url;

    ss_url << urls::get_url(urls::url_type::wim_host) << "buddylist/addBuddy" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&r=" <<  core::tools::system::generate_guid() <<
        "&buddy=" << escape_symbols(aimid_);

    if (!group_.empty())
        ss_url << "&group=" << escape_symbols(group_);

    ss_url << "&authorizationMsg=" << escape_symbols(auth_message_);
    ss_url << "&preAuthorized=1";

    _request->set_url(ss_url.str());
    _request->set_normalized_url(get_method());
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

std::string_view add_buddy::get_method() const
{
    return "addBuddy";
}

int core::wim::add_buddy::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t add_buddy::parse_response_data(const rapidjson::Value& /*_data*/)
{
    return 0;
}
