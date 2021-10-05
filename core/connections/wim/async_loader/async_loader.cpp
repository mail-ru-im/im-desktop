#include "stdafx.h"

#include "../loader/loader_helpers.h"

#include "../wim_packet.h"

#include "../../../core.h"
#include "../../../http_request.h"
#include "../../../network_log.h"
#include "../../../tools/file_sharing.h"
#include "../../../tools/system.h"
#include "../../../../common.shared/json_helper.h"
#include "../../../tools/features.h"
#include "../../../utils.h"
#include "../../../configuration/host_config.h"
#include "../../urls_cache.h"
#include "async_loader.h"
#include "../common.shared/string_utils.h"
#include "../common.shared/config/config.h"

#include "connections/urls_cache.h"
#include "quarantine/quarantine.h"

namespace
{
    std::string_view referrer_url() noexcept
    {
        return config::get().url(config::urls::installer_help);
    }

    constexpr std::string_view get_endpoint_for_url(std::string_view _url)
    {
        using modify_group = std::pair<std::string_view, std::string_view>;
        constexpr modify_group modifiers[] = {
            {"/preview/max/", "filesDirectDownloadPreview"},
            {"/get/", "filesDownload"},
            {"/getinfo", "filesDownloadMetadata"}
        };

        for (const auto& [marker, endpoint] : modifiers)
        {
            if (_url.find(marker) != std::string_view::npos)
                return endpoint;
        }

        return std::string_view("filesDownloadSnippetMetadata");
    }

    bool is_webp_supported(std::string_view _url)
    {
        if (_url.find("/preview/max/") != std::string_view::npos)
            return features::is_webp_preview_accepted();
        else if (_url.find("/get/") != std::string_view::npos)
            return features::is_webp_original_accepted();
        return false;
    }

    void add_webp_if_needed(const std::shared_ptr<core::http_request_simple>& _request, std::string_view _url)
    {
        if (is_webp_supported(_url))
            _request->set_custom_header_param("Accept: */*, image/webp");
    }

    core::log_replace_functor make_default_log_replace_functor(std::string_view _url)
    {
        core::log_replace_functor f;
        f.add_marker("a");
        f.add_marker("url");
        f.add_marker("aimsid", core::aimsid_range_evaluator());
        f.add_url_marker("go.imgsmail.ru/imgpreview", core::tail_from_last_range_evaluator('?'));

        const auto files_url = core::urls::get_url(core::urls::url_type::files_info, core::urls::with_https::no);
        const auto files_preview_url = core::urls::get_url(core::urls::url_type::files_preview, core::urls::with_https::no);

        if (_url.find(files_url) != _url.npos || _url.find(files_preview_url) != _url.npos)
        {
            if (const auto pos = _url.find('?'); pos != _url.npos)
            {
                if (const auto id = core::tools::get_file_id(_url.substr(0, pos)); !id.empty())
                    f.add_url_marker(id, core::distance_range_evaluator(-std::ptrdiff_t(id.size())));
            }
        }

        return f;
    }

    using load_error_type = core::wim::load_error_type;
    constexpr std::string_view get_error(load_error_type _type) noexcept
    {
        switch (_type)
        {
            case load_error_type::network_error:
                return "network error";
            case load_error_type::load_failure:
                return "failed to load";
            case load_error_type::parse_failure:
                return "failed to parse meta";
            case load_error_type::no_preview:
                return "no preview in meta";
            case load_error_type::cancelled:
                return "cancelled";
            default:
                return "unhandled error";
        }
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

static std::string make_signed_url(std::string_view _url, std::string_view _aimsid)
{
    if (_url.find(core::urls::get_url(core::urls::url_type::files_preview, core::urls::with_https::no)) == std::string_view::npos)
        return std::string{ _url };

    const std::string_view delim = _url.find('?') == std::string_view::npos ? "/?" : "&";
    return su::concat(_url, delim, "aimsid=", _aimsid);
}

void core::wim::async_loader::download(download_params_with_default_handler _params)
{
    auto url = make_signed_url(_params.url_, _params.wim_params_.aimsid_);
    __INFO("async_loader",
        "download\n"
        "url      = <%1%>\n"
        "handler  = <%2%>\n", url % _params.handler_.to_string());

    auto user_proxy = g_core->get_proxy_settings();

    auto stop = [_params, cancelled = cancelled_tasks_]()
    {
        if (_params.wim_params_.stop_handler_ && _params.wim_params_.stop_handler_())
            return true;

        return cancelled->contains(_params.base_url_, _params.id_);
    };

    auto& log_params = _params.log_params_;
    if (log_params.log_functor_.is_null())
        log_params.log_functor_ = make_default_log_replace_functor(url);

    auto request = std::make_shared<http_request_simple>(user_proxy, utils::get_user_agent(_params.wim_params_.aimid_), _params.priority_, std::move(stop), _params.handler_.progress_callback_);

    request->set_url(url);
    add_webp_if_needed(request, url);
    request->set_need_log(_params.wim_params_.full_log_ || log_params.need_log_);
    request->set_write_data_log(_params.wim_params_.full_log_);
    request->set_keep_alive();
    request->set_id(_params.id_);
    if (log_params.need_log_ && !_params.wim_params_.full_log_)
        request->set_replace_log_function(log_params.log_functor_);

    if (!_params.normalized_url_.empty())
        request->set_normalized_url(_params.normalized_url_);
    else
        request->set_normalized_url(get_endpoint_for_url(url));

    if (_params.last_modified_time_ != 0)
        request->set_modified_time_condition(_params.last_modified_time_ - _params.wim_params_.time_offset_);

    std::wstring tmp_file_name = tools::system::create_temp_file_path();
    tools::system::create_empty_file(tmp_file_name);

    auto output_file = tools::system::open_file_for_write(tmp_file_name, std::ios::binary | std::ios::trunc);
    if (!output_file.good())
    {
        fire_callback(loader_errors::save_2_file, default_data_t(), _params.handler_.completion_callback_);
        return;
    }

    request->set_output_stream(std::make_shared<tools::file_output_stream>(std::move(output_file)));
    request->set_use_curl_decompresion(true);

    const auto start = std::chrono::steady_clock().now();
    request->get_async([wr_this = weak_from_this(), start, request, tmp_file_name = std::move(tmp_file_name), url = std::move(url), _params = std::move(_params), cancelled = cancelled_tasks_]
    (curl_easy::completion_code _completion_code) mutable
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        const auto full_log = _params.wim_params_.full_log_;
        const auto finish = std::chrono::steady_clock().now();

        cancelled->remove(_params.base_url_, _params.id_);

        tools::binary_stream bs;
        bs.write<std::string_view>(url);

        if (!full_log)
            _params.log_params_.log_functor_(bs);

        std::stringstream log;
        log << "async request result:" << request->get_response_code() << (_completion_code == curl_easy::completion_code::success ? " (success)" : " (error)") << '\n';
        log << "url: " << (bs.available() ? bs.read_available() : "INVALID OR EMPTY URL") << '\n';
        if (!_params.log_params_.log_str_.empty())
            log << _params.log_params_.log_str_ << std::endl;
        log << "completed in " << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << " ms\n";

        g_core->write_string_to_network_log(log.str());

        __INFO("async_loader",
            "download\n"
            "url      = <%1%>\n"
            "handler  = <%2%>\n"
            "success  = <%3%>\n"
            "response = <%4%>\n", url % _params.handler_.to_string() % logutils::yn(_completion_code == curl_easy::completion_code::success) % request->get_response_code());

        request->get_response()->close();
        ptr_this->file_save_thread_->run_async_function([wr_this, _completion_code, request = std::move(request), params = std::move(_params), tmp_file_name = std::move(tmp_file_name), url = std::move(url)]() mutable
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return 0;

            auto completion_callback = params.handler_.completion_callback_;
            if (_completion_code == curl_easy::completion_code::success)
            {
                const auto response_code = request->get_response_code();
                tools::system::create_directory_if_not_exists(boost::filesystem::wpath(params.file_name_).parent_path());
                if (response_code != 304 && response_code != 425)
                {
                    tools::system::delete_file(params.file_name_);
                    // for downloading the metainfo removing the temporary file has been moved to the callback body
                    const auto res = params.is_binary_data_
                        ? tools::system::move_file(tmp_file_name, params.file_name_)
                        : tools::system::copy_file(tmp_file_name, params.file_name_);
                    if (!res)
                    {
                        tools::system::delete_file(tmp_file_name);
                        fire_callback(loader_errors::move_file, default_data_t(), completion_callback);
                        return 0;
                    }

                }
                else if (tools::system::is_exist(tmp_file_name))
                {
                    tools::system::delete_file(tmp_file_name);
                }

                quarantine::quarantine_file({ params.file_name_, url, referrer_url() });

                if (response_code == 200)
                {
                    default_data_t data(request->get_response_code(), request->get_header());
                    data.additional_data_ = std::make_shared<core::wim::downloaded_file_info>(url, params.is_binary_data_ ? params.file_name_ : tmp_file_name);
                    fire_callback(loader_errors::success, std::move(data), completion_callback);
                }
                else if (response_code == 425)
                {
                    g_core->write_string_to_network_log("async_loader download: it's been delayed by the server and going to be repeated\r\n");
                    fire_callback(loader_errors::come_back_later, default_data_t(response_code), completion_callback);
                }
                else
                {
                    fire_callback(loader_errors::http_error, default_data_t(response_code), completion_callback);
                    if (response_code != 304)
                        tools::system::delete_file(params.file_name_);
                }
            }
            else
            {
                tools::system::delete_file(tmp_file_name);

                const auto full_log = params.wim_params_.full_log_;
                if (_completion_code == curl_easy::completion_code::cancelled)
                {
                    log_error("async_loader download", url, load_error_type::cancelled, full_log);
                    fire_callback(loader_errors::cancelled, default_data_t(), completion_callback);
                }
                else
                {
                    log_error("async_loader download", url, load_error_type::network_error, full_log);
                    fire_callback(loader_errors::network_error, default_data_t(), completion_callback);
                    if (_completion_code == curl_easy::completion_code::resolve_failed)
                        config::hosts::switch_to_ip_mode(url, (int)_completion_code);
                    else
                        config::hosts::switch_to_dns_mode(url, (int)_completion_code);
                }
            }
            return 0;
        });
    });
}

void core::wim::async_loader::cancel(std::string _url, std::optional<int64_t> _seq)
{
    if (_seq)
        cancelled_tasks_->add(_url, *_seq);
}

void core::wim::async_loader::download_file(download_params_with_info_handler _params)
{
   __INFO("async_loader",
       "download_file\n"
       "url      = <%1%>\n"
       "file     = <%2%>\n"
       "handler  = <%3%>\n", _params.url_ % tools::from_utf16(_params.file_name_) % _params.handler_.to_string());

    cancelled_tasks_->remove(_params.base_url_);

    auto local_handler = default_handler_t([url = _params.url_, file_name_utf16 = _params.file_name_, handler = _params.handler_, wr_this = weak_from_this()](loader_errors _error, const default_data_t& _data)
    {
        __INFO("async_loader",
            "download_file\n"
            "url      = <%1%>\n"
            "file     = <%2%>\n"
            "handler  = <%3%>\n"
            "result   = <%4%>\n"
            "response = <%5%>\n", url % tools::from_utf16(file_name_utf16) % handler.to_string() % static_cast<int>(_error) % _data.response_code_);

        file_info_data_t data(_data, std::make_shared<downloaded_file_info>(url, file_name_utf16));

        if (_error != loader_errors::success)
        {
            fire_callback(_error, std::move(data), handler.completion_callback_);
            return;
        }

        auto ptr_this = wr_this.lock();
        if (!ptr_this)
        {
            fire_callback(loader_errors::save_2_file, std::move(data), handler.completion_callback_);
            return;
        }

        fire_callback(loader_errors::success, std::move(data), handler.completion_callback_);
    }, _params.handler_.progress_callback_);

    async_tasks_->run_async_function([_params = std::move(_params), wr_this = weak_from_this(), local_handler]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 1;

        if (ptr_this->cancelled_tasks_->remove(_params.base_url_, _params.id_))
        {
            file_info_data_t empty_data(default_data_t(), std::make_shared<downloaded_file_info>(_params.url_, std::wstring()));
            fire_callback(loader_errors::cancelled, std::move(empty_data), _params.handler_.completion_callback_);
            return 1;
        }

        if (_params.last_modified_time_ == 0 && core::tools::system::is_exist(_params.file_name_))
        {
            file_info_data_t data;

            data.additional_data_ = std::make_shared<core::wim::downloaded_file_info>(_params.url_, _params.file_name_);

            fire_callback(loader_errors::success, std::move(data), _params.handler_.completion_callback_);
        }
        else
        {
            download_params_with_default_handler params(std::move(_params.wim_params_));
            params.priority_ = _params.priority_;
            params.url_ = std::move(_params.url_);
            params.base_url_ = std::move(_params.base_url_);
            params.normalized_url_ = std::move(_params.normalized_url_);
            params.file_name_ = std::move(_params.file_name_);
            params.last_modified_time_ = _params.last_modified_time_;
            params.id_ = _params.id_;
            params.log_params_ = std::move(_params.log_params_);
            params.is_binary_data_ = _params.is_binary_data_;
            params.handler_ = std::move(local_handler);
            ptr_this->download(std::move(params));
        }

        return 0;
    });
}

void core::wim::async_loader::download_image_metainfo(const std::string& _url, const wim_packet_params& _wim_params, link_meta_handler_t _handler, int64_t _id, std::string_view _log_str)
{
    const auto signed_url = sign_loader_uri(preview_proxy::uri::get_preview(), _wim_params, preview_proxy::format_get_preview_params(_url));

    async_loader_log_params params;
    params.log_str_ = _log_str;
    params.need_log_ = true;
    params.log_functor_.add_marker("a");
    params.log_functor_.add_marker("url");
    params.log_functor_.add_marker("aimsid", core::aimsid_range_evaluator());
    params.log_functor_.add_json_marker("url");
    params.log_functor_.add_json_marker("amp_url");
    params.log_functor_.add_json_marker("preview_url");
    params.log_functor_.add_json_marker("redirect_url");
    params.log_functor_.add_json_marker("title");
    params.log_functor_.add_json_marker("snippet");
    params.log_functor_.add_json_marker("value");
    params.log_functor_.add_json_marker("format");

    download_metainfo(_url, signed_url, preview_proxy::parse_json, _wim_params, _handler, _id, "filesDownloadSnippetMetadata", std::move(params));
}

void core::wim::async_loader::download_file_sharing_metainfo(const std::string& _url, const wim_packet_params& _wim_params, file_sharing_meta_handler_t _handler, int64_t _id)
{
    std::string id{ tools::get_file_id(_url) };
    const auto source_id = tools::get_source_id(_url);
    im_assert(!id.empty());

    auto signed_url = su::concat(urls::get_url(urls::url_type::files_info), id, "/?f=json&ptt_text=1&aimsid=", _wim_params.aimsid_);
    if (source_id)
        signed_url += su::concat("&source=", *source_id);

    if (features::is_webp_original_accepted())
        signed_url += "&support_webp=1";

    auto log_params = make_filesharing_log_params({ std::move(id), source_id ? std::make_optional<std::string>(*source_id) : std::nullopt });
    download_metainfo(_url, signed_url, &file_sharing_meta::parse_json, _wim_params, _handler, _id, "filesDownloadMetadata", std::move(log_params));
}

void core::wim::async_loader::download_image_preview(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params,
    link_meta_handler_t _metainfo_handler, file_info_handler_t _preview_handler, int64_t _id)
{
    __INFO("async_loader",
        "download_image_preview\n"
        "url      = <%1%>\n"
        "mhandler = <%2%>\n"
        "phandler = <%3%>\n", _url % _metainfo_handler.to_string() % _preview_handler.to_string());

    auto local_handler = link_meta_handler_t([_priority, _url, _wim_params, _metainfo_handler, _preview_handler, _id, wr_this = weak_from_this()](loader_errors _error, const link_meta_data_t& _data)
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

        const auto full_log = _wim_params.full_log_;

        if (_error == loader_errors::network_error)
        {
            log_error("download_image_preview", _url, core::wim::load_error_type::network_error, full_log);

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
            log_error("download_image_preview", _url, load_error_type::load_failure, full_log);
            fire_callback(_error, _data, _metainfo_handler.completion_callback_);

            fire_callback(_error, std::move(file_data), _preview_handler.completion_callback_);
            return;
        }

        fire_callback(_error, _data, _metainfo_handler.completion_callback_);

        const auto meta = *_data.additional_data_;

        if (!meta.has_preview_uri())
        {
            log_error("download_image_preview", _url, load_error_type::no_preview, full_log);
            fire_callback(loader_errors::no_link_preview, std::move(file_data), _preview_handler.completion_callback_);
            return;
        }

        auto preview_url = meta.get_preview_uri(0, 0);
        auto file_path = get_path_in_cache(ptr_this->content_cache_dir_, preview_url, path_type::link_preview);

        download_params_with_info_handler params(std::move(_wim_params));
        params.priority_ = _priority;
        params.url_ = std::move(preview_url);
        params.base_url_ = std::move(_url);
        params.file_name_ = std::move(file_path);
        params.handler_ = std::move(_preview_handler);
        params.id_ = _id;
        params.is_binary_data_ = true;
        ptr_this->download_file(std::move(params));

    }, _metainfo_handler.progress_callback_);

    download_image_metainfo(_url, _wim_params, local_handler, _id);
}

void core::wim::async_loader::download_image(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params, const bool _use_proxy, const bool _is_external_resource, file_info_handler_t _handler, int64_t _id)
{
    download_image(_priority, _url, std::string(), _wim_params, _use_proxy, _is_external_resource, _handler, _id);
}

void core::wim::async_loader::download_image(priority_t _priority, const std::string& _url, const std::string& _file_name, const wim_packet_params& _wim_params, const bool _use_proxy, const bool _is_external_resource, file_info_handler_t _handler, int64_t _id)
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

        if (_error == loader_errors::network_error)
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
        dl_url = sign_loader_uri(preview_proxy::uri::get_url_content(), _wim_params, preview_proxy::format_get_url_content_params(dl_url));
        endpoint = std::string_view("filesDownloadSnippet");
    }

    download_params_with_info_handler params(_wim_params);
    params.priority_ = _priority;
    params.url_ = std::move(dl_url);
    params.base_url_ = _url;
    params.file_name_ = tools::from_utf8(path);
    params.handler_ = std::move(local_handler);
    params.id_ = _id;
    params.normalized_url_ = endpoint;
    params.is_binary_data_ = true;
    download_file(std::move(params));
}

void core::wim::async_loader::download_file_sharing(
    priority_t _priority, const std::string& _contact, const std::string& _url, std::string _file_name, const wim_packet_params& _wim_params, int64_t _id, file_info_handler_t _handler)
{
    {
        std::scoped_lock lock(in_progress_mutex_);
        auto it = in_progress_.find(_url);
        if (it != in_progress_.end())
        {
            update_file_chunks(*it->second, _priority, std::move(_handler), _id);
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
                if (meta_info && meta_info->local_ && !meta_info->local_->local_path_.empty())
                {
                    const auto local = tools::from_utf8(meta_info->local_->local_path_);
                    if (tools::system::is_exist(local))
                    {
                        const auto last_modified = tools::system::get_file_lastmodified(local);
                        if (tools::system::get_file_size(local) == meta_info->info_.file_size_ && last_modified == meta_info->local_->last_modified_)
                        {
                            force_request_metainfo = meta_info->info_.dlink_.empty();
                            if (_file_name.empty())
                                _file_name = meta_info->local_->local_path_;
                        }
                    }
                }
            }
        }
    }


    file_save_thread_->run_async_function([force_request_metainfo, content_cache_dir = content_cache_dir_, _url]() mutable
    {
        if (force_request_metainfo)
            tools::system::delete_file(get_path_in_cache(content_cache_dir, _url, path_type::link_meta));
        return 0;
    })->on_result_ = [wr_this = weak_from_this(), _url, _wim_params, _priority, _contact, _file_name, _handler = std::move(_handler), _id](int32_t)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->download_file_sharing_metainfo(_url, _wim_params, file_sharing_meta_handler_t(
            [_priority, _contact, _url, _file_name, _wim_params, _handler = std::move(_handler), _id, wr_this](loader_errors _error, const file_sharing_meta_data_t& _data) mutable
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == loader_errors::come_back_later)
            {
                fire_callback(loader_errors::come_back_later, file_info_data_t(), _handler.completion_callback_);
                return;
            }

            if (_error == loader_errors::network_error)
            {
                ptr_this->suspended_tasks_.push([_priority, _contact, _url, _file_name, _handler = std::move(_handler), _id, wr_this](const wim_packet_params& wim_params) mutable
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->download_file_sharing(_priority, _contact, _url, _file_name, wim_params, _id, std::move(_handler));
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

            if (meta->local_ && !meta->local_->local_path_.empty() && !_file_name.empty())
            {
                auto local = tools::from_utf8(meta->local_->local_path_);

                if (tools::system::is_exist(local))
                {
                    const auto last_modified = tools::system::get_file_lastmodified(local);
                    if (tools::system::get_file_size(local) == meta->info_.file_size_ && last_modified == meta->local_->last_modified_)
                    {
                        ptr_this->file_save_thread_->run_async_function([local = std::move(local), _handler = std::move(_handler), _url = std::move(_url), _file_name = std::move(_file_name)]() mutable
                        {
                            const auto dest = tools::from_utf8(_file_name);
                            if (local != dest)
                            {
                                tools::system::copy_file(local, dest);
                                if (_file_name.empty())
                                    tools::system::delete_file(local);
                            }

                            file_info_data_t data(std::make_shared<core::wim::downloaded_file_info>(_url, dest));
                            fire_callback(loader_errors::success, std::move(data), _handler.completion_callback_);
                            return 0;
                        });
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
                    case file_sharing_base_content_type::lottie:
                        file_path = get_path_in_cache(ptr_this->content_cache_dir_, _url, path_type::file) + tools::from_utf8(boost::filesystem::extension(meta->info_.file_name_));
                        break;
                    default:
                        file_path = generate_unique_path(boost::filesystem::wpath(ptr_this->download_dir_) / tools::from_utf8(meta->info_.file_name_));
                        break;
                }
            }

            file_info_data_t data(std::make_shared<core::wim::downloaded_file_info>(_url, file_path));

            if (tools::system::is_exist(file_path))
            {
                auto is_same_file_params = [](const auto& _local_path, const auto& _meta)
                {
                    if (_meta->local_)
                    {
                        const auto last_modified = tools::system::get_file_lastmodified(_local_path);
                        return last_modified == _meta->local_->last_modified_ && tools::system::get_file_size(_local_path) == _meta->info_.file_size_;
                    }

                    return false;
                };
                if (is_same_file_params(file_path, meta))
                {
                    fire_callback(loader_errors::success, std::move(data), _handler.completion_callback_);
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
                    case file_sharing_base_content_type::lottie:
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

            auto file_chunks = std::make_shared<downloadable_file_chunks>(_priority, _contact, meta->info_.dlink_, file_path, meta->info_.file_size_);
            file_chunks->downloaded_ = tools::system::get_file_size(file_chunks->tmp_file_name_);
            ptr_this->file_save_thread_->run_async_function([wr_this, data = std::move(data), file_chunks = std::move(file_chunks), handler = std::move(_handler), url = std::move(_url), id = std::move(_id), wim_params = std::move(_wim_params), priority = std::move(_priority)]() mutable
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return 0;

                if (file_chunks->total_size_ == file_chunks->downloaded_)
                {
                    if (!tools::system::move_file(file_chunks->tmp_file_name_, file_chunks->file_name_))
                    {
                        fire_callback(loader_errors::move_file, std::move(data), handler.completion_callback_);
                        return 0;
                    }

                    quarantine::quarantine_file({ file_chunks->file_name_, url, referrer_url() });

                    fire_callback(loader_errors::success, std::move(data), handler.completion_callback_);
                    return 0;
                }
                else if (file_chunks->total_size_ < file_chunks->downloaded_)
                {
                    tools::system::delete_file(file_chunks->tmp_file_name_);
                    file_chunks->downloaded_ = 0;
                }

                {
                    std::scoped_lock lock(ptr_this->in_progress_mutex_);
                    auto it = ptr_this->in_progress_.find(url);
                    if (it != ptr_this->in_progress_.end())
                    {
                        update_file_chunks(*it->second, priority, std::move(handler), id);
                        return 0;
                    }
                    file_chunks->handlers_.push_back(std::move(handler));
                    file_chunks->active_seqs_.add(id);
                    ptr_this->in_progress_[url] = file_chunks;
                }

                ptr_this->download_file_sharing_impl(url, wim_params, std::move(file_chunks), id);
                return 0;
            });
        }));
    };
}

void core::wim::async_loader::cancel_file_sharing(std::string _url, std::optional<int64_t> _seq)
{
    if (!_seq)
        return;
    std::scoped_lock lock(in_progress_mutex_);
    if (in_progress_.empty())
    {
        cancel(std::move(_url), std::move(_seq));
        return;
    }
    if (auto it = in_progress_.find(_url); it != in_progress_.end())
        it->second->active_seqs_.erase(*_seq);
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

    std::scoped_lock lock(in_progress_mutex_);
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

std::wstring core::wim::async_loader::get_meta_path(std::string_view _url, const std::wstring& _path)
{
    return get_path_in_cache(content_cache_dir_, _url, path_type::link_meta);
}

void core::wim::async_loader::save_filesharing_local_path(const std::wstring& _meta_path, std::string_view _url, const std::wstring& _path)
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
                    meta_info->local_ = fs_meta_local{ tools::system::get_file_lastmodified(_path), tools::from_utf16(_path) };
                else
                    meta_info->local_ = std::nullopt;

                save_metainfo(*meta_info, _meta_path);
            }
        }
    }
}

void core::wim::async_loader::save_metainfo(const file_sharing_meta& _meta, const std::wstring& _path)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    _meta.serialize(doc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    const auto json_string = rapidjson_get_string_view(buffer);

    if (json_string.empty())
    {
        im_assert(false);
        return;
    }
    core::tools::binary_stream::save_2_file(json_string, _path);
}

void core::wim::async_loader::save_filesharing_local_path(std::string_view _url, const std::wstring& _path)
{
    return save_filesharing_local_path(get_meta_path(_url, _path), _url, _path);
}

std::string core::wim::async_loader::path_in_cache(const std::string& _url)
{
    return tools::from_utf16(get_path_in_cache(content_cache_dir_, _url, path_type::file));
}

core::wim::async_loader_log_params core::wim::async_loader::make_filesharing_log_params(const core::tools::filesharing_id& _fs_id)
{
    async_loader_log_params params;
    params.need_log_ = true;
    params.log_functor_.add_marker("aimsid", aimsid_range_evaluator());
    if (!_fs_id.file_id_.empty())
    {
        params.log_functor_.add_url_marker(_fs_id.file_id_, distance_range_evaluator(-std::ptrdiff_t(_fs_id.file_id_.size())));
        if (_fs_id.source_id_ && !_fs_id.source_id_->empty())
            params.log_functor_.add_marker("source");
    }

    return params;
}

void core::wim::async_loader::download_file_sharing_impl(std::string _url, wim_packet_params _wim_params, downloadable_file_chunks_ptr _file_chunks, int64_t _id, std::string_view _normalized_url)
{
    if (_file_chunks->active_seqs_.is_empty())
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
            std::scoped_lock lock(ptr_this->in_progress_mutex_);
            handler_list = _file_chunks->handlers_;
        }

        const auto downloaded = _file_chunks->downloaded_ + _transferred;
        for (auto& handler : handler_list)
        {
            if (handler.progress_callback_)
            {
                g_core->execute_core_context({ [_file_chunks, handler, downloaded]()
                    {
                        handler.progress_callback_(_file_chunks->total_size_, downloaded, int32_t(downloaded / (_file_chunks->total_size_ / 100.0)));
                    } });
            }
        }
    };

    auto wim_stop_handler = _wim_params.stop_handler_;

    auto stop = [_file_chunks, wim_stop_handler]() -> bool
    {
        if (wim_stop_handler && wim_stop_handler())
            return true;

        return _file_chunks->active_seqs_.is_empty();
    };

    auto request = std::make_shared<http_request_simple>(g_core->get_proxy_settings(), utils::get_user_agent(_wim_params.aimid_), _file_chunks->priority_, std::move(stop), std::move(progress));

    request->set_url(_file_chunks->url_);
    add_webp_if_needed(request, _file_chunks->url_);
    request->set_need_log(_wim_params.full_log_);
    request->set_keep_alive();

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
    request->set_use_curl_decompresion(true);

    request->get_async([_url, _wim_params, _file_chunks, request, wr_this, _id](curl_easy::completion_code _completion_code) mutable
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
                ptr_this->file_save_thread_->run_async_function([wr_this, file_chunks = std::move(_file_chunks), url = std::move(_url)]() mutable
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return 0;

                    if (!tools::system::move_file(file_chunks->tmp_file_name_, file_chunks->file_name_))
                    {
                        ptr_this->fire_chunks_callback(loader_errors::move_file, url);
                        return 0;
                    }

                    quarantine::quarantine_file({ file_chunks->file_name_, url, referrer_url() });
                    ptr_this->fire_chunks_callback(loader_errors::success, url);
                    return 0;
                });
                return;
            }
            else if (_file_chunks->total_size_ < _file_chunks->downloaded_)
            {
                ptr_this->fire_chunks_callback(loader_errors::internal_logic_error, _url);
                return;
            }

            ptr_this->download_file_sharing_impl(_url, _wim_params, _file_chunks, _id);
        }
        else
        {
            if (const auto http_error = code >= 400 && code < 500; http_error || _file_chunks->active_seqs_.is_empty())
            {
                request->get_response()->close();
                tools::system::delete_file(_file_chunks->tmp_file_name_);
                ptr_this->fire_chunks_callback(http_error ? loader_errors::http_client_error : loader_errors::cancelled, _url);
                return;
            }

            ptr_this->suspended_tasks_.push([_url, _file_chunks, wr_this, _id](const wim_packet_params& wim_params)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->download_file_sharing_impl(_url, wim_params, _file_chunks, _id);
            });

            if (_completion_code == curl_easy::completion_code::resolve_failed)
                config::hosts::switch_to_ip_mode(_url, (int)_completion_code);
            else if (_completion_code == curl_easy::completion_code::failed)
                config::hosts::switch_to_dns_mode(_url, (int)_completion_code);
        }
    });
}

void core::wim::async_loader::update_file_chunks(downloadable_file_chunks& _file_chunks, priority_t _new_priority, file_info_handler_t _additional_handlers, int64_t _id)
{
    _file_chunks.handlers_.push_back(std::move(_additional_handlers));
    _file_chunks.active_seqs_.add(_id);
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
        std::scoped_lock lock(in_progress_mutex_);
        auto it = in_progress_.find(_url);
        im_assert(it != in_progress_.end());
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
                g_core->execute_core_context({ [=]()
                {
                    handler.completion_callback_(_error, data);
                } });
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
                    const auto write_time = std::chrono::system_clock::from_time_t(tools::system::get_file_lastmodified(dir_iter->path().wstring()));
                    if ((current_time - write_time) > delete_files_older)
                        files_to_delete.push_back(*dir_iter);
                }
            }
        }

        for (const auto& _file_path : files_to_delete)
        {
            boost::filesystem::remove(_file_path, error);

            bs.write<std::string_view>("Delete file: ");
            bs.write<std::string_view>(_file_path.string());
            bs.write<std::string_view>("\r\n");
        }
    }
    catch (const std::exception&)
    {

    }

    bs.write<std::string_view>("Finish cleanup\r\n");
    g_core->write_data_to_network_log(std::move(bs));
}

void core::wim::log_error(std::string_view _context, std::string_view _url, core::wim::load_error_type _type, bool _full_log)
{
    tools::binary_stream bs;
    bs.write<std::string_view>(_url);

    if (!_full_log)
    {
        log_replace_functor f = make_default_log_replace_functor(_url);
        f.add_message_markers();
        f(bs);
    }

    bs.write<std::string_view>(su::concat(_context, ' ', get_error(_type), '\n'));
    if (_full_log)
        bs.write<std::string_view>(su::concat("url: ", (_url.empty() ? "INVALID OR EMPTY URL" : _url) , '\n'));
    else if (_url.empty())
        bs.write<std::string_view>(su::concat("url: ", "INVALID OR EMPTY URL\n"));

    g_core->write_data_to_network_log(std::move(bs));
}
