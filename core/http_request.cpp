#include "stdafx.h"
#include "http_request.h"
#include "curl_context.h"
#include "openssl/crypto.h"
#include "tools/system.h"
#include "tools/url.h"
#include "tools/features.h"
#include "log/log.h"
#include "utils.h"
#include "async_task.h"
#include "core.h"
#include "proxy_settings.h"
#include "network_log.h"
#include "../corelib/enumerations.h"
#include "../libomicron/include/omicron/omicron.h"
#include "../common.shared/string_utils.h"
#include "../common.shared/omicron_keys.h"
#include "../common.shared/string_utils.h"

using namespace core;

namespace
{
    constexpr auto default_http_connect_timeout = std::chrono::seconds(5);
    constexpr auto default_http_execute_timeout = std::chrono::seconds(8);
    constexpr auto default_max_get_url_length = 3500u;
    constexpr auto uri_too_long_code = 414;
}

http_request_simple::http_request_simple(proxy_settings _proxy_settings, std::string _user_agent, priority_t _priority, stop_function _stop_func, progress_function _progress_func)
    : stop_func_(std::move(_stop_func)),
    progress_func_(std::move(_progress_func)),
    response_code_(0),
    output_(std::make_shared<tools::binary_stream>()),
    header_(std::make_shared<tools::binary_stream>()),
    is_time_condition_(false),
    last_modified_time_(0),
    connect_timeout_(default_http_connect_timeout),
    timeout_(default_http_execute_timeout),
    range_from_(-1),
    range_to_(-1),
    is_post_form_(false),
    need_log_(true),
    write_data_log_(true),
    need_log_original_url_(false),
    keep_alive_(false),
    priority_(_priority),
    proxy_settings_(std::move(_proxy_settings)),
    user_agent_(std::move(_user_agent)),
    post_data_(nullptr),
    post_data_size_(0),
    copy_post_data_(false),
    id_(-1),
    is_send_im_stats_(true),
    multi_(false),
    use_curl_decompression_(false),
    compression_method_(data_compression_method::none)
{
    assert(!user_agent_.empty());
}

http_request_simple::~http_request_simple() = default;

void http_request_simple::set_output_stream(std::shared_ptr<tools::stream> _output)
{
    output_ = std::move(_output);
}

void http_request_simple::set_need_log(bool _need)
{
    need_log_ = _need;
}

void http_request_simple::set_write_data_log(bool _enable)
{
    write_data_log_ = _enable;
}

void http_request_simple::set_need_log_original_url(bool _value)
{
    need_log_original_url_ = _value;
}

void http_request_simple::push_post_parameter(std::wstring_view name, std::wstring_view value)
{
    assert(!name.empty());

    post_parameters_[tools::from_utf16(name)] = tools::from_utf16(value);
}

void http_request_simple::push_post_parameter(std::string name, std::string value)
{
    post_parameters_[std::move(name)] = std::move(value);
}

void http_request_simple::push_post_form_parameter(std::wstring_view name, std::wstring_view value)
{
    post_form_parameters_[tools::from_utf16(name)] = tools::from_utf16(value);
}

void http_request_simple::push_post_form_parameter(std::string name, std::string value)
{
    assert(!name.empty());
    post_form_parameters_[std::move(name)] = std::move(value);
}

void http_request_simple::push_post_form_file(std::wstring_view name, std::wstring_view file_name)
{
    push_post_form_file(tools::from_utf16(name), tools::from_utf16(file_name));
}

void http_request_simple::push_post_form_file(const std::string& name, const std::string& file_name)
{
    assert(!name.empty());
    assert(!file_name.empty());
    post_form_files_.insert(std::make_pair(name, file_name));
}

void http_request_simple::push_post_form_filedata(std::wstring_view name, std::wstring_view file_name)
{
    assert(!name.empty());
    assert(!file_name.empty());
    file_binary_stream file_data;
    file_data.file_name_ = file_name.substr(file_name.find_last_of(L"\\/") + 1);
    file_data.file_stream_.load_from_file(file_name);
    if (file_data.file_stream_.available())
        post_form_filedatas_.insert(std::make_pair(tools::from_utf16(name), std::move(file_data)));
}

void core::http_request_simple::push_post_form_filedata(std::string_view _param, tools::binary_stream _data)
{
    if (!_param.empty() && _data.available())
    {
        file_binary_stream file_data;
        file_data.file_name_ = tools::from_utf8(_param);
        file_data.file_stream_ = std::move(_data);
        post_form_filedatas_.insert(std::make_pair(std::string(_param), std::move(file_data)));
    }
}

void http_request_simple::set_url(std::wstring_view url)
{
    set_url(tools::from_utf16(url));
}

void http_request_simple::set_url(std::string_view url)
{
    url_ = tools::encode_url(url);
}

void http_request_simple::set_url_meta_info(std::string_view info)
{
    url_meta_info_ = tools::encode_url(info);
}

void http_request_simple::set_normalized_url(std::string_view _normalized_url)
{
    normalized_url_ = _normalized_url;
}

void http_request_simple::set_modified_time_condition(time_t _modified_time)
{
    is_time_condition_ = true;
    last_modified_time_ = _modified_time;
}

void http_request_simple::set_connect_timeout(std::chrono::milliseconds _timeout)
{
    connect_timeout_ = _timeout;
}

void http_request_simple::set_timeout(std::chrono::milliseconds _timeout)
{
    timeout_ = _timeout;
}

void core::http_request_simple::set_use_curl_decompresion(bool _enable)
{
    use_curl_decompression_ = _enable;
}

std::string http_request_simple::get_post_param() const
{
    std::string result;
    for (const auto&[key, value] : post_parameters_)
    {
        result += key;
        if (post_parameters_.size() > 1 || !value.empty())
        {
            result += '=';
            result += value;
        }
        result += '&';
    }
    if (!result.empty())
        result.pop_back();
    return result;
}

bool http_request_simple::send_request(bool _post, double& _request_time)
{
    if (!_post)
    {
        if (const auto max_size = omicronlib::_o(omicron::keys::max_get_url_length, default_max_get_url_length); url_.size() > max_size)
        {
            g_core->write_string_to_network_log(su::concat("send_request: too long url ", std::to_string(url_.size()), " > ", std::to_string(max_size), " [GET ", normalized_url_, "]\r\n"));
            response_code_ = uri_too_long_code;
            return true;
        }
    }

    auto ctx = std::make_shared<curl_context>(output_, stop_func_, progress_func_, keep_alive_);
    const auto& proxy_settings = g_core->get_proxy_settings();

    _request_time = 0;

    if (!ctx->init(connect_timeout_, timeout_, proxy_settings, user_agent_))
    {
        g_core->write_string_to_network_log("send_request: ctx init fail\r\n");
        assert(false);
        return false;
    }

    ctx->set_post_data_compression(compression_method_);
    ctx->set_use_curl_decompression(use_curl_decompression_);
    ctx->set_normalized_url(get_normalized_url());
    if (_post)
    {
        if (post_data_ && post_data_size_)
            ctx->set_post_data(post_data_, post_data_size_, copy_post_data_);
        else
            ctx->set_post_parameters(get_post_param());

        ctx->set_post_form_parameters(post_form_parameters_);
        ctx->set_post_form_files(post_form_files_);
        ctx->set_post_form_filedatas(post_form_filedatas_);

        if (is_post_form_)
            ctx->set_http_post();
        else
            ctx->set_post();
    }

    if (is_time_condition_)
        ctx->set_modified_time(last_modified_time_);

    if (range_from_ >= 0 && range_to_ > 0)
        ctx->set_range(range_from_, range_to_);

    ctx->set_need_log(need_log_);
    ctx->set_write_data_log(write_data_log_);
    ctx->set_need_log_original_url(need_log_original_url_);

    ctx->set_custom_header_params(custom_headers_);
    ctx->set_url(url_);

    ctx->set_replace_log_function(replace_log_function_);

    ctx->set_priority(priority_);

    ctx->set_multi(multi_);

    ctx->set_id(id_);

    ctx->set_send_im_stats(is_send_im_stats_);

    if (!ctx->execute_request())
        return false;

    response_code_ = ctx->get_response_code();
    header_ = ctx->get_header();
    _request_time = ctx->get_request_time();

    return true;
}

void http_request_simple::send_request_async(bool _post, completion_function _completion_function)
{
    if (!_post)
    {
        if (const auto max_size = omicronlib::_o(omicron::keys::max_get_url_length, default_max_get_url_length); url_.size() > max_size)
        {
            g_core->write_string_to_network_log(su::concat("send_request_async: too long url ", std::to_string(url_.size()), " > ", std::to_string(max_size), " [GET ", normalized_url_, "]\r\n"));
            response_code_ = uri_too_long_code;
            if (_completion_function)
                _completion_function(curl_easy::completion_code::success);
            return;
        }
    }

    const auto& proxy_settings = g_core->get_proxy_settings();

    auto ctx = std::make_shared<curl_context>(output_, stop_func_, progress_func_, keep_alive_);
    if (!ctx->init(connect_timeout_, timeout_, proxy_settings, user_agent_))
    {
        g_core->write_string_to_network_log("send_request_async: ctx init fail\r\n");
        if (_completion_function)
            _completion_function(curl_easy::completion_code::failed);
        return;
    }

    ctx->set_post_data_compression(compression_method_);
    ctx->set_use_curl_decompression(use_curl_decompression_);

    if (_post)
    {
        if (post_data_ && post_data_size_)
            ctx->set_post_data(post_data_, post_data_size_, copy_post_data_);
        else
            ctx->set_post_parameters(get_post_param());

        ctx->set_post_form_parameters(post_form_parameters_);
        ctx->set_post_form_files(post_form_files_);
        ctx->set_post_form_filedatas(post_form_filedatas_);

        if (is_post_form_)
            ctx->set_http_post();
        else
            ctx->set_post();
    }

    if (is_time_condition_)
        ctx->set_modified_time(last_modified_time_);

    if (range_from_ >= 0 && range_to_ > 0)
        ctx->set_range(range_from_, range_to_);

    ctx->set_need_log(need_log_);
    ctx->set_write_data_log(write_data_log_);

    ctx->set_custom_header_params(custom_headers_);
    ctx->set_url(url_);
    ctx->set_normalized_url(get_normalized_url());

    ctx->set_replace_log_function(replace_log_function_);

    ctx->set_priority(priority_);

    ctx->set_id(id_);

    ctx->set_send_im_stats(is_send_im_stats_);

    ctx->execute_request_async([this, ctx, _completion_function = std::move(_completion_function)](curl_easy::completion_code _completion_code)
    {
        response_code_ = ctx->get_response_code();

        if (_completion_code == curl_easy::completion_code::success)
        {
            header_ = ctx->get_header();
        }

        if (_completion_function)
            _completion_function(_completion_code);
    });
}

void http_request_simple::post_async(completion_function _completion_function)
{
    send_request_async(true, std::move(_completion_function));
}

void http_request_simple::get_async(completion_function _completion_function)
{
    return send_request_async(false, std::move(_completion_function));
}

bool http_request_simple::post()
{
    double request_time = 0;
    return send_request(true, request_time);
}

bool http_request_simple::get()
{
    double request_time = 0;
    return send_request(false, request_time);
}

bool http_request_simple::get(double& _connect_time)
{
    return send_request(false, _connect_time);
}

void http_request_simple::set_range(int64_t _from, int64_t _to)
{
    range_from_ = _from;
    range_to_ = _to;
}

std::shared_ptr<tools::stream> http_request_simple::get_response() const
{
    return output_;
}

std::shared_ptr<tools::binary_stream> http_request_simple::get_header() const
{
    return header_;
}

long http_request_simple::get_response_code() const
{
    return response_code_;
}

const std::map<std::string, std::string>& http_request_simple::get_post_parameters() const
{
    return post_parameters_;
}

void http_request_simple::set_custom_header_param(std::string _value)
{
    custom_headers_.push_back(std::move(_value));
}

std::vector<std::unique_ptr<std::mutex>> http_request_simple::ssl_sync_objects;

static unsigned long id_function(void)
{
    return tools::system::get_current_thread_id();
}


static void locking_function( int32_t mode, int32_t n, const char *file, int32_t line )
{
    if ( mode & CRYPTO_LOCK )
        http_request_simple::ssl_sync_objects[n]->lock();
    else
        http_request_simple::ssl_sync_objects[n]->unlock();
}


void core::http_request_simple::init_global()
{
    auto lock_count = CRYPTO_num_locks();
    http_request_simple::ssl_sync_objects.reserve(lock_count);
    for (decltype(lock_count) i = 0; i < lock_count; ++i)
        http_request_simple::ssl_sync_objects.emplace_back(std::make_unique<std::mutex>());

    CRYPTO_set_id_callback(id_function);
    CRYPTO_set_locking_callback(locking_function);
}

void core::http_request_simple::shutdown_global()
{
    if (http_request_simple::ssl_sync_objects.empty())
        return;

    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);

    http_request_simple::ssl_sync_objects.clear();
}

std::string core::http_request_simple::normalized_url(std::string_view _url, std::string_view _meta_info)
{
    std::string endpoint;

    if (!_url.empty())
    {
        auto make_end_point = [_meta_info](std::string_view _endpoint)
        {
            if (!_endpoint.empty() && !_meta_info.empty())
                return su::concat('_', _meta_info, '_', _endpoint);
            return std::string(_endpoint);
        };

        endpoint = make_end_point(_url.substr(0, _url.find('?')));

        for (auto& s : endpoint)
            if (!(isalnum(static_cast<int>(s)) || s == '-'))
                s = '_';
    }

    return endpoint;
}

void core::http_request_simple::set_post_form(bool _is_post_form)
{
    is_post_form_ = _is_post_form;
}

void core::http_request_simple::set_keep_alive()
{
    if (keep_alive_)
        return;

    keep_alive_ = true;

    custom_headers_.emplace_back("Connection: keep-alive");
}

void core::http_request_simple::set_multi(bool _multi)
{
    multi_ = _multi;
}

void core::http_request_simple::set_compression_method(data_compression_method _method)
{
    compression_method_ = _method;
}

void core::http_request_simple::set_compression_auto()
{
    if (features::is_zstd_request_enabled())
        set_compression_method(data_compression_method::zstd);
    else
        set_compression_method(data_compression_method::gzip);
}

void core::http_request_simple::set_etag(std::string_view _etag)
{
    if (!_etag.empty())
        custom_headers_.push_back(su::concat("If-None-Match: \"", _etag, '"'));
}

void core::http_request_simple::set_replace_log_function(replace_log_function _func)
{
    replace_log_function_ = std::move(_func);
}

void core::http_request_simple::set_post_data(char* _data, int32_t _size, bool _copy)
{
    post_data_ = _data;
    post_data_size_ = _size;
    copy_post_data_ = _copy;
}

void core::http_request_simple::set_id(int64_t _id)
{
    id_ = _id;
}

void core::http_request_simple::set_send_im_stats(bool _value)
{
    is_send_im_stats_ = _value;
}

bool core::http_request_simple::is_compressed() const
{
    return compression_method_ != data_compression_method::none;
}

std::string core::http_request_simple::get_post_url() const
{
    return su::concat(url_, '?', get_post_param());
}

std::string core::http_request_simple::get_normalized_url() const
{
    if (!normalized_url_.empty())
        return normalized_url_;

    return normalized_url(url_, url_meta_info_);
}

proxy_settings core::http_request_simple::get_user_proxy() const
{
    return proxy_settings_;
}

std::string core::http_header::get_attribute(const std::shared_ptr<tools::binary_stream>& _header, std::string_view _name)
{
    std::string value;
    if (_header && !_name.empty())
    {
        if (const auto size = _header->available(); size > 0)
        {
            std::string_view data(_header->read_available(), size);

            // use case-insensitive search
            auto it = std::search(data.cbegin(), data.cend(), _name.cbegin(), _name.cend(), [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); });
            if (it != data.cend())
            {
                auto start_pos = std::distance(data.cbegin(), it) + _name.size();
                if ((start_pos + 2) < data.size() && data[start_pos] == ':' && data[start_pos + 1] == ' ')
                {
                    start_pos += 2;
                    if (auto end_pos = data.find("\r\n", start_pos); end_pos != std::string_view::npos)
                        value = data.substr(start_pos, end_pos - start_pos);
                    else
                        value = data.substr(start_pos);
                }
            }
            _header->reset_out();
        }
    }
    return value;
}
