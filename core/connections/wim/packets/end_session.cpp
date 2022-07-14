#include "stdafx.h"
#include "end_session.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"

#include "../../urls_cache.h"
#include "../common.shared/string_utils.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

end_session::end_session(wim_packet_params _params)
    : wim_packet(std::move(_params))
{
}

end_session::~end_session() = default;

std::string_view end_session::get_method() const
{
    return "endSession";
}

int core::wim::end_session::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t end_session::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::string url = su::concat(urls::get_url(urls::url_type::wim_host), "aim/endSession?f=json&aimsid=", escape_symbols(get_params().aimsid_), "&invalidateToken=1");

    _request->set_url(std::move(url));
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

int32_t end_session::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t end_session::execute_request(const std::shared_ptr<core::http_request_simple>& request)
{
    url_ = request->get_url();

    if (auto error_code = get_error(request->get()))
        return *error_code;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
    {
        if (http_code_ > 400 && http_code_ < 500)
            return on_http_client_error();

        return wpie_http_error;
    }

    return 0;
}
