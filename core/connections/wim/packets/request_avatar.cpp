#include "stdafx.h"

#include "request_avatar.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

#include "../common.shared/string_utils.h"

using namespace core;
using namespace wim;

request_avatar::request_avatar(wim_packet_params params, const std::string& _contact, std::string_view _avatar_type, time_t _write_time)
    : wim_packet(std::move(params))
    , avatar_type_(_avatar_type)
    , contact_(_contact)
    , write_time_(_write_time)
{
}

request_avatar::~request_avatar() = default;

int32_t request_avatar::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::string url = su::concat(urls::get_url(urls::url_type::avatars), "/get?aimsid=", escape_symbols(get_params().aimsid_), "&targetSn=", escape_symbols(contact_), "&size=", avatar_type_);

    _request->set_normalized_url("avatarGet");
    _request->set_use_curl_decompresion(true);

    if (write_time_ != 0)
        _request->set_modified_time_condition(write_time_ - params_.time_offset_);

    _request->set_write_data_log(params_.full_log_);
    _request->set_need_log_original_url(false);
    _request->set_url(url);
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t core::wim::request_avatar::execute_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    // request for an empty contact in the old method returned code 404, but for now the new method returns 200 and the stub file (later replaced with code 400)
    // in any case, an empty contact request is an bad request, so immediately returns an error code without a request
    return contact_.empty() ? wpie_client_http_error : wim_packet::execute_request(_request);
}

int32_t core::wim::request_avatar::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    data_ = _response;

    return 0;
}

priority_t request_avatar::get_priority() const
{
    return packets_priority_high() + increase_priority();
}

std::string_view request_avatar::get_method() const
{
    return "avatarGet";
}

const std::shared_ptr<core::tools::binary_stream>& core::wim::request_avatar::get_data() const
{
    return data_;
}
