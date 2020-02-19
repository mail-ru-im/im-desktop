#include "stdafx.h"
#include "send_feedback.h"

#include "../../../core.h"
#include "../../../network_log.h"
#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../tools/system.h"
#include "../../../tools/strings.h"
#include "../common.shared/config/config.h"

#include "zip.h"

using namespace core;
using namespace wim;

send_feedback::send_feedback(wim_packet_params _params, const std::string &url, const std::map<std::string, std::string>& fields, const std::vector<std::string>& attachments)
    : wim_packet(_params)
    , url_(url)
    , result_(0)
{
    fields_.insert(fields.begin(), fields.end());
    attachments_.assign(attachments.begin(), attachments.end());

    if (fields_.find("fb.user_name") == fields_.end() || fields_["fb.user_name"].empty())
        fields_["fb.user_name"] = _params.aimid_;

    fields_[std::string(config::get().string(config::values::feedback_aimid_id))] = _params.aimid_;
}

send_feedback::~send_feedback() = default;

int32_t send_feedback::init_request(std::shared_ptr<core::http_request_simple> _request)
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
        _request->push_post_form_parameter(name, value);
    for (const auto& a: attachments_)
        _request->push_post_form_filedata(L"fb.attachement", tools::from_utf8(a));
    if (core::tools::system::is_exist(log_))
        _request->push_post_form_filedata(L"fb.attachement", log_);
    _request->push_post_form_parameter("submit", "send");
    _request->push_post_form_parameter("r", core::tools::system::generate_guid());

    _request->set_url(url_);
    _request->set_normalized_url("help");
    _request->set_post_form(true);

    return 0;
}

int32_t send_feedback::parse_response(std::shared_ptr<core::tools::binary_stream> response)
{
    return 0;
}

int32_t send_feedback::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();
    if (http_code_ != 200)
        return wpie_http_error;

    auto to = boost::filesystem::wpath(log_);
    if (core::tools::system::is_exist(to))
    {
        boost::system::error_code e;
        boost::filesystem::remove(to, e);
    }

    return 0;
}
