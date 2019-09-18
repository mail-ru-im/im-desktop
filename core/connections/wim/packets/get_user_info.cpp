#include "stdafx.h"

#include "../../../log/log.h"
#include "../../../http_request.h"

#include "get_user_info.h"

#include "../../urls_cache.h"

CORE_WIM_NS_BEGIN

get_user_info::get_user_info(
    wim_packet_params _params,
    const std::string &_contact_aimid
)
    : robusto_packet(std::move(_params))
    , contact_aimid_(_contact_aimid)
{
    assert(!contact_aimid_.empty());
}

user_info get_user_info::get_info() const
{
    return info_;
}

const std::shared_ptr<core::archive::persons_map>& get_user_info::get_persons() const
{
    return persons_;
}

int32_t get_user_info::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    assert(_request);

    constexpr char method[] = "getUserInfo";

    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", contact_aimid_, a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_user_info::parse_results(const rapidjson::Value& _data)
{
    const auto res = info_.unserialize(_data);

    if (!contact_aimid_.empty())
    {
        core::archive::person p;
        p.friendly_ = info_.friendly_;
        p.nick_ = info_.nick_;
        p.official_ = std::make_optional(info_.official_);

        persons_ = std::make_shared<core::archive::persons_map>();
        persons_->insert({ contact_aimid_, std::move(p) });
    }

    return res;
}

CORE_WIM_NS_END
