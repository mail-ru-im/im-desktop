#include "stdafx.h"
#include "set_avatar.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../tools/json_helper.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

set_avatar::set_avatar(wim_packet_params _params, tools::binary_stream _image, const std::string& _aimid, const bool _chat)
    : wim_packet(std::move(_params))
    , aimid_(_aimid)
    , chat_(_chat)
    , image_(_image)
{
}

set_avatar::~set_avatar()
{
}

int32_t set_avatar::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    if (is_new_avatar_rapi())
    {
        ss_url << urls::get_url(urls::url_type::rapi_host) << "avatar/set?"
            << "aimsid=" << escape_symbols(get_params().aimsid_);

        if (!chat_ && !aimid_.empty())
            ss_url << "&targetSn=" << escape_symbols(aimid_);
        else if (chat_)
            ss_url << "&beforehandUpload=1";

        _request->set_normalized_url("avatarSet");
        _request->set_custom_header_param("Content-Type: multipart/form-data");
        _request->set_post_form(true);
        _request->push_post_form_filedata("image", image_);
    }
    else
    {
        ss_url << urls::get_url(urls::url_type::wim_host) << "expressions/upload"
            << "?f=json"
            << "&aimsid=" << escape_symbols(get_params().aimsid_)
            << "&r=" << core::tools::system::generate_guid()
            << "&type=largeBuddyIcon";

        if (!chat_ && !aimid_.empty())
            ss_url << "&t=" << escape_symbols(aimid_);
        else if (chat_)
            ss_url << "&livechat=1";

        _request->set_gzip(true);
        _request->set_normalized_url("setAvatar");
        _request->set_custom_header_param("Content-Type: application/octet-stream");
        const auto size = image_.available();
        _request->set_post_data(image_.read_available(), size, true);
    }
    image_.reset_out();

    _request->set_need_log(params_.full_log_);
    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t set_avatar::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    if (is_new_avatar_rapi())
    {
        // avatar id is returned in the response header as X-Avatar-Hash
        if (std::string val = request->get_header_attribute("X-Avatar-Hash"); !val.empty())
            id_ = std::move(val);
        else
            return wpie_http_empty_response;
    }

    return 0;
}

int32_t set_avatar::parse_response(std::shared_ptr<core::tools::binary_stream> response)
{
    return is_new_avatar_rapi() ? 0 : wim_packet::parse_response(response);
}

int32_t set_avatar::parse_response_data(const rapidjson::Value& _data)
{
    if (!is_new_avatar_rapi())
        tools::unserialize_value(_data, "id", id_);

    return 0;
}
