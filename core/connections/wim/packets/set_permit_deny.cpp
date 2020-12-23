#include "stdafx.h"
#include "set_permit_deny.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

set_permit_deny::set_permit_deny(
    wim_packet_params _params,
    const std::string& _aimid,
    const set_permit_deny::operation _op)
    :    wim_packet(std::move(_params)),
        aimid_(_aimid),
        op_(_op)
{
    assert(op_ > operation::min);
    assert(op_ < operation::max);
    assert(!aimid_.empty());
}

set_permit_deny::~set_permit_deny()
{
}

std::string_view set_permit_deny::get_method() const
{
    return "setPreferencePermitDeny";
}

const char* operation_2_str(const set_permit_deny::operation _op)
{
    assert(_op > set_permit_deny::operation::min);
    assert(_op < set_permit_deny::operation::max);

    switch (_op)
    {
    case set_permit_deny::operation::ignore:
        return "pdIgnore";

    case set_permit_deny::operation::ignore_remove:
        return "pdIgnoreRemove";
    default:
        break;
    };

    assert(false);
    return "";
}

int32_t set_permit_deny::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::stringstream ss_url;

    ss_url << urls::get_url(urls::url_type::wim_host) << "preference/setPermitDeny" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&r=" << core::tools::system::generate_guid() <<
        '&' << operation_2_str(op_) << '=' << aimid_;

    if (op_ == set_permit_deny::operation::ignore_remove) // remove from 'blocks' too
        ss_url << "&pdBlockRemove=" << aimid_;

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

int32_t set_permit_deny::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
