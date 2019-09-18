#pragma once

#include "../external/curl/include/curl.h"

#include "http_request.h"

#include <boost/variant.hpp>

namespace core
{
    struct curl_context : std::enable_shared_from_this<curl_context>
    {
    public:
        curl_context(std::shared_ptr<tools::stream> _output, http_request_simple::stop_function _stop_func, http_request_simple::progress_function _progress_func, bool _keep_alive);
        ~curl_context();

        bool init(std::chrono::milliseconds _connect_timeout, std::chrono::milliseconds _timeout, core::proxy_settings _proxy_settings, const std::string &_user_agent);
        bool init_handler(CURL* _curl);

        bool is_need_log() const;
        void set_need_log(bool _need);
        void set_need_log_original_url(bool _value);

        bool is_stopped() const;

        bool is_write_data_log();
        void set_write_data_log(bool _enable);

        void write_log_data(const char* _data, uint32_t _size);
        void write_log_string(std::string_view _log_string);

        void set_replace_log_function(replace_log_function _func);
        void set_range(int64_t _from, int64_t _to);
        void set_url(std::string_view sz_url);
        void set_normalized_url(std::string&& _nurl);
        void set_post();
        void set_http_post();
        void set_modified_time(time_t _last_modified_time);
        void set_custom_header_params(const std::vector<std::string>& _params);

        void set_priority(priority_t _priority);
        priority_t get_priority() const;

        void set_id(int64_t _id);
        int64_t get_id() const;

        void set_post_data(const char* _data, int32_t _size, bool _copy);
        void set_post_parameters(const std::string& _post_parameters);
        void set_post_form_parameters(const std::map<std::string, std::string>& _post_form_parameters);
        void set_post_form_files(const std::multimap<std::string, std::string>& _post_form_files);
        void set_post_form_filedatas(const std::multimap<std::string, file_binary_stream>& _post_form_filedatas);
        void set_gzip(bool _gzip);

        long get_response_code() const;
        double get_request_time() const;

        bool is_send_im_stats() const;
        void set_send_im_stats(bool _val);

        std::shared_ptr<tools::binary_stream> get_header() const;

        CURLcode execute_handler(CURL* _curl);
        CURLMcode execute_multi_handler(CURLM* _multi, CURL* _curl);

        void load_info(CURL* _curl, CURLcode _resul);
        void write_log(CURLcode res);

        bool execute_request();
        void execute_request_async(http_request_simple::completion_function _completion_function);

        void set_multi(bool _multi);
        bool is_multi_task() const;

        bool is_gzip() const;

    private:
        void write_log_message(std::string_view _result, std::chrono::steady_clock::time_point _start_time);

    public:
        http_request_simple::stop_function stop_func_;
        http_request_simple::progress_function progress_func_;
        core::replace_log_function replace_log_function_;

        std::shared_ptr<tools::stream> output_;
        std::shared_ptr<tools::binary_stream> header_;
        std::shared_ptr<tools::binary_stream> log_data_;

        int32_t bytes_transferred_pct_;

    private:
        std::string original_url_;
        std::string normalized_url_;
        std::string user_agent_;
        std::chrono::milliseconds connect_timeout_;
        std::chrono::milliseconds timeout_;
        core::proxy_settings proxy_settings_;
        priority_t priority_;

        bool need_log_;
        bool write_data_log_;
        bool need_log_original_url_;
        bool keep_alive_;
        bool curl_post_;
        bool curl_http_post_;

        curl_slist* header_chunk_;
        struct curl_httppost* post;
        struct curl_httppost* last;

        long response_code_;
        double request_time_;

        int64_t curl_range_from_;
        int64_t curl_range_to_;

        std::map<std::string, std::string> post_form_parameters_;
        std::multimap<std::string, std::string> post_form_files_;
        std::multimap<std::string, file_binary_stream> post_form_filedatas_;

        char* post_data_;
        int32_t post_data_size_;
        bool free_post_data_;

        time_t modified_time_;
        int64_t id_;

        bool max_log_data_size_reached_;

        char err_buf_[CURL_ERROR_SIZE];

        bool is_send_im_stats_;
        bool multi_;
        bool gzip_;

        std::chrono::steady_clock::time_point start_time_;
    };
}
