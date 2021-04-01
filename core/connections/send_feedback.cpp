#include "stdafx.h"
#include "send_feedback.h"

#include "../core.h"
#include "../network_log.h"
#include "../http_request.h"
#include "../tools/system.h"
#include "../tools/strings.h"
#include "../../common.shared/config/config.h"
#include "../utils.h"

#include "zip.h"

namespace
{
    constexpr int64_t max_log_size() noexcept { return 1024 * 1024; } // 1 Mb per log file
}

using namespace core;

send_feedback::send_feedback(const std::string& _aimid, std::string _url, std::map<std::string, std::string> _fields, std::vector<std::string> _attachments)
    : url_(std::move(_url))
    , fields_(std::move(_fields))
    , attachments_(std::move(_attachments))

{
    if (auto& user_name = fields_["fb.user_name"]; user_name.empty())
        user_name = _aimid;

    fields_[std::string(config::get().string(config::values::feedback_aimid_id))] = _aimid;
}

send_feedback::~send_feedback() = default;

void send_feedback::init_request()
{
    log_ = utils::create_feedback_logs_archive({}, max_log_size()).wstring();

    // fill in post fields
    for (const auto& [name, value]: fields_)
        request_->push_post_form_parameter(name, value);
    for (const auto& a: attachments_)
        request_->push_post_form_filedata(L"fb.attachement", tools::from_utf8(a));
    if (core::tools::system::is_exist(log_))
        request_->push_post_form_filedata(L"fb.attachement", log_);
    request_->push_post_form_parameter("submit", "send");
    request_->push_post_form_parameter("r", core::tools::system::generate_guid());

    request_->set_url(url_);
    request_->set_normalized_url("help");
    request_->set_post_form(true);
}

void send_feedback::send(feedback_callback_t _callback)
{
    request_ = std::make_shared<core::http_request_simple>(g_core->get_proxy_settings(), utils::get_user_agent(), default_priority());
    init_request();

    request_->post_async([wr_this = weak_from_this(), _callback](curl_easy::completion_code _code) mutable
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        const auto success = (_code == curl_easy::completion_code::success && ptr_this->request_->get_response_code() == 200);
        g_core->execute_core_context({ [_callback = std::move(_callback), success] ()
        {
            _callback(success);
        } });

        if (success && !ptr_this->log_.empty())
        {
            auto to = boost::filesystem::wpath(ptr_this->log_);
            if (core::tools::system::is_exist(to))
            {
                boost::system::error_code e;
                boost::filesystem::remove(to, e);
            }
        }
    });
}
