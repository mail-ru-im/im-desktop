#include "stdafx.h"
#include "modify_chat.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

modify_chat::modify_chat(
    wim_packet_params _params,
    const std::string& _aimid_,
    const std::string&    _m_chat_name)
    :    wim_packet(std::move(_params)),
    m_chat_name(_m_chat_name),
    aimid_(_aimid_)
{
}

modify_chat::~modify_chat()
{
}

int32_t modify_chat::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << urls::get_url(urls::url_type::wim_host) << "mchat/ModifyChat" <<
        "?f=json" <<
        "&chat_name="<< escape_symbols(m_chat_name) <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&r=" << core::tools::system::generate_guid() <<
        "&chat_id=" << aimid_;

    _request->set_url(ss_url.str());
    _request->set_normalized_url("modifyChat");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t modify_chat::parse_response_data(const rapidjson::Value& /*_data*/)
{
    return 0;
}
