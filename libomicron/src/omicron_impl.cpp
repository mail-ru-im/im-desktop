#include "stdafx.h"

#ifdef OMICRON_USE_LIBCURL
#include <curl.h>
#include "curl_tools/curl_ssl_callbacks.h"
#endif

#include <omicron/omicron_conf.h>
#include <omicron/omicron.h>

#include "omicron_impl.h"

#ifdef OMICRON_USE_LIBCURL
static size_t write_data(void *ptr, size_t size, size_t nitems, void *stream)
{
    std::string data((const char*)ptr, size * nitems);
    *((std::stringstream*)stream) << data << std::endl;

    return size * nitems;
}

static bool internal_download_json(const omicronlib::omicron_proxy_settings& _proxy_settings, const std::string& _url, std::string& _json_data, long& _response_code)
{
    CURL* curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(curl, CURLOPT_PIPEWAIT, long(1));
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, omicronlib::ssl_ctx_callback);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");
    std::stringstream out;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);

    if (_proxy_settings.use_proxy_)
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, _proxy_settings.server_.c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE, (long)_proxy_settings.type_);
        curl_easy_setopt(curl, CURLOPT_PROXYPORT, (long)_proxy_settings.port_);

        if (_proxy_settings.need_auth_)
        {
            std::string auth_params = _proxy_settings.login_ + ':' + _proxy_settings.password_;
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, auth_params.c_str());
        }
    }

    if (omicronlib::build::is_debug())
        std::cout << "GET request: " << _url << "\n\n";

    CURLcode res = curl_easy_perform(curl);

    long code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    _response_code = code;

    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        return false;

    _json_data = out.str();

    if (omicronlib::build::is_debug())
        std::cout << "GET response (" << _response_code << "): " << _json_data << "\n\n";

    return true;
}
#endif

namespace omicronlib
{
    omicron_impl& omicron_impl::instance()
    {
        static std::shared_ptr<omicron_impl> instance;

        if (!instance)
            instance.reset(new omicron_impl());

        return *instance;
    }

    omicron_code omicron_impl::init(const omicron_config& _conf, const std::wstring& _file_name)
    {
        config_ = make_unique<omicron_config>(_conf);
        file_name_ = _file_name;
        logger_func_ = config_->get_logger();
        callback_updater_func_ = config_->get_callback_updater();

#ifdef OMICRON_USE_LIBCURL
        if (config_->get_json_downloader() == nullptr)
            config_->set_json_downloader(internal_download_json);
#endif

        if (load())
            callback_updater();

        return update_data();
    }

    bool omicron_impl::star_auto_updater()
    {
        std::weak_ptr<omicron_impl> wr_this = shared_from_this();
        constexpr auto tick_timeout = std::chrono::seconds(internal_tick_timeout());

        schedule_thread_ = make_unique<std::thread>([wr_this, tick_timeout]()
        {
            bool fl_once = true;
            for (;;)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (fl_once)
                {
                    tools::set_this_thread_name("omicron_scheduler");
                    ptr_this->write_to_log("started update service");
                    fl_once = false;
                }

                std::unique_lock<std::mutex> lock(ptr_this->schedule_mutex_);
                const auto wait_res = ptr_this->schedule_condition_.wait_for(lock, tick_timeout);
                const auto current_time = std::chrono::system_clock::now();
                const auto update_interval = std::chrono::seconds(ptr_this->config_->get_update_interval());

                if (ptr_this->is_schedule_stop_)
                    return;

                if (wait_res == std::cv_status::timeout)
                    if (current_time - ptr_this->last_update_time_ >= update_interval)
                        ptr_this->update_data();
            }
        });

        if (!schedule_thread_->joinable())
            return false;

        return true;
    }

    omicron_code omicron_impl::update_data()
    {
        if (is_updating_)
            return OMICRON_ALREADY_UPDATING;

        is_updating_ = true;
        last_update_status_ = OMICRON_IS_UPDATING;
        last_update_time_ = std::chrono::system_clock::now();

        std::weak_ptr<omicron_impl> wr_this = shared_from_this();
        std::thread update_thread([wr_this]()
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            tools::set_this_thread_name("omicron_update");

            ptr_this->last_update_status_ = ptr_this->get_omicron_data();
            ptr_this->last_update_time_ = std::chrono::system_clock::now();
            ptr_this->is_updating_ = false;
        });
        update_thread.detach();

        return OMICRON_OK;
    }

    void omicron_impl::cleanup()
    {
        is_schedule_stop_ = true;
        is_json_data_init_ = false;

        schedule_condition_.notify_all();
        if (schedule_thread_)
        {
            if (schedule_thread_->joinable())
            {
                schedule_thread_->join();
                write_to_log("stopped update service");
            }
        }
        schedule_thread_.reset();

        config_.reset();
        json_data_.reset();

        last_update_status_ = OMICRON_NOT_INITIALIZED;
    }

    time_t omicron_impl::get_last_update_time() const
    {
        return std::chrono::system_clock::to_time_t(last_update_time_);
    }

    omicron_code omicron_impl::get_last_update_status() const
    {
        return last_update_status_;
    }

    void omicron_impl::add_fingerprint(std::string _name, std::string _value)
    {
        if (config_)
            config_->add_fingerprint(std::move(_name), std::move(_value));
    }

    bool omicron_impl::bool_value(const char* _key_name, bool _default_value) const
    {
        if (is_json_data_init_)
        {
            auto json_data = get_json_data();
            auto it = json_data->FindMember(_key_name);
            if (it != json_data->MemberEnd() && it->value.IsBool())
                return it->value.GetBool();
        }

        return _default_value;
    }

    int omicron_impl::int_value(const char* _key_name, int _default_value) const
    {
        if (is_json_data_init_)
        {
            auto json_data = get_json_data();
            auto it = json_data->FindMember(_key_name);
            if (it != json_data->MemberEnd() && it->value.IsInt())
                return it->value.GetInt();
        }

        return _default_value;
    }

    unsigned int omicron_impl::uint_value(const char* _key_name, unsigned int _default_value) const
    {
        if (is_json_data_init_)
        {
            auto json_data = get_json_data();
            auto it = json_data->FindMember(_key_name);
            if (it != json_data->MemberEnd() && it->value.IsInt())
                return it->value.GetUint();
        }

        return _default_value;
    }

    int64_t omicron_impl::int64_value(const char* _key_name, int64_t _default_value) const
    {
        if (is_json_data_init_)
        {
            auto json_data = get_json_data();
            auto it = json_data->FindMember(_key_name);
            if (it != json_data->MemberEnd() && it->value.IsInt64())
                return it->value.GetInt64();
        }

        return _default_value;
    }

    uint64_t omicron_impl::uint64_value(const char* _key_name, uint64_t _default_value) const
    {
        if (is_json_data_init_)
        {
            auto json_data = get_json_data();
            auto it = json_data->FindMember(_key_name);
            if (it != json_data->MemberEnd() && it->value.IsInt64())
                return it->value.GetUint64();
        }

        return _default_value;
    }

    double omicron_impl::double_value(const char* _key_name, double _default_value) const
    {
        if (is_json_data_init_)
        {
            auto json_data = get_json_data();
            auto it = json_data->FindMember(_key_name);
            if (it != json_data->MemberEnd() && it->value.IsDouble())
                return it->value.GetDouble();
        }

        return _default_value;
    }

    std::string omicron_impl::string_value(const char* _key_name, std::string _default_value) const
    {
        if (is_json_data_init_)
        {
            auto json_data = get_json_data();
            auto it = json_data->FindMember(_key_name);
            if (it != json_data->MemberEnd() && it->value.IsString())
                return std::string(it->value.GetString(), it->value.GetStringLength());
        }

        return _default_value;
    }

    json_string omicron_impl::json_value(const char* _key_name, json_string _default_value) const
    {
        if (is_json_data_init_)
        {
            auto json_data = get_json_data();
            auto it = json_data->FindMember(_key_name);
            if (it != json_data->MemberEnd() && (it->value.IsObject() || it->value.IsArray()))
            {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                it->value.Accept(writer);
                return json_string(buffer.GetString(), buffer.GetSize());
            }
        }

        return _default_value;
    }

    omicron_impl::omicron_impl()
        : is_json_data_init_(false)
        , json_data_(std::make_shared<rapidjson::Document>(rapidjson::kObjectType))
        , is_schedule_stop_(false)
        , is_updating_(false)
        , last_update_time_(std::chrono::system_clock::now())
        , last_update_status_(OMICRON_NOT_INITIALIZED)
        , logger_func_(nullptr)
        , callback_updater_func_(nullptr)
    {
    }

    bool omicron_impl::load()
    {
        if (!tools::is_exist(file_name_))
            return false;

        auto ifile = tools::open_file_for_read(file_name_);
        if (!ifile.is_open())
            return false;

        std::stringstream data;
        data << ifile.rdbuf();
        ifile.close();

        if (!parse_json(data.str()))
            return false;

        std::stringstream log;
        log << "loaded data from the cache:\n";
        log << data.str();
        write_to_log(log.str());

        return true;
    }

    bool omicron_impl::save(const std::string& _data) const
    {
        if (!tools::create_parent_directories_for_file(file_name_))
            return false;

        auto temp_file_name = file_name_ + L".tmp";
        auto ofile = tools::open_file_for_write(temp_file_name, std::ofstream::trunc);
        if (!ofile.is_open())
            return false;

        if (!_data.empty())
            ofile << _data;

        ofile.flush();
        ofile.close();

        tools::rename_file(temp_file_name, file_name_);

        return true;
    }

    std::shared_ptr<const rapidjson::Document> omicron_impl::get_json_data() const
    {
        std::lock_guard<spin_lock> lock(data_spin_lock_);

        return json_data_;
    }

    std::string omicron_impl::get_json_data_string() const
    {
        if (is_json_data_init_)
        {
            auto json_data = get_json_data();

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json_data->Accept(writer);

            return std::string(buffer.GetString(), buffer.GetSize());
        }

        return "{}";
    }

    omicron_code omicron_impl::get_omicron_data(bool _is_save)
    {
        if (!config_)
            return OMICRON_NOT_INITIALIZED;

        auto downloader = config_->get_json_downloader();
        if (downloader == nullptr)
            return OMICRON_DOWNLOAD_NOT_IMPLEMENTED;

        std::string data;
        long code;
        auto request_str = config_->generate_request_string();
        write_to_log("send request:\n" + request_str);
        auto download_res = downloader(config_->get_proxy_settings(), request_str, data, code);
        if (!download_res || (code != 200 && code != 304))
        {
            write_to_log("request failed with HTTP status " + std::to_string(code));
            return OMICRON_DOWNLOAD_FAILED;
        }
        if (code == 200)
        {
            if (!parse_json(data))
            {
                write_to_log("get wrong data:\n" + data);
                return OMICRON_WRONG_JSON_DATA;
            }

            write_to_log("get new data:\n" + data);

            if (_is_save)
            {
                std::weak_ptr<omicron_impl> wr_this = shared_from_this();

                std::thread save_thread([wr_this, data = std::move(data)]()
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    tools::set_this_thread_name("omicron_save");

                    if (!ptr_this->save(data))
                        ptr_this->write_to_log("failure to write data to disk");
                });
                save_thread.detach();
            }

            callback_updater();
        }
        else
        {
            write_to_log("data is not changed");
        }

        return OMICRON_OK;
    }

    bool omicron_impl::parse_json(const std::string& _json)
    {
        rapidjson::Document doc;
        const auto& parse_result = doc.Parse(_json.c_str());
        if (parse_result.HasParseError())
            return false;

        auto new_config = make_unique<omicron_config>(*config_);

        auto it_version = doc.FindMember("config_v");
        if (it_version == doc.MemberEnd() || !it_version->value.IsInt())
            return false;

        new_config->set_config_v(it_version->value.GetInt());

        auto it_condition = doc.FindMember("cond_s");
        if (it_condition == doc.MemberEnd() || !it_condition->value.IsString())
            return false;

        new_config->set_cond_s(std::string(it_condition->value.GetString(), it_condition->value.GetStringLength()));

        new_config->reset_segments();
        auto it_segments = doc.FindMember("segments");
        if (it_segments != doc.MemberEnd() && it_segments->value.IsObject())
        {
            for (auto it = it_segments->value.MemberBegin(), end = it_segments->value.MemberEnd(); it != end; ++it)
            {
                if (it->name.IsString() && it->value.IsString())
                    new_config->add_segment(std::string(it->name.GetString(), it->name.GetStringLength()),
                                            std::string(it->value.GetString(), it->value.GetStringLength()));
            }
        }

        auto it_config = doc.FindMember("config");
        if (it_config == doc.MemberEnd() || !it_config->value.IsObject())
            return false;

        auto new_json_data = std::make_shared<rapidjson::Document>(rapidjson::kObjectType);
        new_json_data->CopyFrom(it_config->value, new_json_data->GetAllocator());

        config_.swap(new_config);

        {
            std::lock_guard<spin_lock> lock(data_spin_lock_);
            json_data_.swap(new_json_data);
        }

        if (!is_json_data_init_)
            is_json_data_init_ = true;

        return true;
    }

    void omicron_impl::write_to_log(const std::string& _text) const
    {
        if (logger_func_)
        {
            std::stringstream log;
            log << "omicron: " << _text << "\r\n";
            logger_func_(log.str());
        }
    }

    void omicron_impl::callback_updater()
    {
        if (callback_updater_func_)
            callback_updater_func_(get_json_data_string());
    }
}
