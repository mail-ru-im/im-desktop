#ifndef __HTTP_REQUEST_H_
#define __HTTP_REQUEST_H_

#pragma once

#include "proxy_settings.h"

#include "../external/curl/include/curl.h"

#include "curl_easy_handler.h"

namespace core
{
    struct curl_context;

    class hosts_map;

    class ithread_callback;

    typedef std::function<void(tools::binary_stream&)> replace_log_function;

    namespace tools
    {
        class binary_stream;
    }

    struct file_binary_stream
    {
        std::wstring file_name_;
        tools::binary_stream file_stream_;
    };

    class http_request_simple
    {
    public:
        using completion_function = curl_easy::completion_function;

        using stop_function = std::function<bool()>;

        using progress_function = std::function<void(int64_t _bytes_total, int64_t _bytes_transferred, int32_t _pct_transferred)>;

    private:
        stop_function stop_func_;
        progress_function progress_func_;
        replace_log_function replace_log_function_;
        std::map<std::string, std::string> post_parameters_;
        std::map<std::string, std::string> post_form_parameters_;
        std::multimap<std::string, std::string> post_form_files_;
        std::multimap<std::string, file_binary_stream> post_form_filedatas_;

        std::string url_;
        std::string url_meta_info_;
        std::string normalized_url_;
        std::vector<std::string> custom_headers_;

        long response_code_;
        std::shared_ptr<tools::stream> output_;
        std::shared_ptr<tools::binary_stream> header_;

        bool is_time_condition_;
        time_t last_modified_time_;

        std::chrono::milliseconds connect_timeout_;
        std::chrono::milliseconds timeout_;

        int64_t range_from_;
        int64_t range_to_;
        bool is_post_form_;
        bool need_log_;
        bool write_data_log_;
        bool need_log_original_url_;
        bool keep_alive_;
        priority_t priority_;

        std::string get_post_param() const;
        bool send_request(bool _post, double& _request_time);
        void send_request_async(bool _post, completion_function _completion_function);
        proxy_settings proxy_settings_;

        std::string user_agent_;
        char* post_data_;
        int32_t post_data_size_;
        bool copy_post_data_;

        int64_t id_;
        bool is_send_im_stats_;
        bool multi_;
        bool gzip_;

    public:
        http_request_simple(proxy_settings _proxy_settings, std::string _user_agent, stop_function _stop_func = nullptr, progress_function _progress_func = nullptr);
        virtual ~http_request_simple();

        static void init_global();
        static void shutdown_global();
        static std::string normalized_url(std::string_view _url, std::string_view _meta_info = {});

        void set_output_stream(std::shared_ptr<tools::stream> _output);

        std::shared_ptr<tools::stream> get_response() const;
        std::shared_ptr<tools::binary_stream> get_header() const;
        std::string get_header_attribute(std::string_view _name) const;
        long get_response_code() const;

        void set_modified_time_condition(time_t _modified_time);

        void set_url(std::wstring_view url);
        void set_url(std::string_view url);
        void set_url_meta_info(std::string_view url);
        void set_normalized_url(std::string_view normalized_url);
        std::string get_post_url() const;
        std::string get_normalized_url() const;

        void push_post_parameter(std::wstring_view  name, std::wstring_view value);
        void push_post_parameter(std::string name, std::string value);
        void push_post_form_parameter(std::wstring_view name, std::wstring_view value);
        void push_post_form_parameter(std::string name, std::string value);
        void push_post_form_file(std::wstring_view name, std::wstring_view file_name);
        void push_post_form_file(const std::string& name, const std::string& file_name);
        void push_post_form_filedata(std::wstring_view name, std::wstring_view file_name);
        void push_post_form_filedata(std::string_view _param_name, tools::binary_stream _file_data);

        void set_need_log(bool _need);
        void set_write_data_log(bool _enable);
        void set_need_log_original_url(bool _value);
        void set_keep_alive();
        void set_priority(priority_t _priority);
        void set_multi(bool _multi);
        void set_gzip(bool _gzip);
        void set_etag(std::string_view etag);
        void set_replace_log_function(replace_log_function _func);
        void set_post_data(char* _data, int32_t _size, bool _copy);
        void set_id(int64_t _id);
        void set_send_im_stats(bool _value);

        const std::map<std::string, std::string>& get_post_parameters() const;

        void set_range(int64_t _from, int64_t _to);

        void post_async(completion_function _completion_function);
        void get_async(completion_function _completion_function);

        bool post();
        bool get();
        bool get(double& _connect_time);
        void set_post_form(bool _is_post_form);

        void set_custom_header_param(std::string _value);

        void set_connect_timeout(std::chrono::milliseconds _timeout);
        void set_timeout(std::chrono::milliseconds _timeout);

        static std::vector<std::mutex*> ssl_sync_objects;
        proxy_settings get_user_proxy() const;
    };
}


#endif// __HTTP_REQUEST_H_