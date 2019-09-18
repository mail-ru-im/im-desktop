#include "stdafx.h"
#include "set_attention_attribute.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

set_attention_attribute::set_attention_attribute(wim_packet_params _params, std::string _contact, bool _value)
    : wim_packet(std::move(_params)),
    contact_(_contact),
    value_(_value)
{
    assert(!contact_.empty());
}

set_attention_attribute::~set_attention_attribute()
{
}

int32_t set_attention_attribute::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;
    ss_url << urls::get_url(urls::url_type::wim_host) << "recently/setAttentionAttribute?aimsid=" << escape_symbols(get_params().aimsid_)
        << "&sn=" << escape_symbols(contact_)
        << "&r=" << core::tools::system::generate_guid()
        << "&value=" << (value_ ? "true" : "false");

    _request->set_url(ss_url.str());
    _request->set_normalized_url("setAttentionAttribute");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t set_attention_attribute::parse_response(std::shared_ptr<core::tools::binary_stream> /*_response*/)
{
    return 0;
}