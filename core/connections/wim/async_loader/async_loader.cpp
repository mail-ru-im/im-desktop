#include "stdafx.h"

#include "../loader/loader_helpers.h"

#include "../wim_packet.h"

#include "../../../core.h"
#include "../../../http_request.h"
#include "../../../network_log.h"
#include "../../../tools/file_sharing.h"
#include "../../../tools/system.h"
#include "../../../utils.h"
#include "../../../configuration/app_config.h"
#include "../../urls_cache.h"
#include "async_loader.h"

#include "quarantine/quarantine.h"

namespace
{
    constexpr std::string_view referrer_url() noexcept
    {
        return "https://icq.com";
    }
}


core::wim::async_loader::async_loader(std::wstring _content_cache_dir)
    : content_cache_dir_(std::move(_content_cache_dir))
    , cancelled_tasks_(std::make_shared<cancelled_tasks>())
    , async_tasks_(std::make_shared<async_executer>("al async tasks", 3))
    , file_save_thread_(std::make_shared<async_executer>("al file save", 1))
{
}

void core::wim::async_loader::set_download_dir(std::wstring _download_dir)
{
    download_dir_ = std::move(_download_dir);
}

void core::wim::async_loader::download(priority_t _priority, const std::string& _url, const std::string& _base_url, const wim_packet_params& _wim_params,
                                       default_handler_t _handler, time_t _last_modified_time, int64_t _id, std::string_view _normalized_url)
{
    __INFO("async_loader",
        "download\n"
        "url      = <%1%>\n"
        "handler  = <%2%>\n", _url % _handler.to_string());

    auto user_proxy = g_core->get_proxy_settings();

    auto wr_this = weak_from_this();

    auto wim_stop_handler = _wim_params.stop_handler_;

    auto stop = [wim_stop_handler, _base_url, cancelled = cancelled_tasks_]()
    {
        if (wim_stop_handler && wim_stop_handler())
            return true;

        std::lock_guard<std::mutex> lock(cancelled->mutex_);
        auto it = cancelled->tasks_.find(_base_url);
        if (it != cancelled->tasks_.end())
        {
            cancelled->tasks_.erase(it);
            return true;
        }

        return false;
    };

    auto request = std::make_shared<http_request_simple>(user_proxy, utils::get_user_agent(_wim_params.aimid_), std::move(stop), _handler.progress_callback_);

    request->set_url(_url);
    request->set_need_log(_wim_params.full_log_);
    request->set_keep_alive();
    request->set_priority(_priority);
    request->set_id(_id);

    if (!_normalized_url.empty())
        request->set_normalized_url(_normalized_url);
    else
        request->set_normalized_url(get_endpoint_for_url(_url));

    if (_last_modified_time != 0)
        request->set_modified_time_condition(_last_modified_time - _wim_params.time_offset_);

    auto full_log = _wim_params.full_log_;
    const auto start = std::chrono::steady_clock().now();
    request->get_async([start, _url, _base_url, request, _handler, full_log, cancelled = cancelled_tasks_](curl_easy::completion_code _completion_code)
    {
        const auto finish = std::chrono::steady_clock().now();

        {
            std::lock_guard<std::mutex> lock(cancelled->mutex_);
            auto it = cancelled->tasks_.find(_base_url);
            if (it != cancelled->tasks_.end())
                cancelled->tasks_.erase(it);
        }

        tools::binary_stream bs;
        bs.write(_url.c_str(), _url.size());

        if (!full_log)
        {
            log_replace_functor f;
            f.add_marker("a");
            f.add_marker("file_id");
            f.add_marker("url");
            f.add_url_marker("files.icq.com/preview", tail_from_last_range_evaluator('/'));
            auto config_url = core::configuration::get_app_config().get_url_files();
            if (!config_url.empty())
            {
                config_url.append("/preview");
                f.add_url_marker(config_url.c_str(), tail_from_last_range_evaluator('/'));
            }
            f.add_url_marker("go.imgsmail.ru/imgpreview", tail_from_last_range_evaluator('?'));
            f(bs);
        }

        std::stringstream log;
        log << "async request result:" << request->get_response_code() << (_completion_code ==  curl_easy::completion_code::success ? " (success)" : " (error)") << '\n';
        log << "url: " << bs.read_available() << '\n';
        log << "completed in " << std::chrono::duration_cast<std::chrono::milliseconds>(finish -start).count() << " ms\n";

        g_core->write_string_to_network_log(log.str());

        __INFO("async_loader",
            "download\n"
            "url      = <%1%>\n"
            "handler  = <%2%>\n"
            "success  = <%3%>\n"
            "response = <%4%>\n", _url % _handler.to_string() % logutils::yn(_completion_code == curl_easy::completion_code::success) % request->get_response_code());

        if (_completion_code ==  curl_easy::completion_code::success)
        {
            if (request->get_response_code() == 200)
            {
                if (_handler.completion_callback_)
                {
                    default_data_t data(request->get_response_code(), request->get_header(), std::static_pointer_cast<tools::binary_stream>(request->get_response()));
                    _handler.completion_callback_(loader_errors::success, data);
                }
            }
            else
            {
                fire_callback(loader_errors::http_error, default_data_t(request->get_response_code()), _handler.completion_callback_);
            }
        }
        else if (_completion_code ==  curl_easy::completion_code::cancelled)
        {
            fire_callback(loader_errors::cancelled, default_data_t(), _handler.completion_callback_);
        }
        else
        {
            fire_callback(loader_errors::network_error, default_data_t(), _handler.completion_callback_);
        }
    });
}

void core::wim::async_loader::cancel(const std::string& _url)
{
    std::lock_guard<std::mutex> lock(cancelled_tasks_->mutex_);

    cancelled_tasks_->tasks_.insert(_url);
}

void core::wim::async_loader::download_file(priority_t _priority, const std::string& _url, const std::string& _base_url, const std::wstring& _file_name, const wim_packet_params& _wim_params,
                                            async_handler<downloaded_file_info> _handler, time_t _last_modified_time, int64_t _id, const bool _with_data, std::string_view _normalized_url)
{
    download_file(_priority, _url, _base_url, tools::from_utf16(_file_name), _wim_params, _handler, _last_modified_time, _id, _with_data, _normalized_url);
}

void core::wim::async_loader::download_file(priority_t _priority, const std::string& _url, const std::string& _base_url, const std::string& _file_name, const wim_packet_params& _wim_params,
                                            async_handler<downloaded_file_info> _handler, time_t _last_modified_time, int64_t _id, const bool _with_data, std::string_view _normalized_url)
{
    __INFO("async_loader",
        "download_file\n"
        "url      = <%1%>\n"
        "file     = <%2%>\n"
        "handler  = <%3%>\n", _url % _file_name % _handler.to_string());

    {
        std::lock_guard<std::mutex> lock(cancelled_tasks_->mutex_);
        auto it = cancelled_tasks_->tasks_.find(_base_url);
        if (it != cancelled_tasks_->tasks_.end())
            cancelled_tasks_->tasks_.erase(it);
    }

    auto local_handler = default_handler_t([_url, _file_name, _handler, wr_this = weak_from_this()](loader_errors _error, const default_data_t& _data)
    {
        __INFO("async_loader",
            "download_file\n"
            "url      = <%1%>\n"
            "file     = <%2%>\n"
            "handler  = <%3%>\n"
            "result   = <%4%>\n"
            "response = <%5%>\n", _url % _file_name % _handler.to_string() % static_cast<int>(_error) % _data.response_code_);

        const auto file_name_utf16 = tools::from_utf8(_file_name);
        file_info_data_t data(_data, std::make_shared<downloaded_file_info>(_url, file_name_utf16));

        if (_error != loader_errors::success)
        {
            fire_callback(_error, data, _handler.completion_callback_);
            return;
        }

        auto ptr_this = wr_this.lock();
        if (!ptr_this)
        {
            fire_callback(loader_errors::save_2_file, data, _handler.completion_callback_);
            return;
        }

        ptr_this->file_save_thread_->run_async_function([data, _handler, file_name_utf16, _url]
        {
            if (!data.content_->save_2_file(file_name_utf16))
            {
                fire_callback(loader_errors::save_2_file, data, _handler.completion_callback_);
                return 0;
            }
            data.content_->reset_out();

            quarantine::quarantine_file({ file_name_utf16, _url, referrer_url() });

            fire_callback(loader_errors::success, data, _handler.completion_callback_);
            return 0;
        });

    }, _handler.progress_callback_);

    async_tasks_->run_async_function([_file_name, wr_this = weak_from_this(), _priority, _url, _base_url, _wim_params, local_handler, _last_modified_time, _id, _with_data, _handler, normalized_url = std::string(_normalized_url)]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 1;

        {

            std::lock_guard<std::mutex> lock(ptr_this->cancelled_tasks_->mutex_);
            auto it = ptr_this->cancelled_tasks_->tasks_.find(_base_url);
            if (it != ptr_this->cancelled_tasks_->tasks_.end())
            {
                ptr_this->cancelled_tasks_->tasks_.erase(it);
                file_info_data_t empty_data(default_data_t(), std::make_shared<downloaded_file_info>(_url, std::wstring()));
                ptr_this->fire_callback(loader_errors::cancelled, empty_data, _handler.completion_callback_);
                return 1;
            }
        }

        if (_last_modified_time == 0 && core::tools::system::is_exist(_file_name))
        {
            const auto file_name_utf16 = core::tools::from_utf8(_file_name);

            file_info_data_t data;
            data.content_ = std::make_shared<core::tools::binary_stream>();

            if (_with_data)
                data.content_->load_from_file(file_name_utf16);

            data.additional_data_ = std::make_shared<core::wim::downloaded_file_info>(_url, file_name_utf16);

            ptr_this->fire_callback(loader_errors::success, data, _handler.completion_callback_);

        }
        else
        {
            ptr_this->download(_priority, _url, _base_url, _wim_params, local_handler, _last_modified_time, _id, normalized_url);
        }

        return 0;
    });
}

void core::wim::async_loader::download_image_metainfo(const std::string& _url, const wim_packet_params& _wim_params, link_meta_handler_t _handler, int64_t _id)
{
    const auto ext_params = preview_proxy::format_get_preview_params(_url);
    const auto signed_url = sign_loader_uri(preview_proxy::uri::get_preview(), _wim_params, ext_params);
    download_metainfo(_url, signed_url, preview_proxy::parse_json, _wim_params, _handler, _id, "filesDownloadSnippetMetadata");
}

void core::wim::async_loader::download_file_sharing_metainfo(const std::string& _url, const wim_packet_params& _wim_params, file_sharing_meta_handler_t _handler, int64_t _id)
{
    const auto id = tools::get_file_id(_url);
    assert(!id.empty());

    std::stringstream ss_url;

    const auto config_url = core::configuration::get_app_config().get_url_files_if_not_default();
    if (!config_url.empty())
        ss_url << std::string_view("https://") << config_url;
    else
        ss_url << urls::get_url(urls::url_type::files_host);

    ss_url << std::string_view("/getinfo?file_id=") << id;
    ss_url << "&r=" <<  core::tools::system::generate_guid();

    download_metainfo(_url, ss_url.str(), &file_sharing_meta::parse_json, _wim_params, _handler, _id, "filesDownloadMetadata");
}

void core::wim::async_loader::download_image_preview(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params,
    link_meta_handler_t _metainfo_handler, file_info_handler_t _preview_handler, int64_t _id)
{
    __INFO("async_loader",
        "download_image_preview\n"
        "url      = <%1%>\n"
        "mhandler = <%2%>\n"
        "phandler = <%3%>\n", _url % _metainfo_handler.to_string() % _preview_handler.to_string());

    auto wr_this = weak_from_this();

    auto local_handler = link_meta_handler_t([_priority, _url, _wim_params, _metainfo_handler, _preview_handler, _id, wr_this](loader_errors _error, const link_meta_data_t& _data)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        __INFO("async_loader",
            "download_metainfo\n"
            "url      = <%1%>\n"
            "mhandler = <%2%>\n"
            "phandler = <%3%>\n"
            "result   = <%4%>\n"
            "response = <%5%>\n", _url % _metainfo_handler.to_string() % _preview_handler.to_string() % static_cast<int>(_error) % _data.response_code_);

        file_info_data_t file_data(_data, std::make_shared<downloaded_file_info>(_url));

        if (_error == loader_errors::network_error)
        {
            ptr_this->suspended_tasks_.push([_priority, _url, _metainfo_handler, _preview_handler, wr_this](const wim_packet_params& wim_params)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->download_image_preview(_priority, _url, wim_params, _metainfo_handler, _preview_handler);
            });
            return;
        }

        if (_error != loader_errors::success)
        {
            fire_callback(_error, _data, _metainfo_handler.completion_callback_);

            fire_callback(_error, file_data, _preview_handler.completion_callback_);
            return;
        }

        fire_callback(_error, _data, _metainfo_handler.completion_callback_);

        const auto meta = *_data.additional_data_;

        if (!meta.has_preview_uri())
        {
            fire_callback(loader_errors::no_link_preview, file_data, _preview_handler.completion_callback_);
            return;
        }

        const auto preview_url = meta.get_preview_uri(0, 0);
        const auto file_path = get_path_in_cache(ptr_this->content_cache_dir_, preview_url, path_type::link_preview);

        ptr_this->download_file(_priority, preview_url, _url, file_path, _wim_params, _preview_handler, 0, _id);

    }, _metainfo_handler.progress_callback_);

    download_image_metainfo(_url, _wim_params, local_handler, _id);
}

void core::wim::async_loader::download_image(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params, const bool _use_proxy, const bool _is_external_resource, file_info_handler_t _handler, int64_t _id, const bool _with_data)
{
    download_image(_priority, _url, std::string(), _wim_params, _use_proxy, _is_external_resource, _handler, _id, _with_data);
}

void core::wim::async_loader::download_image(priority_t _priority, const std::string& _url, const std::string& _file_name, const wim_packet_params& _wim_params, const bool _use_proxy, const bool _is_external_resource, file_info_handler_t _handler, int64_t _id, const bool _with_data)
{
    const auto path = _file_name.empty()
        ? tools::from_utf16(get_path_in_cache(content_cache_dir_, _url, path_type::file))
        : _file_name;

    __INFO("async_loader",
        "download_image\n"
        "url      = <%1%>\n"
        "file     = <%2%>\n"
        "use_proxy= <%3%>\n"
        "handler  = <%4%>\n", _url % path % logutils::yn(_use_proxy) % _handler.to_string());

    auto local_handler = file_info_handler_t([_priority, _url, _file_name, path, _wim_params, _use_proxy, _is_external_resource, _handler, _id, wr_this = weak_from_this()](loader_errors _error, const file_info_data_t& _data)
    {
        __INFO("async_loader",
            "download_image\n"
            "url      = <%1%>\n"
            "file     = <%2%>\n"
            "use_proxy= <%3%>\n"
            "handler  = <%4%>\n"
            "result   = <%5%>\n"
            "response = <%6%>\n", _url % path % logutils::yn(_use_proxy) % _handler.to_string() % static_cast<int>(_error) % _data.response_code_);

        // for icq image previews http code 204 means: try it later
        if (_error == loader_errors::network_error || (!_is_external_resource && _data.response_code_ == 204))
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->suspended_tasks_.push([_priority, _url, _file_name, _use_proxy, _is_external_resource, _handler, _id, wr_this](const wim_packet_params& wim_params)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->download_image(_priority, _url, _file_name, wim_params, _use_proxy, _is_external_resource, _handler, _id);
            });
            return;
        }

        fire_callback(_error, _data, _handler.completion_callback_);

    }, _handler.progress_callback_);

    auto dl_url = _url;
    std::string_view endpoint;
    if (_use_proxy)
    {
        const auto ext_params = preview_proxy::format_get_url_content_params(dl_url);
        dl_url = sign_loader_uri(preview_proxy::uri::get_url_content(), _wim_params, ext_params);
        endpoint = std::string_view("filesDownloadSnippet");
    }
    download_file(_priority, dl_url, _url, path, _wim_params, local_handler, 0, _id, _with_data, endpoint);
}

void core::wim::async_loader::download_file_sharing(
    priority_t _priority, const std::string& _contact, const std::string& _url, std::string _file_name, const wim_packet_params& _wim_params, file_info_handler_t _handler)
{
    {
        std::lock_guard<std::mutex> lock(in_progress_mutex_);
        auto it = in_progress_.find(_url);
        if (it != in_progress_.end())
        {
            update_file_chunks(*it->second, _priority, _handler);
            return;
        }
    }

    bool force_request_metainfo = true;
    {
        const auto meta_path = get_path_in_cache(content_cache_dir_, _url, path_type::link_meta);
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

                auto meta_info = file_sharing_meta::parse_json(json.data(), _url);
                if (meta_info && !meta_info->local_path_.empty())
                {
                    boost::filesystem::wpath local(tools::from_utf8(meta_info->local_path_));
                    if (tools::system::is_exist(local))
                    {
                        boost::system::error_code e;
                        uint64_t last_modified = boost::filesystem::last_write_time(local, e);
                        if (tools::system::get_file_size(local.wstring()) == meta_info->file_size_ && last_modified == meta_info->last_modified_)
                        {
                            force_request_metainfo = false;
                            if (_file_name.empty())
                                _file_name = meta_info->local_path_;
                        }
                    }
                }
            }
        }
    }

    if (force_request_metainfo)
        tools::system::delete_file(get_path_in_cache(content_cache_dir_, _url, path_type::link_meta));

    download_file_sharing_metainfo(_url, _wim_params, file_sharing_meta_handler_t(
        [_priority, _contact, _url, _file_name, _wim_params, _handler, wr_this = weak_from_this()](loader_errors _error, const file_sharing_meta_data_t& _data)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == loader_errors::network_error)
            {
                ptr_this->suspended_tasks_.push([_priority, _contact, _url, _file_name, _handler, wr_this](const wim_packet_params& wim_params)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->download_file_sharing(_priority, _contact, _url, _file_name, wim_params, _handler);
                });
                return;
            }

            auto meta = _data.additional_data_;
            if (!meta)
            {
                fire_callback(loader_errors::network_error, file_info_data_t(), _handler.completion_callback_);
                return;
            }

            ptr_this->cleanup_cache();

            if (!meta->local_path_.empty() && !_file_name.empty())
            {
                boost::filesystem::wpath local(tools::from_utf8(meta->local_path_));
                boost::filesystem::wpath dest(tools::from_utf8(_file_name));

                if (tools::system::is_exist(local))
                {
                    boost::system::error_code e;
                    uint64_t last_modified = boost::filesystem::last_write_time(local, e);
                    if (tools::system::get_file_size(local.wstring()) == meta->file_size_ && last_modified == meta->last_modified_)
                    {
                        if (local != dest)
                        {
                            tools::system::copy_file(local.wstring(), dest.wstring());
                            if (_file_name.empty())
                            {
                                tools::system::delete_file(local.wstring());
                            }
                        }

                        file_info_data_t data(std::make_shared<core::wim::downloaded_file_info>(_url, dest.wstring()));
                        fire_callback(loader_errors::success, data, _handler.completion_callback_);
                        return;
                    }
                }
            }

            std::wstring file_path;

            auto generate_unique_path = [ptr_this, meta](const boost::filesystem::wpath& _path)
            {
                auto result = _path.wstring();
                int n = 0;
                while (tools::system::is_exist(result))
                {
                    const auto stem = _path.stem().wstring();
                    const auto extension = _path.extension().wstring();
                    std::wstringstream new_name;
                    new_name << '/' << stem << " (" << ++n << ')' << extension;
                    result = _path.parent_path().wstring() + new_name.str();
                }

                return result;
            };

            file_sharing_content_type content_type;

            tools::get_content_type_from_uri(_url, Out content_type);

            if (!_file_name.empty())
            {
                file_path = tools::from_utf8(_file_name);
            }
            else
            {
                switch (content_type.type_)
                {
                    case file_sharing_base_content_type::image:
                    case file_sharing_base_content_type::gif:
                    case file_sharing_base_content_type::video:
                    case file_sharing_base_content_type::ptt:
                        file_path = get_path_in_cache(ptr_this->content_cache_dir_, _url, path_type::file) + tools::from_utf8(boost::filesystem::extension(meta->file_name_short_));
                        break;
                    default:
                        file_path = generate_unique_path(boost::filesystem::wpath(ptr_this->download_dir_) / tools::from_utf8(meta->file_name_short_));
                        break;
                }
            }

            file_info_data_t data(std::make_shared<core::wim::downloaded_file_info>(_url, file_path));

            if (tools::system::is_exist(file_path))
            {
                boost::system::error_code e;
                uint64_t last_modified = boost::filesystem::last_write_time(file_path, e);
                if (tools::system::get_file_size(file_path) == meta->file_size_ && last_modified == meta->last_modified_)
                {
                    fire_callback(loader_errors::success, data, _handler.completion_callback_);
                    return;
                }
                else
                {
                    switch (content_type.type_)
                    {
                    case file_sharing_base_content_type::image:
                    case file_sharing_base_content_type::gif:
                    case file_sharing_base_content_type::video:
                    case file_sharing_base_content_type::ptt:
                        tools::system::delete_file(file_path);
                        break;
                    default:
                        if (_file_name.empty())
                            file_path = generate_unique_path(boost::filesystem::wpath(file_path));
                        else
                            tools::system::delete_file(file_path);
                        break;
                    }
                }
            }
            else
            {
                const auto dir = boost::filesystem::path(file_path).parent_path();
                tools::system::create_directory_if_not_exists(dir);
            }

            auto file_chunks = std::make_shared<downloadable_file_chunks>(_priority, _contact, meta->file_download_url_, file_path, meta->file_size_);
            file_chunks->downloaded_ = tools::system::get_file_size(file_chunks->tmp_file_name_);

            if (file_chunks->total_size_ == file_chunks->downloaded_)
            {
                if (!tools::system::move_file(file_chunks->tmp_file_name_, file_chunks->file_name_))
                {
                    fire_callback(loader_errors::move_file, data, _handler.completion_callback_);
                    return;
                }

                quarantine::quarantine_file({ file_chunks->file_name_, _url, referrer_url() });

                fire_callback(loader_errors::success, data, _handler.completion_callback_);
                return;
            }
            else if (file_chunks->total_size_ < file_chunks->downloaded_)
            {
                tools::system::delete_file(file_chunks->tmp_file_name_);
                file_chunks->downloaded_ = 0;
            }

            {
                std::lock_guard<std::mutex> lock(ptr_this->in_progress_mutex_);
                auto it = ptr_this->in_progress_.find(_url);
                if (it != ptr_this->in_progress_.end())
                {
                    update_file_chunks(*it->second, _priority, _handler);
                    return;
                }
                file_chunks->handlers_.push_back(_handler);
                ptr_this->in_progress_[_url] = file_chunks;
            }

            ptr_this->download_file_sharing_impl(_url, _wim_params, file_chunks);
        }));
}

void core::wim::async_loader::cancel_file_sharing(const std::string& _url)
{
    std::lock_guard<std::mutex> lock(in_progress_mutex_);
    auto it = in_progress_.find(_url);
    if (it != in_progress_.end())
        it->second->cancel_ = true;
}

void core::wim::async_loader::resume_suspended_tasks(const wim_packet_params& _wim_params)
{
    while (!suspended_tasks_.empty())
    {
        auto task = std::move(suspended_tasks_.front());
        suspended_tasks_.pop();
        if (task)
            task(_wim_params);
    }
}

void core::wim::async_loader::contact_switched(const std::string& _contact)
{
    const auto hash = std::hash<std::string>()(_contact);

    std::lock_guard<std::mutex> lock(in_progress_mutex_);
    for (auto& it : in_progress_)
    {
        const auto& contacts = it.second->contacts_;

        if (std::any_of(contacts.begin(), contacts.end(), [hash](auto x) { return x == hash; }))
        {
            it.second->priority_ = it.second->priority_on_start_;
        }
        else if (it.second->priority_ > highest_priority())
        {
            it.second->priority_ = low_priority();
        }
    }
}

std::wstring core::wim::async_loader::get_meta_path(const std::string& _url, const std::wstring& _path)
{
    return get_path_in_cache(content_cache_dir_, _url, path_type::link_meta);
}

void core::wim::async_loader::save_filesharing_local_path(const std::wstring& _meta_path, const std::string& _url, const std::wstring& _path)
{
    core::tools::binary_stream json_file;

    if (json_file.load_from_file(_meta_path))
    {
        const auto file_size = json_file.available();
        if (file_size != 0)
        {
            const auto json_str = json_file.read(file_size);
            json_file.close();

            std::vector<char> json;
            json.reserve(file_size + 1);

            json.assign(json_str, json_str + file_size);
            json.push_back('\0');

            auto meta_info = file_sharing_meta::parse_json(json.data(), _url);
            if (meta_info)
            {
                if (!_path.empty())
                {
                    const boost::filesystem::wpath path(_path);
                    boost::system::error_code e;
                    auto last_modified = boost::filesystem::last_write_time(path, e);
                    meta_info->local_path_ = tools::from_utf16(_path);
                    meta_info->last_modified_ = last_modified;
                }
                else
                {
                    meta_info->local_path_.clear();
                    meta_info->last_modified_ = 0;
                }

                rapidjson::Document doc(rapidjson::Type::kObjectType);
                meta_info->serialize(doc, doc.GetAllocator());

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);

                const auto json_string = rapidjson_get_string_view(buffer);

                if (!json_string.length())
                {
                    assert(false);
                    return;
                }

                core::tools::binary_stream bstream;
                bstream.write<std::string_view>(json_string);
                bstream.save_2_file(_meta_path);
            }
        }
    }
}

void core::wim::async_loader::save_filesharing_local_path(const std::string& _url, const std::wstring& _path)
{
    return save_filesharing_local_path(get_meta_path(_url, _path), _url, _path);
}

void core::wim::async_loader::download_file_sharing_impl(std::string _url, wim_packet_params _wim_params, downloadable_file_chunks_ptr _file_chunks, std::string_view _normalized_url)
{
    if (_file_chunks->cancel_)
    {
        tools::system::delete_file(_file_chunks->tmp_file_name_);
        fire_chunks_callback(loader_errors::cancelled, _url);
        return;
    }

    auto wr_this = weak_from_this();

    auto progress = [_file_chunks, wr_this](int64_t /*_total*/, int64_t _transferred, int32_t /*_in_percentages*/)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        downloadable_file_chunks::handler_list_t handler_list;

        {
            std::lock_guard<std::mutex> lock(ptr_this->in_progress_mutex_);
            handler_list = _file_chunks->handlers_;
        }

        const auto downloaded = _file_chunks->downloaded_ + _transferred;
        for (auto& handler : handler_list)
        {
            if (handler.progress_callback_)
            {
                g_core->execute_core_context([_file_chunks, handler, downloaded]()
                    {
                        handler.progress_callback_(_file_chunks->total_size_, downloaded, int32_t(downloaded / (_file_chunks->total_size_ / 100.0)));
                    });
            }
        }
    };

    auto wim_stop_handler = _wim_params.stop_handler_;

    auto stop = [_file_chunks, wim_stop_handler]()
    {
        if (wim_stop_handler && wim_stop_handler())
            return true;

        return _file_chunks->cancel_;
    };

    auto user_proxy = g_core->get_proxy_settings();

    auto request = std::make_shared<http_request_simple>(user_proxy, utils::get_user_agent(_wim_params.aimid_), std::move(stop), std::move(progress));

    request->set_url(_file_chunks->url_);
    request->set_need_log(_wim_params.full_log_);
    request->set_keep_alive();
    request->set_priority(_file_chunks->priority_);

    if (!_normalized_url.empty())
        request->set_normalized_url(_normalized_url);
    else
        request->set_normalized_url(get_endpoint_for_url(_file_chunks->url_));


    constexpr int64_t max_chunk_size = 512 * 1024;
    const auto bytes_left = _file_chunks->total_size_ - _file_chunks->downloaded_;
    const auto chunk_size = _file_chunks->priority_ <= highest_priority()
        ? bytes_left
        : bytes_left < max_chunk_size ? bytes_left : max_chunk_size;

    request->set_range(_file_chunks->downloaded_, _file_chunks->downloaded_ + chunk_size);

    const auto flags = _file_chunks->downloaded_ > 0
        ? std::ios::binary | std::ios::app
        : std::ios::binary | std::ios::trunc;

    auto tmp_file = tools::system::open_file_for_write(_file_chunks->tmp_file_name_, flags);

    if (!tmp_file.good())
    {
        fire_chunks_callback(loader_errors::save_2_file, _url);
        return;
    }

    request->set_output_stream(std::make_shared<tools::file_output_stream>(std::move(tmp_file)));

    request->get_async([_url, _wim_params, _file_chunks, request, wr_this](curl_easy::completion_code _completion_code) mutable
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        const auto code = request->get_response_code();
        const auto success = _completion_code == curl_easy::completion_code::success;
        if (success && (code == 206 || code == 200 || code == 201))
        {
            const auto size = request->get_response()->all_size();
            _file_chunks->downloaded_ += size;

            request->get_response()->close();

            if (_file_chunks->total_size_ == _file_chunks->downloaded_)
            {
                if (!tools::system::move_file(_file_chunks->tmp_file_name_, _file_chunks->file_name_))
                {
                    ptr_this->fire_chunks_callback(loader_errors::move_file, _url);
                    return;
                }

                quarantine::quarantine_file({ _file_chunks->file_name_, _url, referrer_url() });
                ptr_this->fire_chunks_callback(loader_errors::success, _url);
                return;
            }
            else if (_file_chunks->total_size_ < _file_chunks->downloaded_)
            {
                ptr_this->fire_chunks_callback(loader_errors::internal_logic_error, _url);
                return;
            }

            ptr_this->download_file_sharing_impl(_url, _wim_params, _file_chunks);
        }
        else
        {
            if (_file_chunks->cancel_ || code == 404)
            {
                request->get_response()->close();
                tools::system::delete_file(_file_chunks->tmp_file_name_);
                ptr_this->fire_chunks_callback(loader_errors::cancelled, _url);
                return;
            }

            ptr_this->suspended_tasks_.push([_url, _file_chunks, wr_this](const wim_packet_params& wim_params)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->download_file_sharing_impl(_url, wim_params, _file_chunks);
            });
        }
    });
}

void core::wim::async_loader::update_file_chunks(downloadable_file_chunks& _file_chunks, priority_t _new_priority, file_info_handler_t _additional_handlers)
{
    _file_chunks.handlers_.push_back(_additional_handlers);
    if (_file_chunks.priority_ > _new_priority)
    {
        _file_chunks.priority_on_start_ = _new_priority;
        _file_chunks.priority_ = _new_priority;
    }
}

void core::wim::async_loader::fire_chunks_callback(loader_errors _error, const std::string& _url)
{
    std::wstring file_name;

    downloadable_file_chunks_ptr file_chunks;

    {
        std::lock_guard<std::mutex> lock(in_progress_mutex_);
        auto it = in_progress_.find(_url);
        assert(it != in_progress_.end());
        if (it != in_progress_.end())
        {
            file_chunks = it->second;
            in_progress_.erase(it);
        }
    }

    if (file_chunks)
    {
        auto data = file_info_data_t(std::make_shared<downloaded_file_info>(_url, file_chunks->file_name_));

        for (auto& handler : file_chunks->handlers_)
        {
            if (handler.completion_callback_)
            {
                g_core->execute_core_context([=]()
                {
                    handler.completion_callback_(_error, data);
                });
            }
        }
    }
}

void core::wim::async_loader::cleanup_cache()
{
    constexpr int32_t max_files_to_delete = 100;
    constexpr auto delete_files_older = std::chrono::hours(7 * 24);
    constexpr auto cleanup_period = std::chrono::minutes(10);

    static std::chrono::system_clock::time_point last_cleanup_time = std::chrono::system_clock::now();

    std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();

    if (current_time - last_cleanup_time < cleanup_period)
        return;

    tools::binary_stream bs;
    bs.write<std::string_view>("Start cleanup files cache\r\n");

    last_cleanup_time = current_time;

    try
    {
        std::vector<boost::filesystem::wpath> files_to_delete;

        boost::filesystem::directory_iterator end_iter;

        boost::system::error_code error;

        if (boost::filesystem::exists(content_cache_dir_, error) && boost::filesystem::is_directory(content_cache_dir_, error))
        {
            for (boost::filesystem::directory_iterator dir_iter(content_cache_dir_, error); (dir_iter != end_iter) && (files_to_delete.size() < max_files_to_delete) && !error; dir_iter.increment(error))
            {
                if (boost::filesystem::is_regular_file(dir_iter->status()))
                {
                    const auto write_time = std::chrono::system_clock::from_time_t(boost::filesystem::last_write_time(dir_iter->path(), error));
                    if ((current_time - write_time) > delete_files_older)
                        files_to_delete.push_back(*dir_iter);
                }
            }
        }

        for (const auto& _file_path : files_to_delete)
        {
            boost::filesystem::remove(_file_path, error);

            bs.write<std::string_view>("Delete file: ");
            bs.write<std::string>(_file_path.string());
            bs.write<std::string_view>("\r\n");

        }
    }
    catch (const std::exception&)
    {

    }

    bs.write<std::string_view>("Finish cleanup\r\n");
    g_core->write_data_to_network_log(std::move(bs));
}

constexpr std::string_view core::wim::async_loader::get_endpoint_for_url(std::string_view _url)
{
    using modify_group = std::pair<std::string_view, std::string_view>;
    constexpr modify_group modifiers[] = {
        {"/preview/max/", "filesDirectDownloadPreview"},
        {"/get/", "filesDownload"},
        {"/getinfo", "filesDownloadMetadata"}
    };

    for (const auto& [marker, endpoint] : modifiers)
    {
        if (_url.find(marker) != std::string::npos)
            return endpoint;
    }

    return std::string_view("filesDownloadSnippetMetadata");
}
