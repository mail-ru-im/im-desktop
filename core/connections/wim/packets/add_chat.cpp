#include "stdafx.h"
#include "add_chat.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

add_chat::add_chat(
    wim_packet_params _params,
    const std::string& _m_chat_name,
    std::vector<std::string> _m_chat_members)
    :    wim_packet(std::move(_params)),
    m_chat_name(_m_chat_name),
    m_chat_members(std::move(_m_chat_members))
{
}

add_chat::~add_chat()
{
}

int32_t add_chat::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << urls::get_url(urls::url_type::wim_host) << "mchat/AddChat" <<
        "?f=json" <<
        "&chat_name="<< escape_symbols(m_chat_name) <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&r=" << core::tools::system::generate_guid() <<
        "&members=" << (*m_chat_members.begin());

    std::for_each(std::next(m_chat_members.begin()), m_chat_members.end(), [&ss_url](const std::string& item){ ss_url << ';' << item; });
    _request->set_url(ss_url.str());
    _request->set_normalized_url("addChat");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t add_chat::parse_response_data(const rapidjson::Value& /*_data*/)
{
    return 0;
}

int32_t add_chat::get_members_count() const
{
    return (int32_t)m_chat_members.size();
}
