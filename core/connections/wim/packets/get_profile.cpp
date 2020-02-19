#include "stdafx.h"
#include "get_profile.h"

#include "../../../http_request.h"
#include "../../../../corelib/enumerations.h"
#include "../../../core.h"
#include "../../contact_profile.h"
#include "../../urls_cache.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

get_profile::get_profile(
    wim_packet_params _params,
    const std::string& _aimId)
    : wim_packet(std::move(_params)),
    aimId_(_aimId)
{
}


get_profile::~get_profile()
{

}

int32_t get_profile::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << urls::get_url(urls::url_type::wim_host) << "presence/get" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&mdir=1" <<
        "&t=" << escape_symbols(aimId_);

    _request->set_url(ss_url.str());
    _request->set_normalized_url("presenceGet");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

const profile::profiles_list& get_profile::get_result() const
{
    return search_result_;
}

int32_t get_profile::parse_response_data(const rapidjson::Value& _data)
{
    auto iter_users = _data.FindMember("users");
    if (iter_users == _data.MemberEnd() || !iter_users->value.IsArray())
        return wpie_http_parse_response;

    persons_ = std::make_shared<core::archive::persons_map>();
    for (auto iter = iter_users->value.Begin(), end = iter_users->value.End(); iter != end; ++iter)
    {
        auto iter_aimid = iter->FindMember("aimId");
        if (iter_aimid == iter->MemberEnd() || !iter_aimid->value.IsString())
            return wpie_http_parse_response;

        auto contact_profile = std::make_shared<profile::info>(rapidjson_get_string(iter_aimid->value));
        if (!contact_profile->unserialize(*iter))
        {
            assert(false);
            return wpie_http_parse_response;
        }

        if (const auto& friendly = contact_profile->get_friendly(); !friendly.empty())
        {
            archive::person p;
            p.friendly_ = friendly;
            p.official_ = contact_profile->get_official();

            persons_->emplace(contact_profile->get_aimid(), std::move(p));
        }

        search_result_.push_back(std::move(contact_profile));
    }

    return 0;
}
