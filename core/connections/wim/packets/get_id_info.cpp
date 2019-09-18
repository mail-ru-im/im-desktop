#include "stdafx.h"

#include "get_id_info.h"
#include "http_request.h"
#include "../../urls_cache.h"

using namespace core::wim;

get_id_info::get_id_info(core::wim::wim_packet_params _params, const std::string_view _id)
    : robusto_packet(std::move(_params))
    , id_(_id)
{
    assert(!_id.empty());
}

const id_info_response& get_id_info::get_response() const
{
    return response_;
}

const std::shared_ptr<core::archive::persons_map>& core::wim::get_id_info::get_persons() const
{
    return persons_;
}

int32_t get_id_info::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    constexpr char method[] = "getIdInfo";

    _request->set_gzip(true);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("id", id_, a);

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

int32_t get_id_info::parse_results(const rapidjson::Value& _node_results)
{
    const auto res = response_.unserialize(_node_results);

    if (response_.type_ != id_info_response::id_type::invalid)
    {
        core::archive::person p;
        p.friendly_ = response_.name_;
        p.nick_ = response_.nick_;

        persons_ = std::make_shared<core::archive::persons_map>();
        persons_->insert({ response_.sn_, std::move(p) });
    }

    return res;
}
