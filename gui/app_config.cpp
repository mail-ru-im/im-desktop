#include "stdafx.h"

#include "../common.shared/config/config.h"
#ifndef STRIP_CRASH_HANDLER
#ifdef _WIN32
#include "../common.shared/crash_report/crash_reporter.h"
#endif //_WIN32
#endif // !STRIP_CRASH_HANDLER

#include "../corelib/collection_helper.h"
#include "../gui/core_dispatcher.h"
#include "utils/gui_coll_helper.h"

#ifdef IM_AUTO_TESTING
#include <rapidjson/writer.h>
#endif

#include "app_config.h"

UI_NS_BEGIN

namespace
{
    AppConfigUptr AppConfig_;

    constexpr std::string_view c_dev_unlock_context_menu_features = "dev.unlock_context_menu_features";
    constexpr std::string_view c_history_is_server_history_enabled = "history.is_server_history_enabled";
    constexpr std::string_view c_enable_crash = "enable_crash";
    constexpr std::string_view c_enable_testing = "enable_testing";
    constexpr std::string_view c_fulllog = "fulllog";
    constexpr std::string_view c_updateble = "updateble";
    constexpr std::string_view c_dev_show_message_ids = "dev.show_message_ids";
    constexpr std::string_view c_dev_save_rtp_dumps = "dev.save_rtp_dumps";
    constexpr std::string_view c_dev_server_search = "dev.server_search";
    constexpr std::string_view c_show_hidden_themes = "show_hidden_themes";
    constexpr std::string_view c_sys_crash_handler_enabled = "sys_crash_handler_enabled";
    constexpr std::string_view c_dev_net_compression = "dev.net_compression";
    constexpr std::string_view c_ssl_verification_enabled = "dev.webview_ssl_check";
    constexpr std::string_view c_dev_watch_gui_memory = "dev.watch_gui_memory";
    constexpr std::string_view c_gdpr_user_has_agreed = "gdpr.user_has_agreed";
    constexpr std::string_view c_gdpr_agreement_reported_to_server = "gdpr.agreement_reported_to_server";
    constexpr std::string_view c_gdpr_user_has_logged_in_ever = "gdpr.user_has_logged_in_ever";
    constexpr std::string_view c_dev_cache_history_pages_secs = "dev.cache_history_pages_secs";
    constexpr std::string_view c_dev_cache_history_pages_check_interval_secs = "dev.cache_history_pages_check_interval_secs";
    constexpr std::string_view c_dev_app_update_interval_secs = "dev.app_update_interval_secs";
    constexpr std::string_view c_dev_id = "dev_id";
    constexpr std::string_view c_dev_show_seconds_in_time_picker = "dev.show_seconds_in_time_picker";
    constexpr std::string_view c_urls_url_update_mac_alpha = "urls.url_update_mac_alpha";
    constexpr std::string_view c_urls_url_update_mac_beta = "urls.url_update_mac_beta";
    constexpr std::string_view c_urls_url_update_mac_release = "urls.url_update_mac_release";
    constexpr std::string_view c_urls_url_attach_phone = "urls.url_attach_phone";
}

AppConfig::AppConfig(const core::coll_helper& collection)
    : IsContextMenuFeaturesUnlocked_(collection.get<bool>(c_dev_unlock_context_menu_features))
    , IsServerHistoryEnabled_(collection.get<bool>(c_history_is_server_history_enabled))
    , IsCrashEnable_(collection.get<bool>(c_enable_crash))
    , IsTestingEnable_(collection.get<bool>(c_enable_testing))
    , IsFullLogEnabled_(collection.get<bool>(c_fulllog))
    , IsUpdateble_(collection.get<bool>(c_updateble))
    , IsShowMsgIdsEnabled_(collection.get<bool>(c_dev_show_message_ids))
    , IsSaveCallRTPEnabled_(collection.get<bool>(c_dev_save_rtp_dumps, false))
    , IsServerSearchEnabled_(collection.get<bool>(c_dev_server_search, true))
    , IsShowHiddenThemes_(collection.get<bool>(c_show_hidden_themes, false))
    , IsSysCrashHandleEnabled_(collection.get<bool>(c_sys_crash_handler_enabled, false))
    , IsNetCompressionEnabled_(collection.get<bool>(c_dev_net_compression, true))
    , IsSslVerificationEnabled_(collection.get<bool>(c_ssl_verification_enabled, true))
    , showSecondsInTimePicker_(collection.get<bool>(c_dev_show_seconds_in_time_picker, false))
    , WatchGuiMemoryEnabled_(collection.get<bool>(c_dev_watch_gui_memory, false))
    , ShowMsgOptionHasChanged_(false)
    , GDPR_UserHasAgreed_(collection.get<bool>(c_gdpr_user_has_agreed) || config::get().is_on(config::features::auto_accepted_gdpr))
    , GDPR_AgreementReportedToServer_(collection.get<int32_t>(c_gdpr_agreement_reported_to_server) || config::get().is_on(config::features::auto_accepted_gdpr))
    , GDPR_UserHasLoggedInEver_(collection.get<bool>(c_gdpr_user_has_logged_in_ever) || config::get().is_on(config::features::auto_accepted_gdpr))
    , CacheHistoryContolPagesFor_(collection.get<int>(c_dev_cache_history_pages_secs))
    , CacheHistoryContolPagesCheckInterval_(collection.get<int>(c_dev_cache_history_pages_check_interval_secs))
    , AppUpdateIntervalSecs_(collection.get<uint32_t>(c_dev_app_update_interval_secs))
    , deviceId_(collection.get<std::string>(c_dev_id))
    , urlMacUpdateAlpha_(collection.get<std::string>(c_urls_url_update_mac_alpha))
    , urlMacUpdateBeta_(collection.get<std::string>(c_urls_url_update_mac_beta))
    , urlMacUpdateRelease_(collection.get<std::string>(c_urls_url_update_mac_release))
    , urlAttachPhone_(collection.get<std::string>(c_urls_url_attach_phone))
{
#ifndef STRIP_CRASH_HANDLER
#ifdef _WIN32
    crash_system::reporter::instance().set_sys_handler_enabled(IsSysCrashHandleEnabled_);
#endif //_WIN32
#endif // !STRIP_CRASH_HANDLER
}

bool AppConfig::IsContextMenuFeaturesUnlocked() const noexcept
{
    return environment::is_develop() || IsContextMenuFeaturesUnlocked_;
}

bool AppConfig::IsServerHistoryEnabled() const noexcept
{
    return IsServerHistoryEnabled_;
}

bool AppConfig::isCrashEnable() const noexcept
{
    return IsCrashEnable_;
}

bool AppConfig::IsTestingEnable() const noexcept
{
    return IsTestingEnable_;
}

bool AppConfig::IsFullLogEnabled() const noexcept
{
    return IsFullLogEnabled_;
}

bool AppConfig::IsUpdateble() const noexcept
{
    return IsUpdateble_;
}

bool AppConfig::IsShowMsgIdsEnabled() const noexcept
{
    return IsShowMsgIdsEnabled_;
}

bool AppConfig::IsSaveCallRTPEnabled() const noexcept
{
    return IsSaveCallRTPEnabled_;
}

bool AppConfig::IsServerSearchEnabled() const noexcept
{
    return IsServerSearchEnabled_;
}

bool AppConfig::IsShowHiddenThemes() const noexcept
{
    return IsShowHiddenThemes_;
}

bool AppConfig::IsSysCrashHandleEnabled() const noexcept
{
    return IsSysCrashHandleEnabled_;
}

bool AppConfig::IsNetCompressionEnabled() const noexcept
{
    return IsNetCompressionEnabled_;
}

bool AppConfig::IsSslVerificationEnabled() const noexcept
{
    return IsSslVerificationEnabled_;
}

bool AppConfig::showSecondsInTimePicker() const noexcept
{
    return showSecondsInTimePicker_;
}

bool AppConfig::WatchGuiMemoryEnabled() const noexcept
{
    return WatchGuiMemoryEnabled_;
}

bool AppConfig::ShowMsgOptionHasChanged() const noexcept
{
    return ShowMsgOptionHasChanged_;
}

bool AppConfig::GDPR_UserHasAgreed() const noexcept
{
    return GDPR_UserHasAgreed_;
}

int32_t AppConfig::GDPR_AgreementReportedToServer() const noexcept
{
    return GDPR_AgreementReportedToServer_;
}

bool AppConfig::GDPR_AgreedButAwaitingSend() const noexcept
{
    static std::set<GDPR_Report_To_Server_State> AwaitingResponseStates = {
        GDPR_Report_To_Server_State::Sent,
        GDPR_Report_To_Server_State::Sent_to_Core,
        GDPR_Report_To_Server_State::Failed,
    };

    auto currentState = static_cast<GDPR_Report_To_Server_State>(GDPR_AgreementReportedToServer());

    return GDPR_UserHasAgreed() && (AwaitingResponseStates.find(currentState) != AwaitingResponseStates.end());

}

bool AppConfig::GDPR_UserHasLoggedInEver() const noexcept
{
    return GDPR_UserHasLoggedInEver_;
}

const std::string& AppConfig::getMacUpdateAlpha() const noexcept
{
    return urlMacUpdateAlpha_;
}

const std::string& AppConfig::getMacUpdateBeta() const noexcept
{
    return urlMacUpdateBeta_;
}

const std::string& AppConfig::getMacUpdateRelease() const noexcept
{
    return urlMacUpdateRelease_;
}

const std::string& AppConfig::getUrlAttachPhone() const noexcept
{
    return urlAttachPhone_;
}

const std::string& AppConfig::getDevId() const noexcept
{
    return deviceId_;
}

std::chrono::seconds AppConfig::CacheHistoryControlPagesFor() const noexcept
{
    return std::chrono::seconds(CacheHistoryContolPagesFor_);
}

std::chrono::seconds AppConfig::CacheHistoryControlPagesCheckInterval() const noexcept
{
    return std::chrono::seconds(CacheHistoryContolPagesCheckInterval_);
}

uint32_t AppConfig::AppUpdateIntervalSecs() const noexcept
{
    return AppUpdateIntervalSecs_;
}

void AppConfig::SetFullLogEnabled(bool enabled) noexcept
{
    IsFullLogEnabled_ = enabled;
}

void AppConfig::SetUpdateble(bool enabled) noexcept
{
    IsUpdateble_ = enabled;
}

void AppConfig::SetSaveCallRTPEnabled(bool enabled) noexcept
{
    IsSaveCallRTPEnabled_ = enabled;
}

void AppConfig::SetServerSearchEnabled(bool enabled) noexcept
{
    IsServerSearchEnabled_ = enabled;
}

void AppConfig::SetShowHiddenThemes(bool enabled) noexcept
{
    IsShowHiddenThemes_ = enabled;
}

void AppConfig::SetContextMenuFeaturesUnlocked(bool unlocked) noexcept
{
    IsContextMenuFeaturesUnlocked_ = unlocked;
}

void AppConfig::SetShowMsgIdsEnabled(bool doShow) noexcept
{
    IsShowMsgIdsEnabled_ = doShow;
}

void AppConfig::SetShowMsgOptionHasChanged(bool changed) noexcept
{
    ShowMsgOptionHasChanged_ = changed;
}

void AppConfig::SetGDPR_UserHasLoggedInEver(bool hasLoggedIn) noexcept
{
    GDPR_UserHasLoggedInEver_ = hasLoggedIn;
}

void AppConfig::SetGDPR_AgreementReportedToServer(GDPR_Report_To_Server_State state) noexcept
{
    GDPR_AgreementReportedToServer_ = static_cast<int32_t>(state);
}

void AppConfig::SetCacheHistoryControlPagesFor(int secs) noexcept
{
    CacheHistoryContolPagesFor_ = secs;
}

void AppConfig::SetWatchGuiMemoryEnabled(bool _watch) noexcept
{
    WatchGuiMemoryEnabled_ = _watch;
}

void AppConfig::SetCustomDeviceId(bool _custom) noexcept
{
    if (_custom)
        deviceId_ = feature::default_dev_id();
    else
        deviceId_.clear();
}

void AppConfig::SetNetCompressionEnabled(bool _enabled) noexcept
{
    IsNetCompressionEnabled_ = _enabled;
}

void AppConfig::setShowSecondsInTimePicker(bool _enabled) noexcept
{
    showSecondsInTimePicker_ = _enabled;
}

bool AppConfig::hasCustomDeviceId() const
{
    return !deviceId_.empty() && deviceId_ != common::get_dev_id();
}

#ifdef IM_AUTO_TESTING
QString AppConfig::toJsonString() const
{
    const auto appConfig = Ui::GetAppConfig();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember(rapidjson::StringRef(c_dev_unlock_context_menu_features.data()), IsContextMenuFeaturesUnlocked(), a);
    doc.AddMember(rapidjson::StringRef(c_history_is_server_history_enabled.data()), IsServerHistoryEnabled(), a);
    doc.AddMember(rapidjson::StringRef(c_enable_crash.data()), isCrashEnable(), a);
    doc.AddMember(rapidjson::StringRef(c_enable_testing.data()), IsTestingEnable(), a);
    doc.AddMember(rapidjson::StringRef(c_fulllog.data()), IsFullLogEnabled(), a);
    doc.AddMember(rapidjson::StringRef(c_updateble.data()), IsUpdateble(), a);
    doc.AddMember(rapidjson::StringRef(c_dev_show_message_ids.data()), IsShowMsgIdsEnabled(), a);
    doc.AddMember(rapidjson::StringRef(c_dev_save_rtp_dumps.data()), IsSaveCallRTPEnabled(), a);
    doc.AddMember(rapidjson::StringRef(c_dev_server_search.data()), IsServerSearchEnabled(), a);
    doc.AddMember(rapidjson::StringRef(c_show_hidden_themes.data()), IsShowHiddenThemes(), a);
    doc.AddMember(rapidjson::StringRef(c_sys_crash_handler_enabled.data()), IsSysCrashHandleEnabled(), a);
    doc.AddMember(rapidjson::StringRef(c_dev_net_compression.data()), IsNetCompressionEnabled(), a);
    doc.AddMember(rapidjson::StringRef(c_dev_watch_gui_memory.data()), WatchGuiMemoryEnabled(), a);
    doc.AddMember(rapidjson::StringRef(c_gdpr_user_has_agreed.data()), GDPR_UserHasAgreed(), a);
    doc.AddMember(rapidjson::StringRef(c_gdpr_agreement_reported_to_server.data()), GDPR_AgreementReportedToServer(), a);
    doc.AddMember(rapidjson::StringRef(c_gdpr_user_has_logged_in_ever.data()), GDPR_UserHasLoggedInEver(), a);
    doc.AddMember(rapidjson::StringRef(c_dev_cache_history_pages_secs.data()), CacheHistoryControlPagesFor().count(), a);
    doc.AddMember(rapidjson::StringRef(c_dev_cache_history_pages_check_interval_secs.data()), CacheHistoryControlPagesCheckInterval().count(), a);
    doc.AddMember(rapidjson::StringRef(c_dev_app_update_interval_secs.data()), AppUpdateIntervalSecs(), a);
    doc.AddMember(rapidjson::StringRef(c_dev_id.data()), deviceId_, a);
    doc.AddMember(rapidjson::StringRef(c_urls_url_update_mac_alpha.data()), urlMacUpdateAlpha_, a);
    doc.AddMember(rapidjson::StringRef(c_urls_url_update_mac_beta.data()), urlMacUpdateBeta_, a);
    doc.AddMember(rapidjson::StringRef(c_urls_url_update_mac_release.data()), urlMacUpdateRelease_, a);
    doc.AddMember(rapidjson::StringRef(c_urls_url_attach_phone.data()), getUrlAttachPhone(), a);
    doc.AddMember(rapidjson::StringRef(c_ssl_verification_enabled.data()), IsSslVerificationEnabled(), a);
    doc.AddMember(rapidjson::StringRef(c_dev_show_seconds_in_time_picker.data()), showSecondsInTimePicker(), a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return QString::fromLatin1(buffer.GetString(), buffer.GetSize());
}
#endif

const AppConfig& GetAppConfig()
{
    im_assert(AppConfig_);

    return *AppConfig_;
}

void SetAppConfig(AppConfigUptr&& appConfig)
{
    im_assert(appConfig);

    if (AppConfig_)
    {
        if (AppConfig_->IsShowMsgIdsEnabled() != appConfig->IsShowMsgIdsEnabled())
            appConfig->SetShowMsgOptionHasChanged(true);
    }

    AppConfig_ = std::move(appConfig);
}

void ModifyAppConfig(AppConfig _appConfig, message_processed_callback _callback, QObject *_qobj, bool _postToCore)
{
    if (!_postToCore)
    {
        SetAppConfig(std::make_unique<AppConfig>(std::move(_appConfig)));
        return;
    }

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_bool("fulllog", _appConfig.IsFullLogEnabled());
    collection.set_value_as_bool("updateble", _appConfig.IsUpdateble());
    collection.set_value_as_bool("dev.show_message_ids", _appConfig.IsShowMsgIdsEnabled());
    collection.set_value_as_bool("dev.save_rtp_dumps", _appConfig.IsSaveCallRTPEnabled());
    collection.set_value_as_bool("dev.unlock_context_menu_features", _appConfig.IsContextMenuFeaturesUnlocked());
    collection.set_value_as_bool("gdpr.user_has_logged_in_ever", _appConfig.GDPR_UserHasLoggedInEver());
    collection.set_value_as_int("dev.cache_history_pages_secs", _appConfig.CacheHistoryControlPagesFor().count());
    collection.set_value_as_int("dev.cache_history_pages_check_interval_secs", _appConfig.CacheHistoryControlPagesCheckInterval().count());
    collection.set_value_as_bool("dev.server_search", _appConfig.IsServerSearchEnabled());
    collection.set_value_as_bool("dev.watch_gui_memory", _appConfig.WatchGuiMemoryEnabled());
    collection.set_value_as_string("dev_id", _appConfig.getDevId());
    collection.set_value_as_bool("dev.net_compression", _appConfig.IsNetCompressionEnabled());
    collection.set_value_as_bool(c_dev_show_seconds_in_time_picker, _appConfig.showSecondsInTimePicker());

    GetDispatcher()->post_message_to_core("change_app_config", collection.get(), _qobj,
        [_appConfig{ std::move(_appConfig) }, _callback{ std::move(_callback) }](core::icollection* _coll)
        {
            Ui::gui_coll_helper coll(_coll, false);
            auto err = coll.get_value_as_int("error");
            Q_UNUSED(err);

            SetAppConfig(std::make_unique<AppConfig>(std::move(_appConfig)));

            if (_callback)
                _callback(_coll);
        });
}

UI_NS_END