#include "stdafx.h"

#include "statistics.h"
#include "../errors.h"

#include "../../common.shared/version_info_constants.h"
#include "../../common.shared/im_statistics.h"
#include "../../common.shared/json_unserialize_helpers.h"
#include "../../common.shared/config/config.h"
#include "../../common.shared/common_defs.h"

#include "../../external/curl/include/curl.h"
#include "curl_utils.h"

#include <rapidjson/writer.h>

namespace
{
    static std::string_view error_to_stat_name(const installer::error& _error)
    {
        const auto stat_event = int(core::stats::im_stat_event_names::installer_ok) + _error.code();
        return core::stats::im_stat_event_to_string(core::stats::im_stat_event_names(stat_event));
    }

    std::string device_id() // some duplicate core/tools/deice_id.cpp
    {
        const auto d = common::get_win_device_id();
        const auto res = QCryptographicHash::hash(QByteArray::fromRawData(d.data(), int(d.size())), QCryptographicHash::Sha256);
        return std::string(res.data(), res.size() / 2);
    }

    std::string make_data(const installer::error& _error, std::chrono::system_clock::time_point)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        rapidjson::Value node_general(rapidjson::Type::kObjectType);

        const auto version = QString::fromUtf8(VER_FILEVERSION_STR);
        const auto splitted = version.splitRef(ql1c('.'), QString::SkipEmptyParts);

        if (splitted.size() > 1)
            node_general.AddMember("app_ver", QString(splitted.at(0) % ql1c('.') % splitted.at(1)).toStdString(), a);
        if (splitted.size() > 2)
            node_general.AddMember("app_build", splitted.at(2).toString().toStdString(), a);
        node_general.AddMember("developer_id", common::json::make_string_ref(config::get().string(config::values::dev_id_win)), a);
        node_general.AddMember("os_version", common::get_win_os_version_string(), a);
        node_general.AddMember("device_id", device_id(), a);
        doc.AddMember("general", std::move(node_general), a);

        rapidjson::Value node_events(rapidjson::Type::kArrayType);
        node_events.Reserve(1, a);

        rapidjson::Value node_event(rapidjson::Type::kObjectType);
        node_event.AddMember("name", common::json::make_string_ref(error_to_stat_name(_error)), a);
        if (const auto& text = _error.text(); !text.isEmpty())
        {
            rapidjson::Value node_prop(rapidjson::Type::kObjectType);
            node_prop.AddMember(common::json::make_string_ref("text"), text.toStdString(), a);
            node_event.AddMember("params", std::move(node_prop), a);
        }
        node_events.PushBack(std::move(node_event), a);

        doc.AddMember("events", std::move(node_events), a);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        return common::json::rapidjson_get_string(buffer);
    }

    std::string get_url()
    {
        std::string res;
        res += "https://";
        res += config::get().url(config::urls::stat_base);
        res += "/srp/clientStat";
        return res;
    }

    struct Curl_result
    {
        CURLcode res = CURLcode::CURLE_OK;
        long response_code = 0;
    };

    Curl_result send_impl(std::string_view data)
    {
        constexpr std::chrono::milliseconds timeout = std::chrono::seconds(5);

        CURLcode res;
        long response_code = 0;

        curl_global_init(CURL_GLOBAL_ALL);

        auto curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config::get().is_on(config::features::ssl_verify_peer) ? 1L : 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, curl_utils::ssl_ctx_callback_impl);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
            curl_easy_setopt(curl, CURLOPT_PIPEWAIT, long(1));
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout.count());
            curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
            curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, timeout.count() / 1000);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
            struct curl_slist *chunk = nullptr;

            chunk = curl_slist_append(chunk, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, long(data.size()));
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.data());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);

            curl_easy_setopt(curl, CURLOPT_URL, get_url().c_str());

            res = curl_easy_perform(curl);
            curl_slist_free_all(chunk);
            if (res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

            curl_easy_cleanup(curl);

        }
        curl_global_cleanup();

        return { res, response_code };
    }

    bool is_ssl_related_error(CURLcode code)
    {
        switch (code)
        {
        case CURLE_SSL_CERTPROBLEM:
        case CURLE_PEER_FAILED_VERIFICATION:
        case CURLE_SSL_ISSUER_ERROR:
            return true;
        default:
            return false;
        }
    }

    void send(const installer::error& _error, std::chrono::system_clock::time_point _start_time)
    {
        const auto data = make_data(_error, _start_time);

        for (int i = 0; i < 2; ++i)
        {
            const auto res = send_impl(data);
            if (res.res == CURLE_OK || !is_ssl_related_error(res.res))
                return;
        }
    }
}

namespace installer::statisctics
{
    class SendTaskImpl
    {
    public:
        error error;
        std::chrono::system_clock::time_point start_time;
    };
}

installer::statisctics::SendTask::SendTask(const error& _error, std::chrono::system_clock::time_point _start_time)
    : impl_(std::make_unique<SendTaskImpl>(SendTaskImpl{ _error, _start_time }))
{
}

installer::statisctics::SendTask::~SendTask() = default;

void installer::statisctics::SendTask::run()
{
    send(impl_->error, impl_->start_time);
}
