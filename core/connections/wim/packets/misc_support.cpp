#include "stdafx.h"
#include "misc_support.h"

#include "../../../../common.shared/version_info.h"
#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../tools/json_helper.h"
#include "../../../utils.h"


using namespace core;
using namespace wim;

misc_support::misc_support(wim_packet_params _params, std::string_view _aimid, std::string_view _message, std::string_view _attachment_file_id, std::vector<std::string> _screenshot_file_id_list)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , message_(_message)
    , attachment_file_id_(_attachment_file_id)
    , screenshot_file_id_list_(std::move(_screenshot_file_id_list))
{
}

std::string_view misc_support::get_method() const
{
    return "misc/support";
}

int32_t misc_support::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    node_params.AddMember("message", message_, a);

    node_params.AddMember("platform", "desktop", a);

    node_params.AddMember("appVersion", tools::version_info().get_ua_version(), a);
    node_params.AddMember("osVersion", utils::get_ua_os_version(), a);
    node_params.AddMember("device", "PC", a);
    node_params.AddMember("attachmentFileId", attachment_file_id_, a);

    if (const auto screenshot_list_size = screenshot_file_id_list_.size())
    {
        rapidjson::Value screenshots(rapidjson::Type::kArrayType);
        screenshots.Reserve(screenshot_list_size, a);
        for (const auto& s : screenshot_file_id_list_)
            screenshots.PushBack(tools::make_string_ref(s), a);
        node_params.AddMember("screenshotFileIdList", std::move(screenshots), a);
    }

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method(), use_aimsid::no);

    return 0;
}
