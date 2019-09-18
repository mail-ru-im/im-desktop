#include "stdafx.h"
#include "send_message_typing.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

send_message_typing::send_message_typing(wim_packet_params _params, const std::string& _contact, const core::typing_status& _status, const std::string& _id)
    :
    wim_packet(std::move(_params)),
    contact_(_contact),
    id_(_id),
    status_(_status)
{
}

send_message_typing::~send_message_typing()
{
}

bool send_message_typing::support_async_execution() const
{
    return true;
}

int32_t send_message_typing::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::string_view status;
    switch (status_)
    {
        case core::typing_status::typing:
            status = "typing";
            break;
        case core::typing_status::typed:
            status = "typed";
            break;
        case core::typing_status::looking:
            status = "looking";
            break;
        default:
            status = "none";
            break;
    }

    std::stringstream ss_url;
    ss_url << urls::get_url(urls::url_type::wim_host) << "im/setTyping" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&t=" << escape_symbols(contact_) <<
        "&r=" <<  core::tools::system::generate_guid() <<
        "&typingStatus=" << status;

    if (status_ == core::typing_status::looking && !id_.empty())
        ss_url << "&chatHeadsSubscriptionId=" << id_;

    _request->set_url(ss_url.str());
    _request->set_normalized_url("setTyping");
    _request->set_keep_alive();
    _request->set_connect_timeout(std::chrono::seconds(5));

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t send_message_typing::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
