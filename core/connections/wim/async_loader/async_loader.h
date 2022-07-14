#pragma once

#include "../loader/loader_helpers.h"
#include "../loader/preview_proxy.h"
#include "../loader/web_file_info.h"

#include "async_handler.h"
#include "downloadable_file_chunks.h"
#include "downloaded_file_info.h"
#include "file_sharing_meta.h"
#include "../wim_packet.h"
#include "../log_replace_functor.h"

#include "../../../log/log.h"

#include "../../../corelib/collection_helper.h"
#include "../../../corelib/enumerations.h"

namespace core
{
    class coll_helper;

    namespace tools
    {
        class filesharing_id;
    }

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

        struct async_loader_log_params
        {
            std::string log_str_;
            log_replace_functor log_functor_;
            bool need_log_ = false;
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
            file_key file_key_; // used in cancelled tasks
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

        class async_loader : public std::enable_shared_from_this<async_loader>
        {
        public:

            explicit async_loader(std::wstring _content_cache_dir);

            void set_download_dir(std::wstring _download_dir);

            void download(download_params_with_default_handler _params);
            void download_file(download_params_with_info_handler _params);

            void cancel(file_key _key, std::optional<int64_t> _seq);

            void download_image_metainfo(const std::string& _url, const wim_packet_params& _wim_params, link_meta_handler_t _handler = link_meta_handler_t(), int64_t _id = -1, std::string_view _log_str = {});
            void download_file_sharing_metainfo(const core::tools::filesharing_id& _fs_id, const wim_packet_params& _wim_params, file_sharing_meta_handler_t _handler = file_sharing_meta_handler_t(), int64_t _id = -1);

            void download_image_preview(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params, link_meta_handler_t _metainfo_handler = link_meta_handler_t(), file_info_handler_t _preview_handler = file_info_handler_t(), int64_t _id = -1);

            void download_image(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params, const bool _use_proxy, const bool _is_external_resource, file_info_handler_t _preview_handler = file_info_handler_t(), int64_t _id = -1);
            void download_image(priority_t _priority, const std::string& _url, const std::string& _file_name, const wim_packet_params& _wim_params, const bool _use_proxy, const bool _is_external_resource, file_info_handler_t _preview_handler = file_info_handler_t(), int64_t _id = -1);

            void download_file_sharing(priority_t _priority, const std::string& _contact, const std::string& _url, std::string _file_name, const wim_packet_params& _wim_params, int64_t _id, file_info_handler_t _handler = file_info_handler_t());
            void cancel_file_sharing(const std::string& _url, std::optional<int64_t> _seq);

            void resume_suspended_tasks(const wim_packet_params& _wim_params);

            void contact_switched(const std::string& _contact);

            std::wstring get_meta_path(const file_key& _file_key);

            static void update_filesharing_meta(const std::wstring& _meta_path, std::function<void(file_sharing_meta_uptr&)> meta_handler);
            static void save_filesharing_local_path(const std::wstring& _meta_path, const std::wstring& _path);
            static void save_filesharing_antivirus_check_result(const std::wstring& _meta_path, core::antivirus::check::result _result);
            static void save_metainfo(const file_sharing_meta& _meta, const std::wstring& _path);

            std::string path_in_cache(const std::string& _url);

            static async_loader_log_params make_filesharing_log_params(const core::tools::filesharing_id& _fs_id);

            auto file_save_thread() const { return file_save_thread_; }

        private:
            void download_file_sharing_impl(std::string _url, wim_packet_params _wim_params, downloadable_file_chunks_ptr _file_chunks, int64_t _id, std::string_view _normalized_url = {});

            static void update_file_chunks(downloadable_file_chunks& _file_chunks, priority_t _new_priority, file_info_handler_t _additional_handlers, int64_t _id);

            template <typename T>
            static void fire_callback(loader_errors _error, transferred_data<T> _data, typename async_handler<T>::completion_callback_t _completion_callback)
            {
                if (_completion_callback)
                    g_core->execute_core_context({ [_error, _data = std::move(_data), _completion_callback = std::move(_completion_callback)] () { _completion_callback(_error, _data); } });
            }

            template <class metainfo_parser_t, typename T>
            static bool load_from_local(async_handler<T> _handler, metainfo_parser_t _parser, std::wstring_view _path);

            template <class metainfo_parser_t, typename T>
            default_handler_t create_local_handler(const file_key& _file_key, const std::string& _signed_url, metainfo_parser_t _parser, async_handler<T> _handler, const std::wstring& _meta_path, int64_t _id, async_loader_log_params _log_params, bool _full_log);

            void fire_chunks_callback(loader_errors _error, const std::string& _url);
            template <class metainfo_parser_t, typename T>
            void download_metainfo(const file_key& _file_key,
                const std::string& _signed_url, metainfo_parser_t _parser, const wim_packet_params& _wim_params,
                async_handler<T> _handler, int64_t _id, std::string_view _normalized_url = {},
                async_loader_log_params _log_params = {});

            void cleanup_cache();

        private:
            const std::wstring content_cache_dir_;

            std::wstring download_dir_;

            std::unordered_map<std::string, downloadable_file_chunks_ptr> in_progress_;
            std::mutex in_progress_mutex_;

            std::shared_ptr<cancelled_tasks> cancelled_tasks_;

            using suspended_task_t = std::function<void(const wim_packet_params& _wim_params)>;

            std::queue<suspended_task_t> suspended_tasks_;
            std::shared_ptr<async_executer> async_tasks_;
            std::shared_ptr<async_executer> file_save_thread_;
        };


        template <class metainfo_parser_t, typename T>
        bool async_loader::load_from_local(async_handler<T> _handler, metainfo_parser_t _parser, std::wstring_view _path)
        {
            tools::binary_stream json_file;
            if (json_file.load_from_file(_path))
            {
                if (const auto file_size = json_file.available())
                {
                    const auto json_str = json_file.read(file_size);

                    std::vector<char> json;
                    json.reserve(file_size + 1);

                    json.assign(json_str, json_str + file_size);
                    json.push_back('\0');

                    if (auto meta_info = _parser(json.data()))
                    {
                        const auto status_code = meta_info->get_status_code();
                        if (status_code != 0 && status_code != 20000 && status_code != 20002 && status_code != 200)
                        {
                            const auto need_to_repeat = (status_code == 1000 || status_code == 20001);
                            if (need_to_repeat)
                            {
                                g_core->write_string_to_network_log("download_metainfo's been delayed by the server. It's going to be repeated\r\n");
                                tools::system::delete_file(std::wstring(_path));
                            }

                            fire_callback(need_to_repeat ? loader_errors::come_back_later : loader_errors::http_client_error, transferred_data<T>(), _handler.completion_callback_);
                            return true;
                        }

                        transferred_data<T> result(std::shared_ptr<T>(meta_info.release()));
                        fire_callback(loader_errors::success, result, _handler.completion_callback_);
                        return true;
                    }
                }
            }

            return false;
        }

        template <class metainfo_parser_t, typename T>
        default_handler_t async_loader::create_local_handler(const file_key& _file_key, const std::string& _signed_url, metainfo_parser_t _parser, async_handler<T> _handler, const std::wstring& _meta_path, int64_t _id, async_loader_log_params _log_params, bool _full_log)
        {
            auto local_handler = default_handler_t([key = _file_key, _signed_url, _parser, _handler, _meta_path, _id, _log_params, _full_log, wr_this = weak_from_this()](loader_errors _error, const default_data_t& _data)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                __INFO("async_loader",
                    "download_metainfo\n"
                    "url      = <%1%>\n"
                    "handler  = <%2%>\n"
                    "result   = <%3%>\n"
                    "response = <%4%>\n", key.to_string() % _handler.to_string() % static_cast<int>(_error) % _data.response_code_);

                if (_error == loader_errors::network_error)
                {
                    log_error("download_metainfo", key.to_string(), load_error_type::network_error, _full_log);
                    ptr_this->suspended_tasks_.push([key, _signed_url, _parser, _handler, _id, _log_params, wr_this](const wim_packet_params& wim_params)
                    {
                        if (auto ptr = wr_this.lock())
                            ptr->download_metainfo(key, _signed_url, _parser, wim_params, _handler, _id, {}, _log_params);
                    });
                    return;
                }

                if (_error != loader_errors::success)
                {
                    log_error("download_metainfo", key.to_string(), load_error_type::load_failure, _full_log);
                    fire_callback(_error, transferred_data<T>(), _handler.completion_callback_);
                    return;
                }

                auto data = std::static_pointer_cast<core::wim::downloaded_file_info>(_data.additional_data_);
                const auto has_tmp_file = data && !data->local_path_.empty();
                ptr_this->file_save_thread_->run_async_function([wr_this, has_tmp_file, tmp_file_name = std::move(data->local_path_), _meta_path, _full_log, _handler, _parser, key]()
                {
                    if (load_from_local(_handler, _parser, has_tmp_file ? tmp_file_name : _meta_path))
                    {
                        // delete tmp_file_name after processing
                        // inside load_from_local() the tmp_file_name can be deleted by "need_to_repeat" reason
                        // so delete meta_path too
                        if (has_tmp_file)
                            tools::system::delete_file(!tools::system::is_exist(tmp_file_name) ? _meta_path : tmp_file_name);
                        return 0;
                    }

                    log_error("download_metainfo", key.to_string(), load_error_type::parse_failure, _full_log);
                    fire_callback(loader_errors::invalid_json, transferred_data<T>(), _handler.completion_callback_);
                    return 0;
                });

            }, _handler.progress_callback_);
            return local_handler;
        }

        template <class metainfo_parser_t, typename T>
        void async_loader::download_metainfo(const core::wim::file_key& _file_key, const std::string& _signed_url, metainfo_parser_t _parser, const wim_packet_params& _wim_params, async_handler<T> _handler, int64_t _id, std::string_view _normalized_url, async_loader_log_params _log_params)
        {
            __INFO("async_loader",
                "download_metainfo\n"
                "url      = <%1%>\n"
                "signed   = <%2%>\n"
                "handler  = <%3%>\n", _file_key.to_string() % _signed_url % _handler.to_string());

            auto meta_path = get_meta_path(_file_key);

            file_save_thread_->run_t_async_function<bool>([_handler, _parser, meta_path]()
            {
                return load_from_local(_handler, _parser, meta_path);
            })->on_result_ = [_file_key, _signed_url, _parser, _handler, meta_path, _id, _log_params, _wim_params, wr_this = weak_from_this(), _normalized_url](bool _read_result) mutable
            {
                if (_read_result)
                    return;
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;
                auto local_handler = ptr_this->create_local_handler(_file_key, _signed_url, _parser, std::move(_handler), meta_path, _id, _log_params, _wim_params.full_log_);
                download_params_with_default_handler params(_wim_params);
                params.priority_ = highest_priority();
                params.url_ = _signed_url;
                params.file_key_ = _file_key;
                params.file_name_ = std::move(meta_path);
                params.handler_ = std::move(local_handler);
                params.id_ = _id;
                params.normalized_url_ = _normalized_url;
                params.log_params_ = _log_params;
                ptr_this->download(std::move(params));
            };
        }
    } // namespace wim
} // namespace core
