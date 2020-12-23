#include "stdafx.h"
#include "set_state.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

set_state::set_state(wim_packet_params _params, const profile_state _state)
    :
    wim_packet(std::move(_params)),
    state_(_state)
{
}

set_state::~set_state()
{
}

int32_t set_state::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::string_view state = "online";
    std::string_view invisible = "0";

    if (state_ == profile_state::offline)
        state = "offline";
    else if (state_ == profile_state::dnd)
        state = "occupied";
    else if (state_ == profile_state::away)
        state = "away";
    else if (state_ == profile_state::invisible)
        invisible = "1";

    std::stringstream ss_url;
    ss_url << urls::get_url(urls::url_type::wim_host) << "presence/setState" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&view=" << escape_symbols(state) <<
        "&r=" << core::tools::system::generate_guid() <<
        "&invisible=" << escape_symbols(invisible);

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

int32_t set_state::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

priority_t set_state::get_priority() const
{
    return top_priority();
}

std::string_view set_state::get_method() const
{
    return "presenceSetState";
}
