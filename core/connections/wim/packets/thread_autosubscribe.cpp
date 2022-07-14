#include "stdafx.h"
#include "thread_autosubscribe.h"

#include "../../../http_request.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

thread_autosubscribe::thread_autosubscribe(wim_packet_params _params, thread_autosubscribe_params _thread_params)
	: robusto_packet(std::move(_params))
	, params_(std::move(_thread_params))
{
}

int thread_autosubscribe::minimal_supported_api_version() const
{
	return core::urls::api_version::instance().minimal_supported();
}

int32_t thread_autosubscribe::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
	rapidjson::Document doc(rapidjson::Type::kObjectType);
	auto& a = doc.GetAllocator();

	rapidjson::Value node_params(rapidjson::Type::kObjectType);

	node_params.AddMember("chatId", params_.chat_id_, a);
	node_params.AddMember("enable", params_.enable_, a);
	node_params.AddMember("withExisting", params_.with_existing_, a);

	doc.AddMember("params", std::move(node_params), a);

	setup_common_and_sign(doc, a, _request, get_method());

	if (!robusto_packet::params_.full_log_)
	{
		log_replace_functor f;
		f.add_json_marker("aimsid", aimsid_range_evaluator());
		_request->set_replace_log_function(f);
	}

	return 0;
}

std::string_view thread_autosubscribe::get_method() const
{
	return "thread/autosubscribe";
}
