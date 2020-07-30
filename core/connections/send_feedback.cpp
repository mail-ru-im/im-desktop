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
    log_.clear();

    const long sizeMax = (1024 * 1024); // 1 Mb per log file
    long sizeLeft = sizeMax;
    std::stack<std::vector<char>> logChunks;
    std::vector<char> fullLog;

    auto logFilenames = g_core->network_log_file_names_history_copy();
    while (!logFilenames.empty())
    {
        auto logFilename = logFilenames.top();
        auto logPath = boost::filesystem::wpath(logFilename);
        if (core::tools::system::is_exist(logFilename))
        {
            // prepare output
            if (log_.empty())
            {
                log_ = logPath.parent_path().append("feedbacklog.zip").wstring();
                auto l = boost::filesystem::wpath(log_);
                if (core::tools::system::is_exist(log_))
                {
                    boost::system::error_code e;
                    boost::filesystem::remove(l, e);
                }
            }

            // capture log file data
            boost::system::error_code e;
            auto logTmp = logPath.parent_path().append(L"feedbacklog.processing.tmp");
            boost::filesystem::copy_file(logPath, logTmp, boost::filesystem::copy_option::overwrite_if_exists, e);
            const long sizeTmp = boost::filesystem::file_size(logTmp, e);
            std::ifstream ifs(logTmp.string(), std::ios::binary);
            const long amountToRead = ((sizeTmp - sizeLeft) < 0 ? sizeTmp : sizeLeft);
            if (amountToRead > 0)
            {
                std::vector<char> data;
                data.resize(amountToRead);
                ifs.seekg(-((std::fstream::off_type)amountToRead), std::ios::end);
                ifs.read(&data[0], amountToRead);
                logChunks.push(data);
            }
            ifs.close();
            boost::filesystem::remove(logTmp, e);

            // recalc max possible size of data to send
            sizeLeft -= amountToRead;
            if (!sizeLeft)
                break;
        }
        logFilenames.pop();
    }
    fullLog.resize(sizeMax - sizeLeft);

    // accumulate data
    {
        long previousSize = 0;
        while (!logChunks.empty())
        {
            auto chunk = logChunks.top();
            std::copy(chunk.begin(), chunk.end(), fullLog.begin() + previousSize);
            previousSize = chunk.size();
            logChunks.pop();
        }
    }

    // zip output and remove txt
    if (!fullLog.empty())
    {
        auto logFile = boost::filesystem::wpath(log_);
        auto zipFile = zipOpen(logFile.string().c_str(), APPEND_STATUS_CREATE);
        if (zipFile)
        {
            auto succeeded = true;
            zip_fileinfo zipInfo = {};
            auto stemmed = logFile.stem();
            stemmed += L".txt";
            if (zipOpenNewFileInZip(zipFile, stemmed.string().c_str(), &zipInfo, 0, 0, 0, 0, 0, Z_DEFLATED, Z_DEFAULT_COMPRESSION) == ZIP_OK)
            {
                succeeded &= (zipWriteInFileInZip(zipFile, &fullLog[0], (uint32_t)fullLog.size()) == ZIP_OK);
                succeeded &= (zipCloseFileInZip(zipFile) == ZIP_OK);
                succeeded &= (zipClose(zipFile, 0) == ZIP_OK);
            }
            if (!succeeded)
            {
                boost::system::error_code e;
                boost::filesystem::remove(logFile, e);
            }
        }
    }

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
        g_core->execute_core_context([_callback = std::move(_callback), success]()
        {
            _callback(success);
        });

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
