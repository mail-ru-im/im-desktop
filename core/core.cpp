#include "stdafx.h"
#include "core.h"
#include "async_task.h"
#include "main_thread.h"
#include "network_log.h"
#ifndef STRIP_ZSTD
#include "zstd_helper.h"
#endif // !STRIP_ZSTD
#include "configuration/app_config.h"
#include "configuration/host_config.h"
#include "../common.shared/config/config.h"
#include "../common.shared/omicron_keys.h"

#include "connections/im_container.h"
#include "connections/base_im.h"
#include "connections/login_info.h"
#include "connections/im_login.h"
#include "connections/send_feedback.h"

#ifndef STRIP_VOIP
#include "Voip/VoipManager.h"
#include "voip/voip3.h"
#endif

#include "../corelib/collection_helper.h"
#include "core_settings.h"
#include "curl_handler.h"
#include "gui_settings.h"
#include "themes/theme_settings.h"
#include "themes/wallpaper_loader.h"
#include "utils.h"
#include "http_request.h"
#include "scheduler.h"
#include "archive/local_history.h"
#include "log/log.h"
#include "profiling/profiler.h"
#include "updater/updater.h"
#include "crash_sender.h"
#include "statistics.h"
#include "stats/im/im_stats.h"
#include "tools/md5.h"
#include "tools/hmac_sha_base64.h"
#include "../corelib/enumerations.h"
#include "tools/system.h"
#include "proxy_settings.h"
#include "tools/strings.h"
#include "stats/memory/memory_stats_collector.h"
#include "connections/wim/memory_usage/gui_memory_consumption_reporter.h"
#include "post_install_action.h"

#include "../libomicron/include/omicron/omicron.h"
#include "connections/wim/auth_parameters.h"
#include "../../../common.shared/version_info.h"
#include "connections/urls_cache.h"
#include "connections/wim/wim_packet.h"

#include "network/network_change_notifier.h"
#include "network/network_change_observer.h"


#ifdef _WIN32
    #include "../common.shared/win32/crash_handler.h"
#endif

#if HAVE_SECURE_GETENV && __linux__
// different glibc versions have different secure_getenv naming.
// __secure_getenv is used in precompiled libxcb.
extern "C" {
#if __i386__
__asm__(".symver secure_getenv,__secure_getenv@GLIBC_2.0");
#else
__asm__(".symver secure_getenv,__secure_getenv@GLIBC_2.2.5");
#endif
extern char *__secure_getenv(const char *__name)
{
    return secure_getenv(__name);
}
}
#endif

namespace
{
    std::vector<std::string> get_user_wp_folders_list()
    {
        std::vector<std::string> folders;

        try
        {
            boost::system::error_code e;

            namespace fs = boost::filesystem;
            fs::path themes_root(core::utils::get_themes_path());
            if (fs::exists(themes_root, e))
            {
                fs::directory_iterator end;

                for (fs::directory_iterator dir_it(themes_root, e); dir_it != end && !e; dir_it.increment(e))
                {
                    fs::path path = (*dir_it);

                    if (const auto dir_name = path.leaf().wstring(); dir_name.find(L"u") == 0)
                    {
                        auto str = core::tools::from_utf16(dir_name);
                        const auto id = std::string_view(str.data() + 1, str.size() - 1);
                        if (core::tools::is_uin(id) || core::tools::is_email(id) || core::tools::is_phone(id))
                            folders.push_back(std::move(str));
                    }
                }
            }
        }
        catch (const std::exception&)
        {

        }

        return folders;
    }

    void write_additional_coregui_message_data(core::tools::binary_stream& bs, core::icollection* _message_data)
    {
        if (!_message_data)
            return;

        if (_message_data->is_value_exist("error"))
        {
            if (const auto value = _message_data->get_value("error"))
            {
                if (const auto intval = value->get_as_int())
                {
                    bs.write<std::string_view>(" [");
                    bs.write<std::string_view>(std::to_string(intval));
                    bs.write<std::string_view>("]");
                }
            }
        }

        const auto write_contact = [_message_data, &bs](std::string_view _name)
        {
            if (_message_data->is_value_exist(_name))
            {
                if (const auto value = _message_data->get_value(_name))
                {
                    bs.write<std::string_view>(" <");
                    bs.write<std::string_view>(value->get_as_string());
                    bs.write<std::string_view>(">");
                    return true;
                }
            }
            return false;
        };

        constexpr std::string_view fields[] = { "contact", "aimid", "aimId", "sn", "stamp" };
        for (std::string_view c : fields)
            if (write_contact(c))
                break;

        if (_message_data->is_value_exist("contacts"))
        {
            std::stringstream s;
            if (const auto value = _message_data->get_value("contacts"))
            {
                if (const auto contacts = value->get_as_array())
                {
                    if (const auto size = contacts->size())
                    {
                        for (core::iarray::size_type i = 0; i < size; ++i)
                            s << contacts->get_at(i)->get_as_string() << "; ";
                    }
                }
            }
            if (const auto str = s.str(); !str.empty())
            {
                bs.write<std::string_view>("\r\nfor: ");
                bs.write<std::string_view>(str);
            }
        }
    }

    constexpr auto contains_history_messages(std::string_view message_string)
    {
        return message_string == "archive/messages/get/result"
               || message_string == "archive/buddies/get/result"
               || message_string == "messages/received/dlg_state"
               || message_string == "messages/received/init"
               || message_string == "messages/received/message_status";
    };

    constexpr auto is_loading_progress(std::string_view _message_string)
    {
        return _message_string == "image/download/progress"
            || _message_string == "files/download/progress";
    };

    constexpr auto is_loading_result(std::string_view _message_string)
    {
        return _message_string == "image/download/result"
            || _message_string == "files/download/result"
            || _message_string == "files/preview_size/result";
    };

    std::vector<std::string> to_string_vector(core::coll_helper& _params, std::string_view _name)
    {
        std::vector<std::string> res;

        if (_params.is_value_exist(_name))
        {
            if (auto arr = _params.get_value_as_array(_name))
            {
                const auto size = arr->size();
                res.reserve(size);
                for (std::remove_const_t<decltype(size)> i = 0; i < size; ++i)
                    res.emplace_back(arr->get_at(i)->get_as_string());
            }
        }
        return res;
    }
}

using namespace core;

std::unique_ptr<core::core_dispatcher>    core::g_core;

constexpr std::string_view message_log("_log");
constexpr std::string_view message_network_log("_network_log");

static std::string_view get_omicron_dev_id()
{
    constexpr auto key = platform::is_apple()
        ? config::values::omicron_dev_id_mac : (platform::is_windows() ? config::values::omicron_dev_id_win : config::values::omicron_dev_id_linux);
    return config::get().string(key);
}

static constexpr auto get_omicron_app_environment() noexcept
{
    return environment::is_alpha() ? omicronlib::environment_type::alpha
            : (environment::is_beta() ? omicronlib::environment_type::beta
                : (environment::is_release() ? omicronlib::environment_type::release
                    : omicronlib::environment_type::develop));
}

static constexpr std::string_view get_omicron_arch() noexcept
{
#if defined(__x86_64__)
    return "x64";
#else
    if constexpr (platform::is_apple())
    {
#if defined(__arm64)
        return "arm64";
#else
        return "unkn";
#endif // defined(__arm64)
    }
    return "x32";
#endif // defined(__x86_64__)
}

core_dispatcher::core_dispatcher()
    : core_thread_(nullptr)
    , gui_connector_(nullptr)
    , core_factory_(nullptr)
    , delayed_stat_timer_id_(0)
    , omicron_update_timer_id_(0)
{
#ifdef _WIN32
    std::locale loc = boost::locale::generator().generate(std::string());
    std::locale::global(loc);
#endif

    cl_search_count_.store(0);

    profiler::enable(::build::is_debug());
}



core_dispatcher::~core_dispatcher()
{
    profiler::flush_logs();

    __LOG(log::shutdown();)

    http_request_simple::shutdown_global();
}

std::string core::core_dispatcher::get_uniq_device_id() const
{
    return settings_->get_value<std::string>(core_settings_values::csv_device_id, std::string());
}

void core::core_dispatcher::execute_core_context(std::function<void()> _func)
{
    if (!core_thread_)
    {
        assert(!"core thread empty");
        return;
    }

    core_thread_->execute_core_context(std::move(_func));
}

void core::core_dispatcher::write_data_to_network_log(tools::binary_stream _data)
{
    if (is_network_log_valid())
        get_network_log().write_data(std::move(_data));
}

void core::core_dispatcher::write_string_to_network_log(std::string_view _text)
{
    if (is_network_log_valid())
        get_network_log().write_string(_text);
}

std::stack<std::wstring> core::core_dispatcher::network_log_file_names_history_copy()
{
    std::stack<std::wstring> result;
    if (is_network_log_valid())
        result = get_network_log().file_names_history_copy();

    return result;
}

std::shared_ptr<base_im> core::core_dispatcher::find_im_by_id(unsigned id) const
{
    assert(!!im_container_);
    if (!!im_container_) {
        return im_container_->get_im_by_id(id);
    }
    return nullptr;
}

uint32_t core::core_dispatcher::add_timer(std::function<void()> _func, std::chrono::milliseconds _timeout)
{
    if (!scheduler_)
    {
        assert(false);
        return 0;
    }

    return scheduler_->push_timer(std::move(_func), _timeout);
}

void core::core_dispatcher::stop_timer(uint32_t _id)
{
    if (scheduler_)
        scheduler_->stop_timer(_id);
}

std::shared_ptr<async_task_handlers> core::core_dispatcher::run_async(std::function<int32_t()> task)
{
    assert(!!async_executer_);
    if (!async_executer_)
        return std::make_shared<async_task_handlers>();
    return async_executer_->run_async_function(std::move(task));
}


void core::core_dispatcher::link_gui(icore_interface* _core_face, const common::core_gui_settings& _settings)
{
    // called from main thread

    gui_connector_ = _core_face->get_gui_connector();
    core_factory_ = _core_face->get_factory();
    core_thread_ = new main_thread();

    execute_core_context([this, _settings]
    {
        start(_settings);
    });
}

void core::core_dispatcher::start(const common::core_gui_settings& _settings)
{
    __LOG(log::init(utils::get_logs_path(), false);)

#ifndef STRIP_ZSTD
    zstd_helper_ = std::make_shared<zstd_helper>(utils::get_product_data_path() + L"/zdicts");
#endif // !STRIP_ZSTD

    http_request_simple::init_global();

    curl_handler::instance().init();

    core_gui_settings_ = _settings;

    const auto app_ini_path = utils::get_app_ini_path();
    configuration::load_app_config(app_ini_path);

    // called from core thread
    network_log_ = std::make_unique<network_log>(utils::get_logs_path());

    boost::system::error_code error_code;
    const auto product_data_root = boost::filesystem::weakly_canonical(utils::get_product_data_path(), Out error_code);

    settings_ = std::make_shared<core::core_settings>(product_data_root / L"settings" /  tools::from_utf8(core_settings_file_name)
        , utils::get_product_data_path() + L"/settings/" + tools::from_utf8(core_settings_export_file_name));
    settings_->load();
    if (settings_->get_locale().empty())
        set_locale(core_gui_settings_.locale_);

    if (settings_->end_init_default())
        settings_->save();

    proxy_settings_manager_ = std::make_unique<proxy_settings_manager>(*settings_);

    async_executer_ = std::make_unique<async_executer>("core_async_executer");
    scheduler_ = std::make_unique<scheduler>();

#ifndef STRIP_VOIP
    voip_manager_.reset(voip_manager::createVoipManager(*this));
    assert(!!voip_manager_);
#endif //__STRIP_VOIP

    memory_stats_collector_ = std::make_unique<memory_stats::memory_stats_collector>();

#ifndef STRIP_VOIP
    memory_stats::memory_stats_collector::reporter_ptr voip_memory_reporter
            = std::make_unique<memory_stats::voip_memory_consumption_reporter>(voip_manager_);

    memory_stats_collector_->register_consumption_reporter(std::move(voip_memory_reporter));
#endif

    im_container_ = std::make_shared<im_container>(voip_manager_,
                                                   memory_stats_collector_);

    post_app_config();

    im_container_->create();
    if (config::get().is_on(config::features::omicron_enabled))
        start_omicron_service();

    config::hosts::load_dns_cache();

    if (config::get().is_debug())
        get_network_log().write_string("debug config is used\n");

    load_im_stats();
#ifndef STRIP_ZSTD
    zstd_helper_->start_auto_updater(im_container_);
#endif // !STRIP_ZSTD

#ifdef _WIN32
    core::dump::set_os_version(core::tools::system::get_os_version_string());
#endif

#if defined _WIN32 || defined __linux__ || BUILD_FOR_STORE
    report_sender_ = std::make_shared<dump::report_sender>(g_core->get_login_after_start(), core::configuration::get_app_config().is_full_log_enabled());
    report_sender_->send_report();
#endif

    load_gui_settings();

    load_theme_settings();
    post_theme_settings_and_meta();

    post_gui_settings();
    post_user_proxy_to_gui();

    post_logins();

    updater_ = std::make_unique<update::updater>();

    load_statistics();
    setup_memory_stats_collector();

    post_install_action();

    if (utils::get_logs_path() != utils::get_obsolete_logs_path())
    {
        run_async([]() {
            tools::system::delete_directory(utils::get_obsolete_logs_path());
            return 0;
        });
    }
}


void core::core_dispatcher::unlink_gui()
{
    execute_core_context([this]
    {
        // NOTE : this order is important!
        voip_manager_.reset();
        im_container_.reset();


        if (omicron_update_timer_id_ > 0)
            stop_timer(omicron_update_timer_id_);
        omicronlib::omicron_cleanup();

        const auto keep_local_data = is_keep_local_data();
        if (!keep_local_data)
            gui_settings_->clear_personal_values();

        gui_settings_.reset();
        scheduler_.reset();
        if (is_stats_enabled())
            statistics_.reset();
        if (is_im_stats_enabled())
            im_stats_.reset();
        async_executer_.reset();
        updater_.reset();
        report_sender_.reset();
        network_log_.reset();
        proxy_settings_manager_.reset();
        theme_settings_.reset();
#ifndef STRIP_ZSTD
        zstd_helper_.reset();
#endif // !STRIP_ZSTD

        if (!keep_local_data)
            remove_local_data(true);

        curl_handler::instance().cleanup();
    });

    delete core_thread_;
    core_thread_ = nullptr;

    gui_connector_->release();
    gui_connector_ = nullptr;

    core_factory_->release();
    core_factory_ = nullptr;
}


void core::core_dispatcher::post_message_to_gui(std::string_view _message, int64_t _seq, icollection* _message_data)
{
    tools::binary_stream bs;
    bs.write<std::string_view>("CORE->GUI: message=");
    bs.write<std::string_view>(_message);
    write_additional_coregui_message_data(bs, _message_data);

    const bool is_pending = "archive/messages/pending" == _message;

    if(_message_data && !is_loading_progress(_message))
        bs.write<std::string_view>("\r\n");

    if(_message_data && (is_loading_progress(_message) || is_loading_result(_message)))
    {
        const auto write_progress = [_message_data, &bs](std::string_view _field_total, std::string_view _field_transferred)
        {
            if (_message_data->is_value_exist(_field_total) && _message_data->is_value_exist(_field_transferred))
            {
                const auto value_total = _message_data->get_value(_field_total)->get_as_int64();
                std::stringstream transferred;
                if (value_total > 0)
                {
                    const auto value_transferred = _message_data->get_value(_field_transferred)->get_as_int64();
                    transferred << ' ' << 100 * value_transferred / value_total << '%';
                    transferred << " (" << std::to_string(value_transferred) << "/" << std::to_string(value_total) << ")\n";
                }
                else
                {
                    transferred << " Incorrect file size " << value_total << '\n';
                }
                bs.write<std::string_view>(transferred.str());
            }
        };

        const auto write_url = [_message_data, &bs](std::string_view _header, std::string_view _field)
        {
            if (_message_data->is_value_exist(_field))
            {
                bs.write<std::string_view>(_header);
                bs.write<std::string_view>(_message_data->get_value(_field)->get_as_string());
                bs.write<std::string_view>("\n");
            }
        };

        write_progress("bytes_total", "bytes_transferred");
        write_progress("file_size", "bytes_transfer");
        const auto full_log = core::configuration::get_app_config().is_full_log_enabled();
        if (full_log)
        {
            write_url("URL: ", "url");
            write_url("File URL: ", "file_url");
            write_url("Preview URL: ", "full_preview_uri");
        }
        write_url("Path: ", "local");
        write_url("Path: ", "local_path");
        write_url("Path: ", "file_name");
    }
    else if (_message_data && _message == "voip_signal") // Added type of voip call to log.
    {
        auto value = _message_data->get_value("sig_type");
        if (value)
        {
            std::stringstream s;
            s << "type: ";
            s << value->get_as_string();
            if (_message_data->is_value_exist("contact"))
            {
                auto contact = _message_data->get_value("contact");
                s << "; contact: ";
                s << contact->get_as_string();
            }
            if (_message_data->is_value_exist("param"))
            {
                auto param = _message_data->get_value("param");
                s << "; param: ";
                s << param->get_as_bool();
            }

            bs.write<std::string_view>(s.str());
            bs.write<std::string_view>("\r\n");
        }
    }
    else if (_message_data && _message == "messages/received/server")
    {
        std::stringstream s;
        if (const auto value = _message_data->get_value("ids"))
        {
            if (const auto ids = value->get_as_array())
            {
                if (const auto size = ids->size())
                {
                    s << "ids size: " << size << "; ";
                    s << "first id: " << ids->get_at(0)->get_as_int64() << "; last id: " << ids->get_at(size - 1)->get_as_int64() << ';';
                }
            }
        }
        bs.write<std::string_view>(s.str());
        bs.write<std::string_view>("\r\n");
    }
    else if (_message_data && (is_pending || contains_history_messages(_message)))
    {
        std::stringstream s;
        if (_message_data->is_value_exist("messages"))
        {
            if (const auto value = _message_data->get_value("messages"))
            {
                if (const auto messages = value->get_as_array())
                {
                    if (const auto size = messages->size())
                    {
                        s << "messages' size: " << size << "\r\nids: ";
                        for (iarray::size_type i = 0; i < size; ++i)
                        {
                            const auto data = messages->get_at(i)->get_as_collection();
                            s << data->get_value("id")->get_as_int64();
                            if (is_pending)
                                s << " -> " << data->get_value("internal_id")->get_as_string();
                            s << "; ";
                        }
                    }
                }
            }
        }
        if (_message_data->is_value_exist("intro_messages"))
        {
            if (const auto value = _message_data->get_value("intro_messages"))
            {
                if (const auto messages = value->get_as_array())
                {
                    if (const auto size = messages->size())
                    {
                        s << "\r\nintro messages' size: " << size << "\r\nids: ";
                        for (iarray::size_type i = 0; i < size; ++i)
                            s << messages->get_at(i)->get_as_collection()->get_value("id")->get_as_int64() << "; ";
                    }
                }
            }
        }
        bs.write<std::string_view>(s.str());
        bs.write<std::string_view>("\r\n");
    }
    else if (_message_data && _message == "dlg_states")
    {
        const auto states = _message_data->get_value("dlg_states")->get_as_array();
        bs.write<std::string_view>("count: ");
        const auto size = states->size();
        bs.write<std::string_view>(std::to_string(size));
        bs.write<std::string_view>(";\r\n");
        bs.write<std::string_view>("unreads: ");

        for (iarray::size_type i = 0; i < size; ++i)
        {
            const auto s = states->get_at(i)->get_as_collection();
            const auto unreads = s->get_value("unreads")->get_as_int64();
            bs.write<std::string_view>(s->get_value("contact")->get_as_string());
            bs.write<std::string_view>(": ");
            bs.write<std::string_view>(std::to_string(unreads));
            bs.write<std::string_view>(";");
        }

        bs.write<std::string_view>("\r\n");
    }
    else if (_message_data && _message == "connection/state")
    {
        bs.write<std::string_view>(_message_data->get_value("state")->get_as_string());
        bs.write<std::string_view>(";\r\n");
    }
    else if (_message_data && _message == "contacts/search/local/result")
    {
        const auto results = _message_data->get_value("results")->get_as_array();
        bs.write<std::string_view>("count: ");
        const auto size = results->size();
        bs.write<std::string_view>(std::to_string(size));
        bs.write<std::string_view>(";\r\n");

        for (iarray::size_type i = 0; i < size; ++i)
        {
            const auto r = results->get_at(i)->get_as_collection();
            bs.write<std::string_view>(r->get_value("contact")->get_as_string());
            bs.write<std::string_view>("; ");
        }

        if (size > 0)
            bs.write<std::string_view>("\r\n");
    }
    else if (_message_data && _message == "core/logins")
    {
        const auto login = _message_data->get_value("login")->get_as_string();
        const auto has_valid_login = _message_data->get_value("has_valid_login")->get_as_bool();
        const auto locked = _message_data->get_value("locked")->get_as_bool();
        bs.write<std::string_view>("login: ");
        bs.write<std::string_view>(login);
        bs.write<std::string_view>(";\r\n");
        bs.write<std::string_view>("has_valid_login: ");
        bs.write<std::string_view>(logutils::yn(has_valid_login));
        bs.write<std::string_view>(";\r\n");
        bs.write<std::string_view>("locked: ");
        bs.write<std::string_view>(logutils::yn(locked));
        bs.write<std::string_view>(";\r\n");
    }
    else if (_message_data && _message == "need_login")
    {
        if (_message_data->is_value_exist("is_auth_error"))
        {
            const auto is_auth_error = _message_data->get_value("is_auth_error")->get_as_bool();
            bs.write<std::string_view>("is_auth_error: ");
            bs.write<std::string_view>(logutils::yn(is_auth_error));
            bs.write<std::string_view>(";\r\n");
        }

    }

//     if (_message_data)
//     {
//         bs.write<std::string>(_message_data->log());
//     }

    write_data_to_network_log(std::move(bs));

    //__LOG(core::log::info("core", boost::format("post message to gui, message=%1%\nparameters: %2%") % _message % (_message_data ? _message_data->log() : ""));)

    if (!gui_connector_)
    {
        assert(!"gui unlinked");
        return;
    }

    gui_connector_->receive(_message, _seq, _message_data);
}


icollection* core::core_dispatcher::create_collection()
{
    if (!core_factory_)
    {
        assert(!"core factory empty");
        return nullptr;
    }

    return core_factory_->create_collection();
}


void core_dispatcher::load_gui_settings()
{
    gui_settings_ = std::make_shared<core::gui_settings>(
        utils::get_product_data_path() + L"/settings/" + tools::from_utf8(gui_settings_file_name),
        utils::get_product_data_path() + L"/settings/" + tools::from_utf8(settings_export_file_name));

    auto result = gui_settings_->load();

    auto stream_val = tools::binary_stream();
    stream_val.write(!result);

    gui_settings_->set_value(first_run, std::move(stream_val));
    gui_settings_->start_save();
}

bool core_dispatcher::is_stats_enabled() const
{
    return config::get().is_on(config::features::statistics);
}

void core_dispatcher::load_statistics()
{
    if (!is_stats_enabled())
        return;

    statistics_ = std::make_shared<core::stats::statistics>(
        utils::get_product_data_path() + L"/stats/stats.stg", utils::get_product_data_path() + L"/stats/stats_mt.stg");

    execute_core_context([this]
    {
        statistics_->init();
        start_session_stats(false /* delayed */);

        constexpr auto timeout = (build::is_debug() ? std::chrono::seconds(10) : std::chrono::minutes(5));
        delayed_stat_timer_id_ = add_timer([this]
        {
            if (statistics_)
            {
                start_session_stats(true /* delayed */);
            }

            stop_timer(delayed_stat_timer_id_);
        }, timeout);
    });
}

bool core_dispatcher::is_im_stats_enabled() const
{
    return config::get().is_on(config::features::statistics);
}

void core_dispatcher::load_im_stats()
{
    if (!is_im_stats_enabled())
        return;

    im_stats_ = std::make_shared<core::stats::im_stats>(utils::get_product_data_path() + L"/stats/im_stats.stg");
    im_stats_->init();
}

void core_dispatcher::start_session_stats(bool _delayed)
{
    core::stats::event_props_type props;

    std::string uin;
    auto im = find_im_by_id(1);
    if (im)
    {
        uin = g_core->get_root_login();
    }

    auto user_key = uin + g_core->get_uniq_device_id();
    auto hashed_user_key = core::tools::md5(user_key.c_str(), (int32_t)user_key.length());
    props.emplace_back(std::make_pair("hashed_user_key", hashed_user_key));
    props.emplace_back(std::make_pair("Sys_RAM", std::to_string(core::tools::system::get_memory_size_mb())));

    std::locale loc("");
    props.emplace_back(std::make_pair("Sys_Language", loc.name()));
    props.emplace_back(std::make_pair("Sys_OS_Version", core::tools::system::get_os_version_string()));

    if (voip_manager_)
        props.emplace_back(std::make_pair("Sys_Video_Capture", std::to_string(voip_manager_->get_devices_number(DeviceType::VideoCapturing))));

    insert_event(_delayed ? core::stats::stats_event_names::start_session_params_loaded
        : core::stats::stats_event_names::start_session, std::move(props));
}

void core_dispatcher::insert_event(core::stats::stats_event_names _event)
{
    insert_event(_event, {});
}

void core_dispatcher::insert_event(core::stats::stats_event_names _event, core::stats::event_props_type&& _props)
{
    if (!is_stats_enabled())
        return;

    g_core->execute_core_context([wr_stats = std::weak_ptr<core::stats::statistics>(statistics_), _event, _props = std::move(_props)]() mutable
    {
        auto ptr_stats = wr_stats.lock();
        if (!ptr_stats)
            return;

        ptr_stats->insert_event(_event, std::move(_props));
    });
}

void core_dispatcher::insert_im_stats_event(core::stats::im_stat_event_names _event)
{
    insert_im_stats_event(_event, core::stats::event_props_type());
}

void core_dispatcher::insert_im_stats_event(core::stats::im_stat_event_names _event, core::stats::event_props_type&& _props)
{
    if (!is_im_stats_enabled())
        return;

    g_core->execute_core_context([wr_stats = std::weak_ptr<core::stats::im_stats>(im_stats_), _event, _props = std::move(_props)]() mutable
    {
        auto ptr_stats = wr_stats.lock();
        if (!ptr_stats)
            return;

        ptr_stats->insert_event(_event, std::move(_props));
    });
}

void core_dispatcher::load_theme_settings()
{
    theme_settings_ = std::make_shared<core::theme_settings>(su::wconcat(utils::get_product_data_path(), L"/settings/", tools::from_utf8(theme_settings_file_name)));
    theme_settings_->load();
    theme_settings_->clear_value("image"); // clear legacy value
    theme_settings_->start_save();
}

void core_dispatcher::post_gui_settings()
{
    coll_helper cl_coll(create_collection(), true);
    gui_settings_->serialize(cl_coll);

    cl_coll.set_value_as_string("data_path", core::tools::from_utf16(core::utils::get_product_data_path()));
    cl_coll.set_value_as_string("os_version", core::tools::system::get_os_version_string());

    post_message_to_gui("gui_settings", 0, cl_coll.get());
}

void core_dispatcher::post_logins()
{
    coll_helper cl_coll(create_collection(), true);
    gui_settings_->serialize(cl_coll);

    const auto has_valid_login = im_container_->has_valid_login();
    cl_coll.set_value_as_bool("has_valid_login", has_valid_login);
    if (has_valid_login)
        cl_coll.set_value_as_string("login", im_container_->get_login());
    else
        cl_coll.set_value_as_string("login", im_container_->get_first_login());
    cl_coll.set_value_as_bool("locked", get_local_pin_enabled());
    cl_coll.set_value_as_bool("install_beta_updates", get_install_beta_updates());

    post_message_to_gui("core/logins", 0, cl_coll.get());
}

void core_dispatcher::post_theme_settings_and_meta()
{
    coll_helper cl_coll(create_collection(), true);
    theme_settings_->serialize(cl_coll);

    core::tools::binary_stream bs;
    if (bs.load_from_file(utils::get_themes_meta_path()))
    {
        bs.write(0);

        ifptr<istream> meta_data(cl_coll->create_stream(), true);
        if (const auto data_size = bs.available(); data_size > 0)
        {
            meta_data->write((const uint8_t*)bs.read(data_size), data_size);

            cl_coll.set_value_as_stream("meta", meta_data.get());
        }
    }
    else if (!load_themes_etag().empty())
    {
        save_themes_etag(std::string());
    }

    auto user_folders = get_user_wp_folders_list();
    ifptr<iarray> folders_array(cl_coll->create_array(), true);
    folders_array->reserve(user_folders.size());
    for (const auto& folder : user_folders)
    {
        ifptr<ivalue> iv(cl_coll->create_value());
        iv->set_as_string(folder.c_str(), (int32_t)folder.length());
        folders_array->push_back(iv.get());
    }
    cl_coll.set_value_as_array("user_folders", folders_array.get());

    post_message_to_gui("themes/settings", 0, cl_coll.get());
}

void core_dispatcher::post_app_config()
{
    coll_helper cl_coll(create_collection(), true);

    configuration::get_app_config().serialize(Out cl_coll);

    post_message_to_gui("app_config", 0, cl_coll.get());
}

void core::core_dispatcher::on_message_update_gui_settings_value(int64_t _seq, coll_helper _params)
{
    std::string value_name = _params.get_value_as_string("name");
    istream* value_data = _params.get_value_as_stream("value");

    tools::binary_stream bs_data;
    auto size = value_data->size();
    if (size)
        bs_data.write((const char*) value_data->read(size), size);

    gui_settings_->set_value(value_name, std::move(bs_data));
}

void core::core_dispatcher::on_message_update_theme_settings_value(int64_t _seq, coll_helper _params)
{
    std::string value_name = _params.get_value_as_string("name");
    istream* value_data = _params.get_value_as_stream("value");

    tools::binary_stream bs_data;
    auto size = value_data->size();
    if (size)
        bs_data.write((const char*) value_data->read(size), size);

    theme_settings_->set_value(value_name, std::move(bs_data));
    theme_settings_->save_if_needed();
}

void core::core_dispatcher::on_message_set_default_theme_id(int64_t _seq, coll_helper _params)
{
    const std::string_view theme_id = _params.get_value_as_string("id");
    assert(!theme_id.empty());

    theme_settings_->set_default_theme(theme_id);
    theme_settings_->save_if_needed();
}

void core::core_dispatcher::on_message_set_default_wallpaper_id(int64_t _seq, coll_helper _params)
{
    if (std::string_view wp_id = _params.get_value_as_string(get_global_wp_id_setting_field(), ""); !wp_id.empty())
        theme_settings_->set_default_wallpaper(wp_id);
    else
        theme_settings_->reset_default_wallpaper();

    theme_settings_->clear_value("id"); // clear legacy
    theme_settings_->save_if_needed();
}

void core::core_dispatcher::on_message_set_wallpaper_urls(int64_t _seq, coll_helper _params)
{
    const auto wallpapers = _params.get_value_as_array("wallpapers");

    themes::wallpaper_info_v wallpaper_urls;
    wallpaper_urls.reserve(wallpapers->size());
    for (auto i = 0; i < wallpapers->size(); ++i)
    {
        core::coll_helper wall(wallpapers->get_at(i)->get_as_collection(), false);

        themes::wallpaper_info info;
        info.id_ = wall.get_value_as_string("id");
        info.full_image_url_ = wall.get_value_as_string("image_url");
        info.preview_url_ = wall.get_value_as_string("preview_url");

        wallpaper_urls.emplace_back(std::move(info));
    }

    theme_settings_->set_wallpaper_urls(wallpaper_urls);
}

void core::core_dispatcher::on_feedback(int64_t _seq, coll_helper& _params)
{
    std::map<std::string, std::string> fields;
    fields.insert({"fb.screen_resolution", _params.get_value_as_string("screen_resolution")});
    fields.insert({"fb.referrer", _params.get_value_as_string("referrer")});
    fields.insert({std::string(config::get().string(config::values::feedback_version_id)), _params.get_value_as_string("version")});
    fields.insert({"fb.question.159", _params.get_value_as_string("os_version")});
    fields.insert({"fb.question.178", _params.get_value_as_string("build_type")});
    fields.insert({std::string(config::get().string(config::values::feedback_platform_id)), _params.get_value_as_string("platform")});
    fields.insert({"fb.user_name", _params.get_value_as_string("user_name")});
    fields.insert({"fb.message", _params.get_value_as_string("message")});
    fields.insert({"fb.communication_email", _params.get_value_as_string("communication_email")});
    fields.insert({"Lang", _params.get_value_as_string("language")});
    fields.insert({"attachements_count", _params.get_value_as_string("attachements_count")});

    if (config::get().is_on(config::features::feedback_selected))
    {
        const auto problem = config::get().string(config::values::feedback_selected_id);
        fields.insert(std::make_pair(std::string(problem), _params.get_value_as_string(problem)));
    }

    auto request = std::make_shared<core::send_feedback>(_params.get_value_as_string("aimid"), _params.get_value_as_string("url"), std::move(fields), to_string_vector(_params, "attachement"));
    request->send([_seq, request](bool _success)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_bool("succeeded", _success);
        g_core->post_message_to_gui("feedback/sent", _seq, coll.get());

        if (_success)
            g_core->insert_event(stats::stats_event_names::feedback_sent);
        else
            g_core->insert_event(stats::stats_event_names::feedback_error);
    });
}

void core::core_dispatcher::on_misc_support(int64_t _seq, coll_helper& _params)
{
    // first prepare an archive with client logs
    run_async([_seq, aimid = std::string(_params.get_value_as_string("aimid")), message = std::string(_params.get_value_as_string("message")), screenshots_list = to_string_vector(_params, "attachement")]() mutable
    {
        if (g_core)
        {
            g_core->execute_core_context([_seq, aimid = std::move(aimid), message = std::move(message), attachment = tools::from_utf16(utils::create_feedback_logs_archive({}).wstring()), screenshots_list = std::move(screenshots_list)]()
            {
                // then send the ready data to the support
                if (auto im = g_core->find_im_by_id(0))
                    im->misc_support(_seq, aimid, message, attachment, std::move(screenshots_list));
            });
        }

        return 0;
    });
}

void core::core_dispatcher::on_create_logs_archive(int64_t _seq, coll_helper& _params)
{
    g_core->run_async([_seq, path = boost::filesystem::wpath(tools::from_utf8(_params.get_value_as_string("path")))]()
    {
        coll_helper cl_coll(g_core->create_collection(), true);
        cl_coll.set_value_as_string("archive", tools::from_utf16(utils::create_logs_dir_archive(path).wstring()));

        g_core->post_message_to_gui("logs_archive", _seq, cl_coll.get());

        return 0;
    });
}

void core::core_dispatcher::save_themes_etag(std::string_view etag)
{
    settings_->set_value(core_settings_values::themes_settings_etag, etag);
    settings_->save();
}

std::string core::core_dispatcher::load_themes_etag()
{
    return settings_->get_value(core_settings_values::themes_settings_etag, std::string());
}

themes::wallpaper_info_v core::core_dispatcher::get_wallpaper_urls() const
{
    return theme_settings_->get_wallpaper_urls();
}

void core::core_dispatcher::on_message_network_log(coll_helper _params)
{
    write_string_to_network_log(_params.get_value_as_string("text"));
}


void core::core_dispatcher::on_message_log(coll_helper _params) const
{
    const std::string_view type = _params.get_value_as_string("type");
    const auto area = _params.get_value_as_string("area");
    const auto text = _params.get_value_as_string("text");

    if (type == "trace")
    {
        log::trace(area, text);
        return;
    }

    if (type == "info")
    {
        log::info(area, text);
        return;
    }

    if (type == "warn")
    {
        log::warn(area, text);
        return;
    }

    if (type == "error")
    {
        log::error(area, text);
        return;
    }

    assert(!"unknown log record type");
}

void core::core_dispatcher::on_message_profiler_proc_start(coll_helper _params) const
{
    const auto name = _params.get_value_as_string("name");
    const auto id = _params.get_value_as_int64("id");
    const auto ts = _params.get_value_as_int64("ts");

    profiler::process_started(name, id, ts);
}

void core::core_dispatcher::on_message_profiler_proc_stop(coll_helper _params) const
{
    const auto id = _params.get_value_as_int64("id");
    const auto ts = _params.get_value_as_int64("ts");

    profiler::process_stopped(id, ts);
}

void core::core_dispatcher::receive_message_from_gui(std::string_view _message, int64_t _seq, icollection* _message_data)
{
    // called from main thread

    if (_message_data)
        _message_data->addref();

    if (_message.empty())
    {
        assert(false);
        return;
    }

    execute_core_context([this, message_string = std::string(_message), _seq, _message_data]
    {
        coll_helper params(_message_data, true);

        if (message_string.front() != '_')
        {
            tools::binary_stream bs;
            bs.write<std::string_view>("GUI->CORE: message=");
            bs.write<std::string_view>(message_string);
            write_additional_coregui_message_data(bs, _message_data);
            bs.write<std::string_view>("\r\n");

            if (message_string == "archive/messages/get")
            {
                std::stringstream s;
                s << " from: ";
                s << params.get_value_as_int64("from");
                s << " count_early: ";
                s << params.get_value_as_int64("count_early");
                s << " count_later: ";
                s << params.get_value_as_int64("count_later");

                bs.write<std::string_view>(s.str());
                bs.write<std::string_view>("\r\n");
            }
            else if (message_string == "archive/buddies/get" || message_string == "archive/log/model")
            {
                std::stringstream s;
                if (params.is_value_exist("ids"))
                {
                    s << " ids: ";

                    if (auto ids = params.get_value_as_array("ids"))
                    {
                        for (int32_t i = 0, size = ids->size(); i < size; ++i)
                            s << ids->get_at(i)->get_as_int64() << "; ";
                    }
                }

                if (params.is_value_exist("first_id"))
                    s << " first_id: " << params.get_value_as_int64("first_id") << "; ";
                if (params.is_value_exist("last_id"))
                    s << " last_id: " << params.get_value_as_int64("last_id") << "; ";

                s << "\r\n";

                if (params.is_value_exist("view_bottom"))
                    s << "view_bottom: " << params.get_value_as_int64("view_bottom") << "; ";
                if (params.is_value_exist("view_top"))
                    s << "view_top: " << params.get_value_as_int64("view_top") << "; ";
                if (params.is_value_exist("view_bottom_pending"))
                    s << "view_bottom_pending: " << params.get_value_as_string("view_bottom_pending") << "; ";
                if (params.is_value_exist("view_top_pending"))
                    s << "view_top_pending: " << params.get_value_as_string("view_top_pending") << "; ";

                if (params.is_value_exist("hint"))
                    s << "\r\nhint: " << params.get_value_as_string("hint");

                bs.write<std::string_view>(s.str());
                bs.write<std::string_view>("\r\n");
            }
            // Added type of voip call to log.
            else if (message_string == "voip_call")
            {
#ifndef STRIP_VOIP
                std::stringstream s;
                s << "type: ";
                s << params.get_value_as_string("type");

                voip_manager::addVoipStatistic(s, params);

                bs.write<std::string_view>(s.str());
                bs.write<std::string_view>("\r\n");
#endif
            }

            write_data_to_network_log(std::move(bs));
        }

        if (message_string == "settings/value/set")
        {
            on_message_update_gui_settings_value(_seq, params);
        }
        else if (message_string == message_log)
        {
            on_message_log(params);
        }
        else if (message_string == message_network_log)
        {
            on_message_network_log(params);
        }
        else if (message_string == "profiler/proc/start")
        {
            on_message_profiler_proc_start(params);
        }
        else if (message_string == "profiler/proc/stop")
        {
            on_message_profiler_proc_stop(params);
        }
        else if (message_string == "themes/settings/set")
        {
            on_message_update_theme_settings_value(_seq, params);
        }
        else if (message_string == "themes/default/id")
        {
            on_message_set_default_theme_id(_seq, params);
        }
        else if (message_string == "themes/default/wallpaper/id")
        {
            on_message_set_default_wallpaper_id(_seq, params);
        }
        else if (message_string == "themes/wallpaper/urls")
        {
            on_message_set_wallpaper_urls(_seq, params);
        }
        else if (message_string == "feedback/send")
        {
            if (config::get().is_on(config::features::contact_us_via_backend))
                on_misc_support(_seq, params);
            else
                on_feedback(_seq, params);
        }
        else if (message_string == "create_logs_archive")
        {
            on_create_logs_archive(_seq, params);
        }
        else
        {
            im_container_->on_message_from_gui(message_string, _seq, params);
        }
    });
}

const common::core_gui_settings& core::core_dispatcher::get_core_gui_settings() const
{
    return core_gui_settings_;
}

void core::core_dispatcher::set_recent_avatars_size(int32_t _size)
{
    core_gui_settings_.recents_avatars_size_ = _size;
}

std::string core::core_dispatcher::get_root_login()
{
    auto im = find_im_by_id(1);
    if (!im)
        return std::string();

    return im->get_login();
}

std::string core::core_dispatcher::get_login_after_start() const
{
    auto logins = im_login_list(utils::get_product_data_path() + L"/settings/ims.stg");
    if (logins.load())
    {
        im_login_id login(std::string(), default_im_id);
        if (logins.get_first_login(login))
            return login.get_login();
        assert(false);
    }

    return std::string();
}

std::string core::core_dispatcher::get_contact_friendly_name(unsigned _id, const std::string& _contact_login)
{
    auto im = find_im_by_id(_id);
    if (im)
        return im->get_contact_friendly_name(_contact_login);

    return std::string();
}

bool core::core_dispatcher::is_valid_cl_search() const
{
    return cl_search_count_.load() == 1;
}

void core::core_dispatcher::begin_cl_search()
{
    ++cl_search_count_;
}

unsigned core::core_dispatcher::end_cl_search()
{
    return --cl_search_count_;
}

void core::core_dispatcher::update_login(im_login_id& _login)
{
    // if created new login
    if (!im_container_->update_login(_login))
    {
        g_core->post_message_to_gui("login_new_user", 0, nullptr);
    }

    start_session_stats(false /* delayed */);
}

void core::core_dispatcher::replace_uin_in_login(im_login_id& old_login, im_login_id& new_login)
{
    im_container_->replace_uin_in_login(old_login, new_login);
}

#ifndef STRIP_VOIP
void core::core_dispatcher::post_voip_message(unsigned _id, const voip_manager::VoipProtoMsg& msg) {
    execute_core_context([this, _id, msg] {
        auto im = find_im_by_id(_id);
        assert(!!im);

        if (!!im) {
            im->post_voip_msg_to_server(msg);
        }
    });
}

void core::core_dispatcher::post_voip_alloc(unsigned _id, const char* _data, size_t _len) {
    std::string data_str(_data, _len);
    execute_core_context([this, _id, data_str] {
        auto im = find_im_by_id(_id);
        assert(!!im);

        if (!!im) {
            im->post_voip_alloc_to_server(data_str);
        }
    });
}
#endif

void core::core_dispatcher::on_thread_finish()
{
}

void core::core_dispatcher::unlogin(const bool _is_auth_error, const bool _force_clean_local_data, std::string_view _reason_to_log)
{
    tools::binary_stream bs;
    bs.write<std::string_view>("unlogin: ");
    bs.write<std::string_view>(_reason_to_log);
    bs.write<std::string_view>("\r\n");

    write_data_to_network_log(std::move(bs));

    execute_core_context([this, _is_auth_error]()
    {
        im_container_->logout(_is_auth_error);
    });

    if (_force_clean_local_data)
    {
        auto stream_val = tools::binary_stream();
        stream_val.write(false);
        gui_settings_->set_value(settings_keep_logged_in, std::move(stream_val));

        execute_core_context([this]()
        {
            remove_local_data(false);
        });
    }
}

bool core::core_dispatcher::is_network_log_valid() const
{
    if (network_log_)
        return true;
    return false;
}

network_log& core::core_dispatcher::get_network_log()
{
    return (*network_log_);
}

bool core_dispatcher::get_gui_settings_bool_value(std::string_view _name, const bool _default) const
{
    bool flag = _default;
    if (gui_settings_)
    {
        tools::binary_stream bs;
        if (gui_settings_->get_value(_name, bs) && bs.available() >= sizeof(bool))
            flag = bs.read<bool>();
    }

    return flag;
}

const std::shared_ptr<core::stats::statistics>& core::core_dispatcher::get_statistics() const
{
    return statistics_;
}

proxy_settings core_dispatcher::get_proxy_settings() const
{
    if (proxy_settings_manager_)
        return proxy_settings_manager_->get_current_settings();
    return {};
}

bool core_dispatcher::try_to_apply_alternative_settings()
{
    return proxy_settings_manager_->try_to_apply_alternative_settings();
}

void core_dispatcher::set_user_proxy_settings(const proxy_settings& _user_proxy_settings)
{
    proxy_settings_manager_->apply(_user_proxy_settings);
    g_core->post_user_proxy_to_gui();
}

bool core_dispatcher::locale_was_changed() const
{
    static bool stored = settings_->get_locale_was_changed();
    if (stored)
        settings_->set_locale_was_changed(false);
    return stored;
}

void core_dispatcher::set_locale(const std::string& _locale)
{
    assert(is_core_thread());

    static std::string stored;
    if (stored.empty())
        stored = get_locale();
    if (!stored.empty())
    {
        locale_was_changed(); // just store initiated value
        settings_->set_locale_was_changed(_locale != stored);
    }
    settings_->set_locale(_locale);
}

std::string core_dispatcher::get_locale() const
{
    assert(is_core_thread());

    return settings_->get_locale();
}

void core_dispatcher::post_user_proxy_to_gui()
{
    auto user_proxy = proxy_settings_manager_->get_current_settings();
    coll_helper cl_coll(create_collection(), true);
    user_proxy.serialize(cl_coll);
    g_core->post_message_to_gui("user_proxy/result", 0, cl_coll.get());
}

void core_dispatcher::setup_memory_stats_collector()
{
    memory_stats_collector_->register_async_consumption_reporter(std::make_unique<gui_memory_consumption_reporter>());
}

std::thread::id core_dispatcher::get_core_thread_id() const
{
   return core_thread_->get_core_thread_id();
}

bool core_dispatcher::is_core_thread() const
{
    return (std::this_thread::get_id() == get_core_thread_id());
}

void core_dispatcher::set_voip_mute_fix_flag(bool bValue)
{
    settings_->set_voip_mute_fix_flag(bValue);
}

bool core_dispatcher::get_voip_mute_fix_flag() const
{
    return settings_->get_voip_mute_fix_flag();
}

void core_dispatcher::update_outgoing_msg_count(const std::string& _aimid, int _count)
{
    execute_core_context([this, _aimid, _count]()
    {
        auto im = find_im_by_id(1);
        if (im)
            im->update_outgoing_msg_count(_aimid, _count);
    });
}

void core_dispatcher::post_install_action()
{
    constexpr auto timeout = std::chrono::seconds(20);
    delayed_post_timer_id_ = add_timer([this]
    {
        stop_timer(delayed_post_timer_id_);

        installer_services::post_install_action act(utils::get_product_data_path() + L'/' + ::common::get_final_act_filename());
        act.load();
        act.delete_tmp_resources();
    }, timeout);
}

static void omicron_log_helper(const std::string& _text)
{
    if (g_core)
        g_core->write_string_to_network_log(_text);
}

static void omicron_update_helper(const std::string& _data)
{
    if (g_core)
    {
        g_core->execute_core_context([_data]
        {
            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set_value_as_string("data", _data);
            g_core->post_message_to_gui("omicron/update/data", 0, cl_coll.get());

            g_core->check_if_network_change_notifier_available();
        });
    }
}

static bool omicron_download_helper(const omicronlib::omicron_proxy_settings& /*_proxy_settings*/, const std::string& _url, std::string& _json_data, long& _response_code)
{
    core::http_request_simple request(g_core->get_proxy_settings(), utils::get_user_agent(), top_priority());
    request.set_url(_url);
    request.set_send_im_stats(false);
    request.set_normalized_url("omicron_dl");
    request.set_use_curl_decompresion(true);

    auto res = request.get();
    if (res != curl_easy::completion_code::success)
        return false;

    _response_code = request.get_response_code();

    auto response = std::static_pointer_cast<tools::binary_stream>(request.get_response());
    if (response->available())
    {
        response->write(char(0));
        _json_data = response->read_available();
    }

    return true;
}

static const std::string& omicron_encrypt_hash()
{
    static const std::string hash = []()
    {
        char buffer[65] = { 0 };
        core::tools::sha256(get_omicron_dev_id(), buffer);
        return std::string(buffer);
    }();

    return hash;
}

static bool omicron_save_helper(const std::wstring& _path, const std::string& _json_data)
{
    core::tools::binary_stream bstream;
    bstream.write(_json_data);
    bstream.encrypt_aes256(omicron_encrypt_hash());
    return bstream.save_2_file(_path);
}

static std::string omicron_load_helper(const std::wstring& _path)
{
    std::string res;
    core::tools::binary_stream bstream;
    if (bstream.load_from_file(_path))
    {
        if (bstream.decrypt_aes256(omicron_encrypt_hash()))
        {
            bstream.write<char>('\0');
            res = bstream.read(bstream.available());
        }
    }
    return res;
}

void core::core_dispatcher::start_omicron_service()
{
    std::string inifile_dev_id = core::configuration::get_app_config().device_id();
    const auto app_id = inifile_dev_id.empty() ? std::string(get_omicron_dev_id()) : inifile_dev_id;

    omicronlib::omicron_config conf(su::concat("https://", configuration::get_app_config().get_url_omicron_data()), app_id, get_omicron_app_environment());

    auto app_version = su::concat(core::tools::version_info().get_major_version(), '.', core::tools::version_info().get_minor_version());
    conf.add_fingerprint("app_version", std::move(app_version));
    conf.add_fingerprint("app_build", core::tools::version_info().get_build_version());
    conf.add_fingerprint("os_version", core::tools::system::get_os_version_string());
    conf.add_fingerprint("device_id", g_core->get_uniq_device_id());
    conf.add_fingerprint("arch", std::string(get_omicron_arch()));
    auto account = g_core->get_root_login();
    if (!account.empty())
        conf.add_fingerprint("account", account);
    conf.add_fingerprint("lang", g_core->get_locale());

    conf.set_logger(omicron_log_helper);
    conf.set_callback_updater(omicron_update_helper);
    conf.set_json_downloader(omicron_download_helper);

    if constexpr (!build::is_debug() && !environment::is_develop())
    {
        conf.set_save_helper(omicron_save_helper);
        conf.set_load_helper(omicron_load_helper);
    }

    omicronlib::omicron_init(conf, su::wconcat(utils::get_product_data_path(), L"/settings/", tools::from_utf8(omicron_cache_file_name)));

    auto inifile_update_interval = core::configuration::get_app_config().update_interval();
    auto update_period = (inifile_update_interval == 0) ? omicronlib::default_update_interval() : std::chrono::seconds(inifile_update_interval);
    omicron_update_timer_id_ = add_timer([fl = account.empty()]()
    {
        static auto need_add_account = fl;
        if (need_add_account && g_core)
        {
            auto account = g_core->get_root_login();
            if (!account.empty())
            {
                need_add_account = false;
                omicronlib::omicron_add_fingerprint("account", account);
            }
        }

        omicronlib::omicron_update();
    }, update_period);
}

void core_dispatcher::set_last_stat_send_time(int64_t _time)
{
    settings_->set_value(last_stat_send_time, _time);
    settings_->save();
}

int64_t core_dispatcher::get_last_stat_send_time()
{
    return settings_->get_value(last_stat_send_time, 0);
}

void core_dispatcher::set_local_pin_enabled(bool _enable)
{
    settings_->set_value(local_pin_enabled, _enable);
    settings_->save();
}

bool core_dispatcher::get_local_pin_enabled() const
{
    return settings_->get_value(local_pin_enabled, false);
}

void core_dispatcher::set_install_beta_updates(bool _enable)
{
    settings_->set_value(install_beta_updates, _enable);
    settings_->save();
}

bool core_dispatcher::get_install_beta_updates() const
{
    return omicronlib::_o(omicron::keys::beta_update, config::get().is_on(config::features::beta_update)) &&  settings_->get_value(install_beta_updates, environment::is_beta());
}

void core_dispatcher::set_local_pin(const std::string& _password)
{
    const auto salt = std::to_string(utils::rand_number());

    settings_->set_value(local_pin_salt, salt);
    settings_->set_value(local_pin_hash, utils::calc_local_pin_hash(_password, salt));
    settings_->save();
}

std::string core_dispatcher::get_local_pin_hash() const
{
    return settings_->get_value(local_pin_hash, std::string());
}

std::string core_dispatcher::get_local_pin_salt() const
{
    return settings_->get_value(local_pin_salt, std::string());
}

bool core_dispatcher::verify_local_pin(const std::string& _password) const
{
    const auto salt = settings_->get_value(local_pin_salt, std::string());

    return utils::calc_local_pin_hash(_password, salt) == settings_->get_value(local_pin_hash, std::string());
}

void core_dispatcher::set_external_config_url(std::string_view _url)
{
    settings_->set_value(external_config_host, _url);
    settings_->save();
}

std::string core_dispatcher::get_external_config_url() const
{
    auto result = settings_->get_value(external_config_host, std::string());
    if (result.empty())
        result = config::get().string(config::values::external_config_preset_url);
    return result;
}

int64_t core_dispatcher::get_voip_init_memory_usage() const
{
    if (!voip_manager_)
        return 0;

    return voip_manager_->get_voip_initialization_memory();
}

void core_dispatcher::check_update(int64_t _seq, std::optional<std::string> _url)
{
    updater_->check(_seq, std::move(_url));
}

bool core_dispatcher::is_keep_local_data() const
{
    return get_gui_settings_bool_value(settings_keep_logged_in, settings_keep_logged_in_default());
}

void core_dispatcher::remove_local_data(bool _is_exit)
{
    if (gui_settings_)
        gui_settings_->clear_personal_values();

    utils::remove_user_data_dirs();

    const auto data_path = utils::get_product_data_path();
    utils::remove_dir_contents_recursively2(data_path + L"/settings", { core_settings_file_name, gui_settings_file_name, theme_settings_file_name });

    if (_is_exit)
    {
        tools::system::delete_directory(data_path + L"/stats");
    }
}

void core_dispatcher::update_gui_settings()
{
    if (gui_settings_)
        post_gui_settings();
}

void core_dispatcher::reset_connection()
{
    curl_handler::instance().reset();
}

void core::core_dispatcher::check_if_network_change_notifier_available()
{    
    if (!network_change_notifier_ && network_change_notifier::is_enabled())
    {
        auto network_observer = std::make_unique<network_change_observer>();
        network_change_notifier_ = network_change_notifier::create(std::move(network_observer));
    }
    else if (network_change_notifier_ && !network_change_notifier::is_enabled())
    {
        network_change_notifier_.reset();
    }
}

bool core::core_dispatcher::is_network_change_notifier_valid() const
{
    return !!network_change_notifier_;
}

bool core_dispatcher::is_smartreply_available() const
{
    return get_gui_settings_bool_value(settings_show_smartreply, true);
}

bool core_dispatcher::is_suggests_available() const
{
    return get_gui_settings_bool_value(settings_show_suggests_words, true);
}

#ifndef STRIP_ZSTD
const std::shared_ptr<zstd_helper>& core_dispatcher::get_zstd_helper() const
{
    return zstd_helper_;
}
#endif // !STRIP_ZSTD
