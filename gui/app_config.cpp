#include "stdafx.h"

#include "../corelib/collection_helper.h"
#include "../gui/core_dispatcher.h"
#include "utils/gui_coll_helper.h"

#include "app_config.h"

UI_NS_BEGIN

namespace
{
    AppConfigUptr AppConfig_;
}

AppConfig::AppConfig(const core::coll_helper &collection)
    : IsContextMenuFeaturesUnlocked_(collection.get<bool>("dev.unlock_context_menu_features"))
    , IsServerHistoryEnabled_(collection.get<bool>("history.is_server_history_enabled"))
    , IsCrashEnable_(collection.get<bool>("enable_crash"))
    , IsTestingEnable_(collection.get<bool>("enable_testing"))
    , IsFullLogEnabled_(collection.get<bool>("fulllog"))
    , IsUpdateble_(collection.get<bool>("updateble"))
    , IsShowMsgIdsEnabled_(collection.get<bool>("dev.show_message_ids"))
    , IsSaveCallRTPEnabled_(collection.get<bool>("dev.save_rtp_dumps", false))
    , IsServerSearchEnabled_(collection.get<bool>("dev.server_search", true))
    , WatchGuiMemoryEnabled_(false)
    , ShowMsgOptionHasChanged_(false)
    , GDPR_UserHasAgreed_(collection.get<bool>("gdpr.user_has_agreed") || build::is_dit() || build::is_biz())
    , GDPR_AgreementReportedToServer_(collection.get<int32_t>("gdpr.agreement_reported_to_server") || build::is_dit() || build::is_biz())
    , GDPR_UserHasLoggedInEver_(collection.get<bool>("gdpr.user_has_logged_in_ever") || build::is_dit() || build::is_biz())
    , CacheHistoryContolPagesFor_(collection.get<int>("dev.cache_history_pages_secs"))
    , urlBase_(collection.get<std::string>("urls.url_base"))
    , urlFiles_(collection.get<std::string>("urls.url_files"))
    , urlFilesGet_(collection.get<std::string>("urls.url_files_get"))
    , urlAgentProfile_(collection.get<std::string>("urls.url_profile_agent"))
    , urlAuthMailRu_(collection.get<std::string>("urls.url_auth_mail_ru"))
    , urlRMailRu_(collection.get<std::string>("urls.url_r_mail_ru"))
    , urlWinMailRu_(collection.get<std::string>("urls.url_win_mail_ru"))
    , urlReadMsg_(collection.get<std::string>("urls.url_read_msg"))
    , urlCICQOrg_(collection.get<std::string>("urls.url_cicq_org"))
    , urlCICQCom_(collection.get<std::string>("urls.url_cicq_com"))
{

}

bool AppConfig::IsContextMenuFeaturesUnlocked() const noexcept
{
    return IsContextMenuFeaturesUnlocked_;
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

const std::string& AppConfig::getUrlFilesGet() const noexcept
{
    return urlFilesGet_;
}

const std::string& AppConfig::getUrlAgentProfile() const noexcept
{
    return urlAgentProfile_;
}

const std::string& AppConfig::getUrlAuthMailRu() const noexcept
{
    return urlAuthMailRu_;
}

const std::string& AppConfig::getUrlRMailRu() const noexcept
{
    return urlRMailRu_;
}

const std::string& AppConfig::getUrlWinMailRu() const noexcept
{
    return urlWinMailRu_;
}

const std::string& AppConfig::getUrlReadMsg() const noexcept
{
    return urlReadMsg_;
}

const std::string& AppConfig::getUrlCICQOrg() const noexcept
{
    return urlCICQOrg_;
}

const std::string& AppConfig::getUrlCICQCom() const noexcept
{
    return urlCICQCom_;
}

int AppConfig::CacheHistoryControlPagesFor() const noexcept
{
    return CacheHistoryContolPagesFor_;
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

const AppConfig& GetAppConfig()
{
    assert(AppConfig_);

    return *AppConfig_;
}

void SetAppConfig(AppConfigUptr&& appConfig)
{
    assert(appConfig);

    if (AppConfig_)
    {
        if (AppConfig_->IsShowMsgIdsEnabled() != appConfig->IsShowMsgIdsEnabled())
        {
            appConfig->SetShowMsgOptionHasChanged(true);
        }
    }

    AppConfig_ = std::move(appConfig);
}

void ModifyAppConfig(AppConfig _appConfig, message_processed_callback _callback, QObject *_qobj, bool postToCore)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_bool("fulllog", _appConfig.IsFullLogEnabled());
    collection.set_value_as_bool("updateble", _appConfig.IsUpdateble());
    collection.set_value_as_bool("dev.show_message_ids", _appConfig.IsShowMsgIdsEnabled());
    collection.set_value_as_bool("dev.save_rtp_dumps", _appConfig.IsSaveCallRTPEnabled());
    collection.set_value_as_bool("dev.unlock_context_menu_features", _appConfig.IsContextMenuFeaturesUnlocked());
    collection.set_value_as_bool("gdpr.user_has_logged_in_ever", _appConfig.GDPR_UserHasLoggedInEver());
    collection.set_value_as_int("dev.cache_history_pages_secs", _appConfig.CacheHistoryControlPagesFor());
    collection.set_value_as_bool("dev.server_search", _appConfig.IsServerSearchEnabled());

    if (!postToCore)
    {
        SetAppConfig(std::make_unique<AppConfig>(std::move(_appConfig)));
        return;
    }

    GetDispatcher()->post_message_to_core("change_app_config", collection.get(), _qobj,
        [_appConfig{ std::move(_appConfig) }, _callback{ std::move(_callback) }](core::icollection* _coll) {
            Ui::gui_coll_helper coll(_coll, false);
            auto err = coll.get_value_as_int("error");
            Q_UNUSED(err);

            SetAppConfig(std::make_unique<AppConfig>(std::move(_appConfig)));

            if (_callback)
                _callback(_coll);
    });
}

UI_NS_END
