#include "stdafx.h"
#include "hide_chat.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

hide_chat::hide_chat(wim_packet_params _params, const std::string& _aimid, int64_t _last_msg_id)
    :
    wim_packet(std::move(_params)),
    aimid_(_aimid),
    last_msg_id_(_last_msg_id)
{
}


hide_chat::~hide_chat()
{

}

std::string_view hide_chat::get_method() const
{
    return "hideChat";
}

int32_t hide_chat::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::stringstream ss_url;
    ss_url << urls::get_url(urls::url_type::wim_host) << "buddylist/hideChat" <<
        "?f=json"<<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&buddy=" << escape_symbols(aimid_) <<
        "&r=" << core::tools::system::generate_guid() <<
        "&lastMsgId=" << last_msg_id_;

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

int32_t hide_chat::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t hide_chat::on_empty_data()
{
    return 0;
}
