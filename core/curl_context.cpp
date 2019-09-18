#include "stdafx.h"

#include "tools/system.h"
#include "tools/strings.h"
#include "tools/binary_stream.h"

#include "core.h"
#include "curl_handler.h"
#include "network_log.h"
#include "statistics.h"

#include "curl_context.h"

#include <openssl/ssl.h>

#include "curl_tools/curl_ssl_callbacks.h"

#include "connections/urls_cache.h"
#include "configuration/app_config.h"
#include "../libomicron/include/omicron/omicron.h"


const size_t MAX_LOG_DATA_SIZE = 10 * 1024 * 1024;
const static auto FLUSH_STATS_TO_CORE_TIMEOUT =
        #if defined(DEBUG)
            std::chrono::seconds(30);
        #else
            std::chrono::minutes(3);
        #endif
const static auto SAVE_AGGREGATED_STATS_INTERVAL =
        #if defined(DEBUG)
            std::chrono::seconds(10);
        #else
            FLUSH_STATS_TO_CORE_TIMEOUT;
        #endif

static size_t write_header_function(void *contents, size_t size, size_t nmemb, void *userp);
static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);
static int32_t progress_callback(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded);
static int32_t trace_function(CURL* _handle, curl_infotype _type, unsigned char* _data, size_t _size, void* _userp);

namespace
{
    unsigned long curl_auth_type(core::proxy_auth _type)
    {
        switch (_type)
        {
            case core::proxy_auth::basic:
                return CURLAUTH_BASIC;
            case core::proxy_auth::digest:
                return CURLAUTH_DIGEST;
            case core::proxy_auth::negotiate:
                return CURLAUTH_NEGOTIATE;
            case core::proxy_auth::ntlm:
                return CURLAUTH_NTLM;
        }

        return CURLAUTH_BASIC;
    }
}

struct stats_aggregator
{
    struct stat_delta
    {
        long total_uploaded_ = 0;
        long total_downloaded_ = 0;

        stat_delta reset()
        {
            auto res = *this;

            total_uploaded_ = 0;
            total_downloaded_ = 0;

            return res;
        }
    };

    stats_aggregator(const std::chrono::seconds& _release_duration)
        : release_duration_(_release_duration)
    {
    }

    void add_uploaded(long _upl)
    {
        delta_.total_uploaded_ += _upl;
    }

    void add_downloaded(long _dnl)
    {
        delta_.total_downloaded_ += _dnl;
    }

    bool need_release() const
    {
        std::chrono::seconds secs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - last_release_);
        return secs > release_duration_;
    }

    stat_delta release()
    {
        last_release_ = std::chrono::system_clock::now();
        return delta_.reset();
    }

private:
    std::chrono::system_clock::time_point last_release_ = std::chrono::system_clock::now();
    std::chrono::seconds release_duration_;
    stat_delta delta_;
};

core::curl_context::curl_context(std::shared_ptr<tools::stream> _output, http_request_simple::stop_function _stop_func, http_request_simple::progress_function _progress_func, bool _keep_alive)
    : stop_func_(_stop_func),
    progress_func_(_progress_func),
    output_(_output),
    header_(std::make_shared<core::tools::binary_stream>()),
    log_data_(std::make_shared<core::tools::binary_stream>()),
    bytes_transferred_pct_(0),
    connect_timeout_(0),
    timeout_(0),
    priority_(100),
    need_log_(true),
    write_data_log_(true),
    need_log_original_url_(false),
    keep_alive_(_keep_alive),
    curl_post_(false),
    curl_http_post_(false),
    header_chunk_(nullptr),
    post(nullptr),
    last(nullptr),
    response_code_(0),
    request_time_(0),
    curl_range_from_(-1),
    curl_range_to_(-1),
    post_data_(nullptr),
    post_data_size_(0),
    free_post_data_(false),
    modified_time_(-1),
    id_(-1),
    max_log_data_size_reached_(false),
    is_send_im_stats_(true),
    multi_(false),
    gzip_(false)
{
}

core::curl_context::~curl_context()
{
    if (header_chunk_)
        curl_slist_free_all(header_chunk_);

    if (free_post_data_ && post_data_)
        free(post_data_);
}

bool core::curl_context::init(std::chrono::milliseconds _connect_timeout, std::chrono::milliseconds _timeout, core::proxy_settings _proxy_settings, const std::string &_user_agent)
{
    assert(!_user_agent.empty());

    connect_timeout_ = _connect_timeout;
    timeout_ = _timeout;
    proxy_settings_ = _proxy_settings;
    user_agent_ = _user_agent;

    return true;
}

bool core::curl_context::init_handler(CURL* _curl)
{
    if (!_curl)
        return false;

    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 1L);

    const bool is_http2_used = is_multi_task();

    curl_easy_setopt(_curl, CURLOPT_HTTP_VERSION, (is_http2_used ? CURL_HTTP_VERSION_2TLS : CURL_HTTP_VERSION_1_1));

    if (is_http2_used)
        curl_easy_setopt(_curl, CURLOPT_PIPEWAIT, long(1));

    curl_easy_setopt(_curl, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_callback);

    curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, 2L);

    curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, (void *)this);
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(_curl, CURLOPT_HEADERDATA, (void *)this);
    curl_easy_setopt(_curl, CURLOPT_HEADERFUNCTION, write_header_function);
    curl_easy_setopt(_curl, CURLOPT_PROGRESSDATA, (void *)this);
    curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, false);
    curl_easy_setopt(_curl, CURLOPT_PROGRESSFUNCTION, progress_callback);

    curl_easy_setopt(_curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(_curl, CURLOPT_TCP_KEEPIDLE, 5L);
    curl_easy_setopt(_curl, CURLOPT_TCP_KEEPINTVL, 5L);
    curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(_curl, CURLOPT_ACCEPT_ENCODING, "gzip");

    curl_easy_setopt(_curl, CURLOPT_USERAGENT, user_agent_.c_str());

    curl_easy_setopt(_curl, CURLOPT_DEBUGDATA, (void*)this);
    curl_easy_setopt(_curl, CURLOPT_DEBUGFUNCTION, trace_function);
    curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(_curl, CURLOPT_TIMEOUT_MS, 0);

    if (connect_timeout_.count() > 0)
        curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_.count());

    if (timeout_.count() > 0)
    {
        curl_easy_setopt(_curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(_curl, CURLOPT_LOW_SPEED_TIME, timeout_.count() / 1000);
    }

    if (proxy_settings_.use_proxy_)
    {
        curl_easy_setopt(_curl, CURLOPT_PROXY, tools::from_utf16(proxy_settings_.proxy_server_).c_str());

        if (proxy_settings_.proxy_port_ != proxy_settings::default_proxy_port)
            curl_easy_setopt(_curl, CURLOPT_PROXYPORT, (long)proxy_settings_.proxy_port_);

        curl_easy_setopt(_curl, CURLOPT_PROXYTYPE, (long)proxy_settings_.proxy_type_);

        if (proxy_settings_.need_auth_)
        {
            curl_easy_setopt(_curl, CURLOPT_PROXYUSERPWD, tools::from_utf16(proxy_settings_.login_ + L':' + proxy_settings_.password_).c_str());

            if (proxy_settings_.proxy_type_ == static_cast<int32_t>(core::proxy_type::http_proxy))
                curl_easy_setopt(_curl, CURLOPT_PROXYAUTH, curl_auth_type(proxy_settings_.auth_type_));
        }
    }

    curl_easy_setopt(_curl, CURLOPT_URL, original_url_.c_str());

    curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(_curl, CURLOPT_MAXREDIRS, 10L);

    if (!keep_alive_)
    {
        curl_easy_setopt(_curl, CURLOPT_COOKIEFILE, "");
    }

    if (curl_range_from_ != -1 && curl_range_to_ != -1)
    {
        std::stringstream ss_range;
        ss_range << curl_range_from_ << '-' << curl_range_to_;

        curl_easy_setopt(_curl, CURLOPT_RANGE, ss_range.str().c_str());
    }

    for (const auto& iter : post_form_parameters_)
    {
        if (!iter.second.empty())
            curl_formadd(&post, &last, CURLFORM_COPYNAME, iter.first.c_str(), CURLFORM_COPYCONTENTS, iter.second.c_str(), CURLFORM_END);
    }

    for (const auto& iter : post_form_files_)
    {
        curl_formadd(&post, &last, CURLFORM_COPYNAME, iter.first.c_str(), CURLFORM_FILE, iter.second.c_str(), CURLFORM_END);
    }

    for (auto& iter : post_form_filedatas_)
    {
        long size = iter.second.file_stream_.available();//it should be long
        const auto data = iter.second.file_stream_.read_available();
        iter.second.file_stream_.reset_out();
        curl_formadd(&post, &last,
            CURLFORM_COPYNAME, iter.first.c_str(),
            CURLFORM_BUFFER, tools::from_utf16(iter.second.file_name_).c_str(),
            CURLFORM_BUFFERPTR, data,
            CURLFORM_BUFFERLENGTH, size,
            CURLFORM_CONTENTTYPE, "application/octet-stream",
            CURLFORM_END);
    }

    if (curl_post_)
    {
        if (post_data_ && post_data_size_)
        {
            curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, (long)post_data_size_);
            curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, post_data_);
        }
#ifdef __linux__
        else
        {
            //avoiding bug in curl (empty post body causes hang on linux)
            curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, 0);
            curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, "");
        }
#endif //__linux__
        curl_easy_setopt(_curl, CURLOPT_POST, 1L);
    }

    if (curl_http_post_)
        curl_easy_setopt(_curl, CURLOPT_HTTPPOST, post);

    if (modified_time_ != -1)
    {
        curl_easy_setopt(_curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
        curl_easy_setopt(_curl, CURLOPT_TIMEVALUE, modified_time_);
    }

    if (header_chunk_)
        curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, header_chunk_);

    err_buf_[0] = 0;
    curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, err_buf_);

    return true;
}

void core::curl_context::set_replace_log_function(replace_log_function _func)
{
    replace_log_function_ = std::move(_func);
}

bool core::curl_context::is_need_log() const
{
    if (stop_func_ && stop_func_())
        return false;

    return need_log_;
}

void core::curl_context::set_need_log(bool _need)
{
    need_log_ = _need;
}

void core::curl_context::set_need_log_original_url(bool _value)
{
    need_log_original_url_ = _value;
}

bool core::curl_context::is_stopped() const
{
    if (stop_func_ && stop_func_())
        return true;

    return false;
}

bool core::curl_context::is_write_data_log()
{
    return write_data_log_;
}

void core::curl_context::set_write_data_log(bool _enable)
{
    write_data_log_ = _enable;
}

void core::curl_context::write_log_data(const char* _data, uint32_t _size)
{
    if (max_log_data_size_reached_)
        return;

    if (log_data_->all_size() + _size > MAX_LOG_DATA_SIZE)
    {
        log_data_->write<std::string_view>("\n*** max log data size has been reached; truncated; ***\n");
        max_log_data_size_reached_ = true;
        return;
    }

    log_data_->write(_data, _size);
}

void core::curl_context::write_log_string(std::string_view _log_string)
{
    log_data_->write<std::string_view>(_log_string);
}

void core::curl_context::set_range(int64_t _from, int64_t _to)
{
    assert(_from >= 0);
    assert(_to > 0);
    assert(_from < _to);

    curl_range_from_ = _from;
    curl_range_to_ = _to;
}

void core::curl_context::set_url(std::string_view sz_url)
{
    original_url_ = sz_url;
}

void core::curl_context::set_normalized_url(std::string&& _nurl)
{
    normalized_url_ = std::move(_nurl);
}

void core::curl_context::set_post()
{
    curl_post_ = true;
}

void core::curl_context::set_http_post()
{
    curl_http_post_ = true;
}

long core::curl_context::get_response_code() const
{
    return response_code_;
}

void core::curl_context::set_modified_time(time_t _last_modified_time)
{
    modified_time_ = _last_modified_time;
}

std::shared_ptr<core::tools::binary_stream> core::curl_context::get_header() const
{
    return header_;
}

CURLcode core::curl_context::execute_handler(CURL* _curl)
{
    start_time_ = std::chrono::steady_clock().now();

    auto res = curl_easy_perform(_curl);

    load_info(_curl, res);

    write_log(res);

    return res;
}

CURLMcode core::curl_context::execute_multi_handler(CURLM* _multi, CURL* _curl)
{
    start_time_ = std::chrono::steady_clock().now();
    return curl_multi_add_handle(_multi, _curl);
}

void core::curl_context::load_info(CURL* _curl, CURLcode _result)
{
    curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &response_code_);
    curl_easy_getinfo(_curl, CURLINFO_TOTAL_TIME, &request_time_);

    double dnld = 0, upld = 0;
    long headers_s = 0, request_s = 0;

    auto d_s = curl_easy_getinfo(_curl, CURLINFO_SIZE_DOWNLOAD /*_T*/, &dnld);
    auto u_s = curl_easy_getinfo(_curl, CURLINFO_SIZE_UPLOAD /*_T*/, &upld);
    auto h_s = curl_easy_getinfo(_curl, CURLINFO_HEADER_SIZE, &headers_s);
    auto req_s = curl_easy_getinfo(_curl, CURLINFO_REQUEST_SIZE, &request_s);

    long downloaded_on_iter = 0;
    long uploaded_on_iter = 0;

    if (d_s == CURLE_OK)
        downloaded_on_iter += static_cast<long>(dnld);

    if (u_s == CURLE_OK)
        uploaded_on_iter += static_cast<long>(upld);

    if (h_s == CURLE_OK)
        downloaded_on_iter += headers_s;

    // Disable because it may be counted for more than one request
    UNUSED_ARG(req_s);
    //    if (req_s == CURLE_OK)
    //        uploaded_on_iter += request_s;

    static stats_aggregator aggregator(FLUSH_STATS_TO_CORE_TIMEOUT);
    aggregator.add_downloaded(downloaded_on_iter);
    aggregator.add_uploaded(uploaded_on_iter);

    static bool timer_started = false;

    if (!timer_started)
    {
        g_core->add_timer([]
        {
            if (!aggregator.need_release())
                return;

            auto stat_d = aggregator.release();

            g_core->get_statistics()->increment_event_prop(stats::stats_event_names::send_used_traffic_size_event,
                std::string("download"),
                stat_d.total_downloaded_);

            g_core->get_statistics()->increment_event_prop(stats::stats_event_names::send_used_traffic_size_event,
                std::string("upload"),
                stat_d.total_uploaded_);
        }, SAVE_AGGREGATED_STATS_INTERVAL);

        timer_started = true;
    }

    if (g_core->is_im_stats_enabled() && is_send_im_stats())
    {
        const auto is_one_domain = omicronlib::_o("one_domain_feature", feature::default_one_domain_feature());

        core::stats::event_props_type props;
        props.emplace_back("endpoint", normalized_url_);
        auto event_name = is_one_domain ? stats::im_stat_event_names::u_network_communication_event : stats::im_stat_event_names::network_communication_event;
        g_core->insert_im_stats_event(event_name, std::move(props));

        if (_result != CURLE_OK)
        {
            core::stats::event_props_type props;
            props.emplace_back("endpoint", normalized_url_);
            event_name = is_one_domain ? stats::im_stat_event_names::u_network_error_event : stats::im_stat_event_names::network_error_event;
            g_core->insert_im_stats_event(event_name, std::move(props));
        }
        else if (response_code_ < 200 || response_code_ >= 300)
        {
            // convert http response code into string: 404 -> 4xx
            std::string response_code_str = std::to_string(response_code_);
            auto len = response_code_str.length();
            response_code_str.replace(1, len - 1, len - 1, 'x');

            core::stats::event_props_type props;
            props.emplace_back("not_2xx", normalized_url_);
            props.emplace_back(response_code_str, normalized_url_);
            props.emplace_back("code", std::to_string(response_code_));
            event_name = is_one_domain ? stats::im_stat_event_names::u_network_http_code_event : stats::im_stat_event_names::network_http_code_event;
            g_core->insert_im_stats_event(event_name, std::move(props));
        }
    }
}

void core::curl_context::write_log(CURLcode res)
{
    if (is_need_log())
        write_log_message(std::to_string(res), start_time_);
}

bool core::curl_context::execute_request()
{
    auto handler = curl_handler::instance().perform(shared_from_this());
    assert(handler.valid());

    handler.wait();

    return (handler.get() == CURLE_OK);
}

void core::curl_context::execute_request_async(http_request_simple::completion_function _completion_function)
{
    curl_handler::instance().perform_async(shared_from_this(),
        [_completion_function](curl_easy::completion_code _code)
        {
            _completion_function(_code);
        });
}

void core::curl_context::set_multi(bool _multi)
{
    multi_ = _multi;
}

bool core::curl_context::is_multi_task() const
{
    return multi_ || core::urls::is_one_domain_url(original_url_);
}

bool core::curl_context::is_gzip() const
{
    return gzip_;
}

double core::curl_context::get_request_time() const
{
    return request_time_;
}

void core::curl_context::write_log_message(std::string_view _result, std::chrono::steady_clock::time_point _start_time)
{
    const auto finish = std::chrono::steady_clock().now();

    std::stringstream message;
    if (need_log_original_url_)
        message << "url: " << original_url_ << '\n';
    message << "curl_easy_perform result is ";
    message << _result << '\n';
    if (strlen(err_buf_) != 0)
        message << err_buf_ << '\n';

    message << "completed in " << std::chrono::duration_cast<std::chrono::milliseconds>(finish -_start_time).count() << " ms";
    message << std::endl;
    write_log_string(message.str());

    if (replace_log_function_)
        replace_log_function_(*log_data_);

    g_core->write_data_to_network_log(*log_data_);
}

void core::curl_context::set_custom_header_params(const std::vector<std::string>& _params)
{
    for (const auto& parameter : _params)
        header_chunk_ = curl_slist_append(header_chunk_, parameter.c_str());
}

void core::curl_context::set_priority(priority_t _priority)
{
    priority_ = _priority;
}

core::priority_t core::curl_context::get_priority() const
{
    return priority_;
}

void core::curl_context::set_id(int64_t _id)
{
    id_ = _id;
}

int64_t core::curl_context::get_id() const
{
    return id_;
}

void core::curl_context::set_send_im_stats(bool _value)
{
    is_send_im_stats_ = _value;
}

bool core::curl_context::is_send_im_stats() const
{
    return is_send_im_stats_;
}

void core::curl_context::set_post_form_parameters(const std::map<std::string, std::string>& _post_form_parameters)
{
    post_form_parameters_ = _post_form_parameters;
}

void core::curl_context::set_post_form_files(const std::multimap<std::string, std::string>& _post_form_files)
{
    post_form_files_ = _post_form_files;
}

void core::curl_context::set_post_data(const char* _data, int32_t _size, bool _copy)
{
    if (gzip_)
    {
        tools::binary_stream compressed_data;
        if (tools::compress(_data, _size, compressed_data))
        {
            std::stringstream s;
            s << "\n" << normalized_url_ << " gzip; oldsize = " << _size << " newsize = " << compressed_data.all_size() << "\n";
            write_log_string(s.str());

            post_data_size_ = compressed_data.all_size();
            post_data_ = (char*)malloc(post_data_size_);
            memcpy(post_data_, compressed_data.get_data(), post_data_size_);
            free_post_data_ = true;
        }
        return;
    }
    if (_copy)
    {
        post_data_size_ = _size;
        post_data_ = (char*)malloc(post_data_size_);
        memcpy(post_data_, _data, post_data_size_);
        free_post_data_ = true;
    }
    else
    {
        post_data_ = (char*)_data;
        post_data_size_ = _size;
    }
}

void core::curl_context::set_post_parameters(const std::string& _post_parameters)
{
    if (_post_parameters.empty())
        return;

    set_post_data(_post_parameters.c_str(), (int32_t)_post_parameters.length(), true);
}

void core::curl_context::set_post_form_filedatas(const std::multimap<std::string, file_binary_stream>& _post_form_filedatas)
{
    post_form_filedatas_ = _post_form_filedatas;
}

void core::curl_context::set_gzip(bool _gzip)
{
    gzip_ = (_gzip && omicronlib::_o("one_domain_feature", feature::default_one_domain_feature()));
}

static size_t write_header_function(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    auto ctx = (core::curl_context *) userp;
    ctx->header_->reserve((uint32_t)realsize);
    ctx->header_->write((char *)contents, (uint32_t)realsize);

    return realsize;
}

static size_t write_memory_callback(void* _contents, size_t _size, size_t _nmemb, void* _userp)
{
    size_t realsize = _size * _nmemb;
    auto ctx = (core::curl_context*) _userp;
    ctx->output_->write((char*) _contents, (uint32_t) realsize);

    if (ctx->is_need_log())
    {
        ctx->write_log_data((const char*) _contents, (uint32_t) realsize);
    }

    return realsize;
}

static int32_t progress_callback(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double /*NowUploaded*/)
{
    auto cntx = (core::curl_context*) ptr;

    assert(cntx->bytes_transferred_pct_ >= 0);
    assert(cntx->bytes_transferred_pct_ <= 100);

    if (core::curl_handler::instance().is_stopped() || (cntx->stop_func_ && cntx->stop_func_()))
    {
        return -1;
    }

    const auto file_too_small = (TotalToDownload <= 1);
    const auto skip_progress = (file_too_small || !cntx->progress_func_);
    if (skip_progress)
    {
        return 0;
    }

    const auto bytes_transferred_pct = (int32_t)((NowDownloaded * 100) / TotalToDownload);
    assert(bytes_transferred_pct >= 0);
    assert(bytes_transferred_pct <= 100);

    const auto percentage_updated = (bytes_transferred_pct != cntx->bytes_transferred_pct_);
    if (!percentage_updated)
    {
        return 0;
    }

    cntx->bytes_transferred_pct_ = bytes_transferred_pct;

    cntx->progress_func_((int64_t)TotalToDownload, (int64_t)NowDownloaded, bytes_transferred_pct);

    return 0;
}

static std::string_view curl_infotype_to_string_view(curl_infotype type)
{
    switch (type)
    {
    case CURLINFO_TEXT:
        return "Text";

    case CURLINFO_HEADER_IN:
        return "HeaderIn";

    case CURLINFO_HEADER_OUT:
        return "HeaderOut";

    case CURLINFO_DATA_IN:
        return "DataIn";

    case CURLINFO_DATA_OUT:
        return "DataOut";

    case CURLINFO_SSL_DATA_IN:
        return "SSLDataIn";

    case CURLINFO_SSL_DATA_OUT:
        return "SSLDataOut";

    default:
        return "Unknown";
    }
}

static int32_t trace_function(CURL* /*_handle*/, curl_infotype _type, unsigned char* _data, size_t _size, void* _userp)
{
    auto ctx = (core::curl_context*) _userp;
    if (!ctx->is_need_log())
        return 0;

    if (ctx->is_gzip() && _type == CURLINFO_DATA_OUT)
    {
        core::tools::binary_stream compressed;
        compressed.write((const char*)_data, _size);

        core::tools::binary_stream data;
        core::tools::decompress(compressed, data);

        ctx->write_log_data(data.get_data(), data.all_size());
        if (ctx->replace_log_function_)
            ctx->replace_log_function_(*ctx->log_data_);

        ctx->write_log_string("\n");
        return 0;
    }

    constexpr std::string_view filter1 = "TLSv";
    constexpr std::string_view filter2 = "STATE:";

    if (!core::configuration::get_app_config().is_curl_log_enabled())
    {
        switch (_type)
        {
        case CURLINFO_TEXT:
            if (filter1.size() < _size && memcmp(_data, filter1.data(), filter1.size()) == 0)
                return 0;

            if (filter2.size() < _size && memcmp(_data, filter2.data(), filter2.size()) == 0)
                return 0;

            break;
        case CURLINFO_HEADER_OUT:
        case CURLINFO_HEADER_IN:
        case CURLINFO_END:
            break;
        case CURLINFO_DATA_OUT:
            if (ctx->is_write_data_log())
                break;
            [[fallthrough]];
        case CURLINFO_DATA_IN:
        case CURLINFO_SSL_DATA_IN:
        case CURLINFO_SSL_DATA_OUT:
            return 0;
        }
    }

    ctx->write_log_data((const char*) _data, (uint32_t) _size);
    if (ctx->replace_log_function_)
        ctx->replace_log_function_(*ctx->log_data_);
    return 0;
}
