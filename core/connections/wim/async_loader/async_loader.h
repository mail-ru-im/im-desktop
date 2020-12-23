#pragma once

#include "../loader/loader_helpers.h"
#include "../loader/preview_proxy.h"
#include "../loader/web_file_info.h"

#include "async_handler.h"
#include "downloadable_file_chunks.h"
#include "downloaded_file_info.h"
#include "file_sharing_meta.h"
#include "../wim_packet.h"

#include "../../../log/log.h"

#include "../../../corelib/collection_helper.h"
#include "../../../corelib/enumerations.h"

namespace core
{
    class coll_helper;


    namespace wim
    {
        static const std::string default_file_location;

        typedef transferred_data<void> default_data_t;
        typedef transferred_data<downloaded_file_info> file_info_data_t;
        typedef transferred_data<preview_proxy::link_meta> link_meta_data_t;
        typedef transferred_data<file_sharing_meta> file_sharing_meta_data_t;

        typedef async_handler<void> default_handler_t;
        typedef async_handler<downloaded_file_info> file_info_handler_t;
        typedef async_handler<preview_proxy::link_meta> link_meta_handler_t;
        typedef async_handler<file_sharing_meta> file_sharing_meta_handler_t;

        struct cancelled_tasks
        {
            std::unordered_map<std::string, std::vector<int64_t>> tasks_;

            mutable std::mutex mutex_;

            bool remove(const std::string& _key)
            {
                using it_type = decltype(tasks_)::const_iterator;
                decltype(tasks_.extract(it_type())) to_erase;
                std::scoped_lock lock(mutex_);
                if (auto it = tasks_.find(_key); it != tasks_.end())
                {
                    to_erase = tasks_.extract(it);
                    return true;
                }
                return false;
            }

            bool remove(const std::string& _key, int64_t _seq)
            {
                using it_type = decltype(tasks_)::const_iterator;
                decltype(tasks_.extract(it_type())) to_erase;
                std::scoped_lock lock(mutex_);
                if (const auto it = tasks_.find(_key); it != tasks_.end())
                {
                    auto& seqs = it->second;
                    const auto r = std::remove_if(seqs.begin(), seqs.end(), [_seq](auto x) { return x == _seq; });
                    if (r != seqs.end())
                    {
                        seqs.erase(r, seqs.end());
                        if (seqs.empty())
                            to_erase = tasks_.extract(it);
                        return true;
                    }
                }
                return false;
            }

            void add(const std::string& _key, int64_t _seq)
            {
                std::scoped_lock lock(mutex_);
                auto& node = tasks_[_key];
                node.push_back(_seq);
            }

            bool contains(const std::string& _key, int64_t _seq) const
            {
                std::scoped_lock lock(mutex_);
                if (const auto it = tasks_.find(_key); it != tasks_.end())
                {
                    const auto& seqs = it->second;
                    return std::any_of(seqs.begin(), seqs.end(), [_seq](auto x) { return x == _seq; });
                }
                return false;
            }
        };

        struct async_loader_log_params
        {
            bool need_log_ = false;
            std::string log_str_;
            log_replace_functor log_functor_;
        };

        enum class load_error_type
        {
            network_error,
            load_failure,
            parse_failure,
            no_preview,
            cancelled
        };

        template<typename T>
        struct download_params
        {
            priority_t priority_ = default_priority();
            std::string url_;
            std::string base_url_;
            std::string normalized_url_;
            std::wstring file_name_;
            wim_packet_params wim_params_;
            time_t last_modified_time_ = 0;
            int64_t id_ = -1;
            async_loader_log_params log_params_;
            bool is_binary_data_ = false;
            T handler_;

            download_params(wim_packet_params _wim_params) : wim_params_(std::move(_wim_params)) {}
        };
        using download_params_with_info_handler = download_params<file_info_handler_t>;
        using download_params_with_default_handler = download_params<default_handler_t>;

        void log_error(std::string_view _context, std::string_view _url, load_error_type _type, bool _full_log);

        class async_loader
            : public std::enable_shared_from_this<async_loader>
        {
        public:

            explicit async_loader(std::wstring _content_cache_dir);

            void set_download_dir(std::wstring _download_dir);

            void download(download_params_with_default_handler _params);
            void download_file(download_params_with_info_handler _params);

            void cancel(std::string _url, std::optional<int64_t> _seq);

            void download_image_metainfo(const std::string& _url, const wim_packet_params& _wim_params, link_meta_handler_t _handler = link_meta_handler_t(), int64_t _id = -1, std::string_view _log_str = {});
            void download_file_sharing_metainfo(const std::string& _url, const wim_packet_params& _wim_params, file_sharing_meta_handler_t _handler = file_sharing_meta_handler_t(), int64_t _id = -1);

            void download_image_preview(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params, link_meta_handler_t _metainfo_handler = link_meta_handler_t(), file_info_handler_t _preview_handler = file_info_handler_t(), int64_t _id = -1);

            void download_image(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params, const bool _use_proxy, const bool _is_external_resource, file_info_handler_t _preview_handler = file_info_handler_t(), int64_t _id = -1);
            void download_image(priority_t _priority, const std::string& _url, const std::string& _file_name, const wim_packet_params& _wim_params, const bool _use_proxy, const bool _is_external_resource, file_info_handler_t _preview_handler = file_info_handler_t(), int64_t _id = -1);

            void download_file_sharing(priority_t _priority, const std::string& _contact, const std::string& _url, std::string _file_name, const wim_packet_params& _wim_params, int64_t _id, file_info_handler_t _handler = file_info_handler_t());
            void cancel_file_sharing(std::string _url, std::optional<int64_t> _seq);

            void resume_suspended_tasks(const wim_packet_params& _wim_params);

            void contact_switched(const std::string& _contact);

            std::wstring get_meta_path(std::string_view _url, const std::wstring& _path);

            static void save_filesharing_local_path(const std::wstring& _meta_path, std::string_view _url, const std::wstring& _path);

            void save_filesharing_local_path(std::string_view _url, const std::wstring& _path);

            std::string path_in_cache(const std::string& _url);

            static async_loader_log_params make_filesharing_log_params(std::string_view _fs_id);

        private:
            void download_file_sharing_impl(std::string _url, wim_packet_params _wim_params, downloadable_file_chunks_ptr _file_chunks, int64_t _id, std::string_view _normalized_url = {});

            static void update_file_chunks(downloadable_file_chunks& _file_chunks, priority_t _new_priority, file_info_handler_t _additional_handlers, int64_t _id);

            template <typename T>
            static void fire_callback(loader_errors _error, transferred_data<T> _data, typename async_handler<T>::completion_callback_t _completion_callback)
            {
                if (_completion_callback)
                    g_core->execute_core_context([_error, _data = std::move(_data), _completion_callback = std::move(_completion_callback)]() { _completion_callback(_error, _data); });
            }

            void fire_chunks_callback(loader_errors _error, const std::string& _url);

            template <class metainfo_parser_t, typename T>
            void download_metainfo(const std::string& _url, const std::string& _signed_url, metainfo_parser_t _parser, const wim_packet_params& _wim_params, async_handler<T> _handler, int64_t _id, std::string_view _normalized_url = {}, async_loader_log_params _log_params = {})
            {
                __INFO("async_loader",
                    "download_metainfo\n"
                    "url      = <%1%>\n"
                    "signed   = <%2%>\n"
                    "handler  = <%3%>\n", _url % _signed_url % _handler.to_string());

                auto meta_path = get_path_in_cache(content_cache_dir_, _url, path_type::link_meta);
                auto load_from_local = [_handler, _parser, _url, meta_path]()
                {
                    tools::binary_stream json_file;
                    if (json_file.load_from_file(meta_path))
                    {
                        const auto file_size = json_file.available();
                        if (file_size != 0)
                        {
                            const auto json_str = json_file.read(file_size);

                            std::vector<char> json;
                            json.reserve(file_size + 1);

                            json.assign(json_str, json_str + file_size);
                            json.push_back('\0');

                            auto meta_info = _parser(json.data(), _url);
                            if (meta_info)
                            {
                                const auto status_code = meta_info->get_status_code();
                                if (status_code != 0)
                                {
                                    if (status_code == 1000)
                                    {
                                        g_core->write_string_to_network_log("download_metainfo's been delayed by the server. It's going to be repeated in 1 minute\r\n");
                                        tools::system::delete_file(meta_path);
                                    }

                                    fire_callback(status_code == 1000 ? loader_errors::come_back_later : loader_errors::http_client_error, transferred_data<T>(), _handler.completion_callback_);
                                    return true;
                                }

                                transferred_data<T> result(std::shared_ptr<T>(meta_info.release()));
                                fire_callback(loader_errors::success, result, _handler.completion_callback_);
                                return true;
                            }
                        }
                    }

                    return false;
                };

                if (load_from_local())
                    return;

                auto local_handler = default_handler_t([_url, _signed_url, _parser, _handler, meta_path, _id, load_from_local, _log_params, full_log = _wim_params.full_log_, this](loader_errors _error, const default_data_t& _data)
                {
                    __INFO("async_loader",
                        "download_metainfo\n"
                        "url      = <%1%>\n"
                        "handler  = <%2%>\n"
                        "result   = <%3%>\n"
                        "response = <%4%>\n", _url % _handler.to_string() % static_cast<int>(_error) % _data.response_code_);

                    if (_error == loader_errors::network_error)
                    {
                        log_error("download_metainfo", _url, load_error_type::network_error, full_log);
                        suspended_tasks_.push([_url, _signed_url, _parser, _handler, _id, _log_params, this](const wim_packet_params& wim_params)
                        {
                            download_metainfo(_url, _signed_url, _parser, wim_params, _handler, _id, {}, _log_params);
                        });
                        return;
                    }

                    if (_error != loader_errors::success)
                    {
                        log_error("download_metainfo", _url, load_error_type::load_failure, full_log);
                        fire_callback(_error, transferred_data<T>(), _handler.completion_callback_);
                        return;
                    }

                    if (load_from_local())
                        return;

                    log_error("download_metainfo", _url, load_error_type::parse_failure, full_log);
                    fire_callback(loader_errors::invalid_json, transferred_data<T>(), _handler.completion_callback_);

                }, _handler.progress_callback_);

                download_params_with_default_handler params(_wim_params);
                params.priority_ = highest_priority();
                params.url_ = _signed_url;
                params.base_url_ = _url;
                params.file_name_ = std::move(meta_path);
                params.handler_ = std::move(local_handler);
                params.id_ = _id;
                params.normalized_url_ = _normalized_url;
                params.log_params_ = _log_params;
                download(std::move(params));
            }

            void cleanup_cache();

        private:
            const std::wstring content_cache_dir_;

            std::wstring download_dir_;

            std::unordered_map<std::string, downloadable_file_chunks_ptr> in_progress_;
            std::mutex in_progress_mutex_;

            std::shared_ptr<cancelled_tasks> cancelled_tasks_;

            typedef std::function<void(const wim_packet_params& _wim_params)> suspended_task_t;
            std::queue<suspended_task_t> suspended_tasks_;

            std::shared_ptr<async_executer> async_tasks_;
            std::shared_ptr<async_executer> file_save_thread_;
        };
    }
}
