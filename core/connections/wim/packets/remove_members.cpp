#include "stdafx.h"
#include "remove_members.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

remove_members::remove_members(
    wim_packet_params _params,
    const std::string& _aimid,
    const std::string& _m_chat_members_to_remove)
    :    wim_packet(std::move(_params)),
    aimid_(_aimid),
    m_chat_members_to_remove(_m_chat_members_to_remove)
{
}

remove_members::~remove_members()
{
}

int32_t remove_members::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << urls::get_url(urls::url_type::wim_host) << "mchat/DelMembers" <<
        "?f=json" <<
        "&chat_id=" <<  aimid_ <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&r=" << core::tools::system::generate_guid() <<
        "&members=" << m_chat_members_to_remove;

    _request->set_url(ss_url.str());
    _request->set_normalized_url("delMembers");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t remove_members::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
