#include "stdafx.h"
#include "features.h"
#include "my_info.h"
#include "translator.h"

#include "omicron/omicron_helper.h"
#include "../gui_settings.h"
#include "spellcheck/Spellchecker.h"

#include "../app_config.h"
#include "../url_config.h"
#include "../common.shared/config/config.h"
#include "../common.shared/omicron_keys.h"
#include "cache/emoji/EmojiCode.h"
#include "opengl.h"

namespace
{
    constexpr int64_t kMinimalThreadsForbidSupportVersion = 80;
    constexpr int64_t kMinimalThreadsSupportedApiVersion = 71;

    bool myteamConfigOrOmicronFeatureEnabled(config::features _feature, const char* _omicron_key)
    {
        const auto config_value = config::get().is_on(_feature);
        const auto omicron_value = Omicron::_o(_omicron_key, config_value);
        return config::is_overridden(_feature) ? config_value || omicron_value : omicron_value;
    }
}

namespace Features
{

    bool isNicksEnabled()
    {
        return Omicron::_o(omicron::keys::profile_nickname_allowed, feature::default_profile_nickname_allowed());
    }

    QString getProfileDomain()
    {
        const QString defaultDomain = []() -> QString {
            if (config::get().is_on(config::features::external_url_config))
                return Ui::getUrlConfig().getUrlProfile();

            const std::string_view link = config::get().url(config::urls::profile);
            return QString::fromUtf8(link.data(), link.size());
        }();
        return Omicron::_o(omicron::keys::profile_domain, defaultDomain);
    }

    QString getProfileDomainAgent()
    {
        const QString defaultDomain = []() -> QString {
            if (config::get().is_on(config::features::external_url_config))
                return Ui::getUrlConfig().getUrlProfile();

            const std::string_view link = config::get().url(config::urls::profile_agent);
            return QString::fromUtf8(link.data(), link.size());
        }();
        return Omicron::_o(omicron::keys::profile_domain_agent, defaultDomain);
    }

    size_t maximumUndoStackSize()
    {
        return Omicron::_o(omicron::keys::maximum_undo_size, 20);
    }

    bool useAppleEmoji()
    {
        return Omicron::_o(omicron::keys::use_apple_emoji, feature::default_use_apple_emoji());
    }

    bool opensOnClick()
    {
        return Omicron::_o(omicron::keys::open_file_on_click, feature::default_open_file_on_click());
    }

    bool phoneAllowed()
    {
        const auto defaultValue = config::get().is_on(config::features::phone_allowed);
        const auto omicronValue = Omicron::_o(omicron::keys::phone_allowed, defaultValue);
        return defaultValue && omicronValue;
    }

    bool externalPhoneAttachment()
    {
        return Omicron::_o(omicron::keys::external_phone_attachment, config::get().is_on(config::features::external_phone_attachment));
    }

    bool hideMessageInfoEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::hiding_message_info_enabled, omicron::keys::hiding_message_info_enabled);
    }

    bool isPttRecognitionEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::ptt_recognition, omicron::keys::ptt_recognition_enabled);
    }

    bool hideMessageTextEnabled()
    {
        if (hideMessageInfoEnabled())
            return true;
        return  myteamConfigOrOmicronFeatureEnabled(config::features::hiding_message_text_enabled, omicron::keys::hiding_message_text_enabled);
    }

    QString dataVisibilityLink()
    {
        const static QString link = []() -> QString {
            const std::string_view link = config::get().url(config::urls::data_visibility);
            return u"https://" % QString::fromUtf8(link.data(), link.size()) % u'/';
        }();

        return Omicron::_o(omicron::keys::data_visibility_link, link);
    }

    QString passwordRecoveryLink()
    {
        const static QString link = []() -> QString {
            const std::string_view link = config::get().url(config::urls::password_recovery);
            return u"https://" % QString::fromUtf8(link.data(), link.size()) % u'/';
        }();

        return Omicron::_o(omicron::keys::password_recovery_link, link);
    }

    QString securityCallLink()
    {
        constexpr std::string_view security_call_link = feature::default_security_call_link();
        const static auto link = QString::fromUtf8(security_call_link.data(), security_call_link.size());

        return Omicron::_o(omicron::keys::security_call_link, link);
    }

    QString attachPhoneUrl()
    {
        const static QString link = []() -> QString {
            const std::string_view link = config::get().url(config::urls::attach_phone);
            return u"https://" % QString::fromUtf8(link.data(), link.size()) % u'/';
        }();

        return Omicron::_o("attach_phone_url", link);
    }

    QString updateAppUrl()
    {
        return Omicron::_o("update_app_url", QString());
    }

    LoginMethod loginMethod()
    {
        if (config::get().is_on(config::features::otp_login))
            return LoginMethod::OTPViaEmail;
        return LoginMethod::Basic;
    }

    bool isOAuth2LoginAllowed()
    {
        if (!hasWebEngine() || !myteamConfigOrOmicronFeatureEnabled(config::features::login_by_oauth2_allowed, omicron::keys::login_by_oauth2_allowed))
            return false;

        return !Ui::getUrlConfig().getAuthUrl().isEmpty();
    }

    QString getOAuthScope()
    {
        const auto scope = config::get().string(config::values::oauth_scope);
        const auto config_value = QString::fromUtf8(scope.data(), scope.size());
        const auto omicron_value = Omicron::_o(omicron::keys::oauth_scope, config_value);
        return (config::is_overridden(config::values::oauth_type) && omicron_value.isEmpty()) ? config_value : omicron_value;
    }

    QString getOAuthType()
    {
        const auto oauth = config::get().string(config::values::oauth_type);
        return QString::fromUtf8(oauth.data(), oauth.size());
    }

    QString getClientId()
    {
        const auto sv = config::get().string(config::values::client_id);
        const auto config_value = QString::fromUtf8(sv.data(), sv.size());
        const auto omicron_value = Omicron::_o(omicron::keys::client_id, config_value);
        return (config::is_overridden(config::values::client_id) && omicron_value.isEmpty()) ? config_value : omicron_value;
    }

    size_t getSMSResultTime()
    {
        return Omicron::_o(omicron::keys::sms_result_waiting_time, 60);
    }

    bool showAttachPhoneNumberPopup()
    {
        const bool value = config::get().is_on(config::features::show_attach_phone_number_popup);

        return Omicron::_o(omicron::keys::show_attach_phone_number_popup, value);
    }

    bool closeBtnAttachPhoneNumberPopup()
    {
        const static bool isRuLang = Utils::GetTranslator()->getLang().toLower() == u"ru";
        const static bool value = !(config::get().is_on(config::features::attach_phone_number_popup_modal) && isRuLang);

        return Omicron::_o(omicron::keys::attach_phone_number_popup_close_btn, value);
    }

    int showTimeoutAttachPhoneNumberPopup()
    {
        const static bool isRuLang = Utils::GetTranslator()->getLang().toLower() == u"ru";
        const static int value = (config::get().is_on(config::features::attach_phone_number_popup_modal) && isRuLang) ? 0 : feature::default_show_timeout_attach_phone_number_popup();

        return Omicron::_o(omicron::keys::attach_phone_number_popup_show_timeout, value);
    }

    QString textAttachPhoneNumberPopup()
    {
        const static auto text = QT_TRANSLATE_NOOP("attach_phone_number", "For security reasons you need to confirm your phone number. This number will not appear on your profile.");

        return Omicron::_o(omicron::keys::attach_phone_number_popup_text, text);
    }

    QString titleAttachPhoneNumberPopup()
    {
        const static auto title = QT_TRANSLATE_NOOP("attach_phone_number", "Security alert");

        return Omicron::_o(omicron::keys::attach_phone_number_popup_title, title);
    }

    bool isSmartreplyEnabled()
    {
        const auto value = config::get().is_on(config::features::smartreply_suggests_feature_enabled);
        return config::get().is_on(config::features::smartreplies) && Omicron::_o(omicron::keys::smartreply_suggests_feature_enabled, value);
    }

    bool isSmartreplyForQuoteEnabled()
    {
        const auto value = config::get().is_on(config::features::smartreply_suggests_for_quotes);
        return isSmartreplyEnabled() && Omicron::_o(omicron::keys::smartreply_suggests_for_quotes, value);
    }

    std::chrono::milliseconds smartreplyHideTime()
    {
        const auto value = config::get().number<int64_t>(config::values::smartreply_suggests_click_hide_timeout);
        return std::chrono::milliseconds(Omicron::_o(omicron::keys::smartreply_suggests_click_hide_timeout, value.value_or(feature::default_smartreply_suggests_click_hide_timeout())));
    }

    int smartreplyMsgidCacheSize()
    {
        const auto value = config::get().number<int64_t>(config::values::smartreply_suggests_msgid_cache_size);
        return Omicron::_o(omicron::keys::smartreply_suggests_msgid_cache_size, value.value_or(feature::default_smartreply_suggests_msgid_cache_size()));
    }

    bool isSuggestsEnabled()
    {
        return config::get().is_on(config::features::sticker_suggests);
    }

    std::chrono::milliseconds getSuggestTimerInterval()
    {
        const int value = Omicron::_o(omicron::keys::server_suggests_graceful_time, feature::default_stickers_suggest_time_interval());
        return std::chrono::milliseconds(value);
    }

    bool isServerSuggestsEnabled()
    {
        return isSuggestsEnabled() && config::get().is_on(config::features::sticker_suggests_server) && Omicron::_o(omicron::keys::server_suggests_enabled, feature::default_stickers_server_suggest_enabled());
    }

    int maxAllowedSuggestChars()
    {
        return Omicron::_o(omicron::keys::server_suggests_max_allowed_chars, feature::default_stickers_server_suggests_max_allowed_chars());
    }

    int maxAllowedSuggestWords()
    {
        return Omicron::_o(omicron::keys::server_suggests_max_allowed_words, feature::default_stickers_server_suggests_max_allowed_words());
    }

    int maxAllowedLocalSuggestChars()
    {
        return Omicron::_o(omicron::keys::suggests_max_allowed_chars, feature::default_stickers_local_suggests_max_allowed_chars());
    }

    int maxAllowedLocalSuggestWords()
    {
        return Omicron::_o(omicron::keys::suggests_max_allowed_words, feature::default_stickers_local_suggests_max_allowed_words());
    }

    bool betaUpdateAllowed()
    {
        return !environment::is_alpha() && updateAllowed() && Omicron::_o(omicron::keys::beta_update, config::get().is_on(config::features::beta_update));
    }

    bool updateAllowed()
    {
        return !build::is_store() && !build::is_pkg_msi();
    }

    bool avatarChangeAllowed()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::avatar_change_allowed, omicron::keys::avatar_change_allowed);
    }

    bool clRemoveContactsAllowed()
    {
        return Omicron::_o(omicron::keys::cl_remove_contacts_allowed, config::get().is_on(config::features::remove_contact));
    }

    bool changeNameAllowed()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::changeable_name, omicron::keys::changeable_name);
    }

    bool changeContactNamesAllowed()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::allow_contacts_rename, omicron::keys::allow_contacts_rename);
    }

    bool changeInfoAllowed()
    {
        return config::get().is_on(config::features::info_change_allowed);
    }

    bool pollsEnabled()
    {
        return Omicron::_o(omicron::keys::polls_enabled, feature::default_polls_enabled());
    }

    int getFsIDLength()
    {
        return Omicron::_o(omicron::keys::fs_id_length, feature::default_fs_id_length());
    }

    bool isSpellCheckEnabled()
    {
        return spellcheck::Spellchecker::isAvailable() && Omicron::_o(omicron::keys::spell_check_enabled, config::get().is_on(config::features::spell_check));
    }

    size_t spellCheckMaxSuggestCount()
    {
        return Omicron::_o(omicron::keys::spell_check_max_suggest_count, 3);
    }

    QString favoritesImageIdEnglish()
    {
        constexpr std::string_view id = feature::default_favorites_image_id_english();
        return Omicron::_o(omicron::keys::favorites_image_id_english, QString::fromUtf8(id.data(), id.size()));
    }

    QString favoritesImageIdRussian()
    {
        constexpr std::string_view id = feature::default_favorites_image_id_russian();
        return Omicron::_o(omicron::keys::favorites_image_id_russian, QString::fromUtf8(id.data(), id.size()));
    }

    int getAsyncResponseTimeout()
    {
        return Omicron::_o(omicron::keys::async_response_timeout, feature::default_async_response_timeout());
    }

    int getVoipCallUserLimit()
    {
        static const auto default_value = config::get().number<int64_t>(config::values::voip_call_user_limit).value_or(feature::default_voip_call_user_limit());
        return Omicron::_o(omicron::keys::voip_call_user_limit, default_value);
    }

    int getVoipVideoUserLimit()
    {
        static const auto default_value = config::get().number<int64_t>(config::values::voip_video_user_limit).value_or(feature::default_voip_video_user_limit());
        return Omicron::_o(omicron::keys::voip_video_user_limit, default_value);
    }

    int getVoipBigConferenceBoundary()
    {
        static const auto default_value = config::get().number<int64_t>(config::values::voip_big_conference_boundary).value_or(feature::default_voip_big_conference_boundary());
        return Omicron::_o(omicron::keys::voip_big_conference_boundary, default_value);
    }

    bool isVcsCallByLinkEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::vcs_call_by_link_enabled, omicron::keys::vcs_call_by_link_enabled) || isVcsCallByLinkV2Enabled();
    }

    bool isVcsCallByLinkV2Enabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::call_link_v2_enabled, omicron::keys::call_link_v2_enabled);
    }

    bool isVcsWebinarEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::vcs_webinar_enabled, omicron::keys::vcs_webinar_enabled);
    }

    QString getVcsRoomList()
    {
        const static auto default_value = []() -> QString {
            const auto value = config::get().url(config::urls::vcs_room);
            return QString::fromUtf8(value.data(), value.size());
        }();

        return Omicron::_o(omicron::keys::vcs_room, default_value);
    }

    std::string getReactionsJson()
    {
        return Omicron::_o_json(omicron::keys::reactions_initial_set, std::string());
    }

    bool reactionsEnabled()
    {
        return Omicron::_o(omicron::keys::show_reactions, config::get().is_on(config::features::show_reactions));
    }

    std::string getStatusJson()
    {
        return Omicron::_o_json(omicron::keys::statuses_json, std::string());
    }

    bool isStatusEnabled()
    {
        return Omicron::_o(omicron::keys::statuses_enabled, config::get().is_on(config::features::statuses_enabled));
    }

    bool isCustomStatusEnabled()
    {
        return Omicron::_o(omicron::keys::custom_statuses_enabled, config::get().is_on(config::features::custom_statuses_enabled));
    }

    bool isGlobalContactSearchAllowed()
    {
        return config::get().is_on(config::features::global_contact_search_allowed);
    }

    bool forceCheckMacUpdates()
    {
        return Omicron::_o(omicron::keys::force_update_check_allowed, config::get().is_on(config::features::force_update_check_allowed));
    }

    bool callRoomInfoEnabled()
    {
        return Omicron::_o(omicron::keys::call_room_info_enabled, config::get().is_on(config::features::call_room_info_enabled));
    }

    bool areYourInvitesButtonVisible()
    {
        return Omicron::_o(omicron::keys::show_your_invites_to_group_enabled, config::get().is_on(config::features::show_your_invites_to_group_enabled));
    }

    bool isGroupInvitesBlacklistEnabled()
    {
        return Omicron::_o(omicron::keys::group_invite_blacklist_enabled, config::get().is_on(config::features::group_invite_blacklist_enabled));
    }

    bool IvrLoginEnabled()
    {
        return Omicron::_o(omicron::keys::ivr_login_enabled, false);
    }

    int IvrResendCountToShow()
    {
        return Omicron::_o(omicron::keys::ivr_resend_count_to_show, 2);
    }

    bool isContactUsViaBackend()
    {
        return config::get().is_on(config::features::contact_us_via_backend);
    }

    bool isUpdateFromBackendEnabled()
    {
        return config::get().is_on(config::features::update_from_backend);
    }

    bool isWebpScreenshotEnabled()
    {
        return Omicron::_o(omicron::keys::webp_screenshot_enabled, false);
    }

    qint64 maxFileSizeForWebpConvert()
    {
        return Omicron::_o(omicron::keys::webp_max_file_size_to_convert, int64_t(1.5 * 1024 * 1024));
    }

    bool isInviteBySmsEnabled()
    {
        return Omicron::_o(omicron::keys::invite_by_sms, config::get().is_on(config::features::invite_by_sms));
    }

    bool shouldShowSmsNotifySetting()
    {
        return Omicron::_o(omicron::keys::show_sms_notify_setting, config::get().is_on(config::features::show_sms_notify_setting));
    }

    bool isAnimatedStickersInPickerAllowed()
    {
        const auto disabled = Omicron::_o(omicron::keys::animated_stickers_in_picker_disabled, false) || !isOpenGLSupported();
        return !disabled;
    }

    bool isAnimatedStickersInChatAllowed()
    {
        const auto disabled = Omicron::_o(omicron::keys::animated_stickers_in_chat_disabled, false) || !isOpenGLSupported();
        return !disabled;
    }

    bool isContactListSmoothScrollingEnabled()
    {
        return Omicron::_o(omicron::keys::contact_list_smooth_scrolling_enabled, true);
    }

    bool isBackgroundPttPlayEnabled()
    {
        return Omicron::_o(omicron::keys::background_ptt_play_enabled, true);
    }

    bool removeDeletedFromNotifications()
    {
        return Omicron::_o(omicron::keys::remove_deleted_from_notifications, config::get().is_on(config::features::remove_deleted_from_notifications));
    }

    bool longPathTooltipsAllowed()
    {
        return Omicron::_o(omicron::keys::long_path_tooltip_enabled, config::get().is_on(config::features::long_path_tooltip_enabled));
    }

    bool isFormattingInBubblesEnabled()
    {
        return Omicron::_o(omicron::keys::formatting_in_bubbles, config::get().is_on(config::features::formatting_in_bubbles));
    }

    bool isFormattingInInputEnabled()
    {
        return Omicron::_o(omicron::keys::formatting_in_input, config::get().is_on(config::features::formatting_in_input));
    }

    bool isBlockFormattingInInputEnabled()
    {
        return isFormattingInInputEnabled();
    }

    QString statusBannerEmojis()
    {
        std::string csv(config::get().string(config::values::status_banner_emoji_csv));
        csv = config::is_overridden(config::values::status_banner_emoji_csv) ? csv : Omicron::_o(omicron::keys::status_banner_emoji_csv, csv);
        return QString::fromUtf8(csv.data(), csv.size());
    }

    bool isAppsNavigationBarVisible()
    {
        return config::get().is_on(config::features::apps_bar_enabled);
    }

    bool isMessengerTabBarVisible()
    {
        return !isAppsNavigationBarVisible();
    }

    bool isStatusInAppsNavigationBar()
    {
        return isAppsNavigationBarVisible() && config::get().is_on(config::features::status_in_apps_bar);
    }

    bool isRecentsPinnedItemsEnabled()
    {
        return isThreadsEnabled() || isScheduledMessagesEnabled() || isRemindersEnabled();
    }

    bool isScheduledMessagesEnabled()
    {
        return config::get().is_on(config::features::scheduled_messages_enabled);
    }

    bool isThreadsForbidEnabled()
    {
        const auto version = config::get().number<int64_t>(config::values::server_api_version).value_or(kMinimalThreadsSupportedApiVersion);
        return isThreadsEnabled() && version >= kMinimalThreadsForbidSupportVersion;
    }

    bool isThreadsEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::threads_enabled, omicron::keys::threads_enabled);
    }

    bool isRemindersEnabled()
    {
        return config::get().is_on(config::features::reminders_enabled);
    }

    bool isSharedFederationStickerpacksSupported()
    {
        return config::get().is_on(config::features::support_shared_federation_stickerpacks);
    }

    std::chrono::milliseconds threadResubscribeTimeout()
    {
        im_assert(isThreadsEnabled());

        constexpr std::chrono::seconds def = std::chrono::minutes(1);
        return std::chrono::seconds(Omicron::_o(omicron::keys::subscr_renew_interval_thread, def.count()));
    }

    bool isDraftEnabled()
    {
        return Omicron::_o(omicron::keys::draft_enabled, config::get().is_on(config::features::draft_enabled));
    }

    std::chrono::seconds draftTimeout()
    {
        const auto default_value = config::get().number<int64_t>(config::values::draft_timeout_sec).value_or(feature::default_draft_timeout_sec());
        return std::chrono::seconds(Omicron::_o(omicron::keys::draft_timeout, default_value));
    }

    int draftMaximumLength()
    {
        const auto default_value = config::get().number<int64_t>(config::values::draft_max_len).value_or(feature::default_draft_max_len());
        return Omicron::_o(omicron::keys::draft_max_len, default_value);
    }

    bool isMessageCornerMenuEnabled()
    {
        return config::get().is_on(config::features::message_corner_menu);
    }

    bool isTaskCreationInChatEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::task_creation_in_chat_enabled, omicron::keys::task_creation_in_chat_enabled);
    }

    bool isRestrictedFilesEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::restricted_files_enabled, omicron::keys::restricted_files_enabled);
    }

    bool trustedStatusDefault()
    {
        return !isRestrictedFilesEnabled();
    }

    bool isAntivirusCheckEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::antivirus_check_enabled, omicron::keys::antivirus_check_enabled);
    }

    bool isAntivirusCheckProgressVisible()
    {
        return Features::isAntivirusCheckEnabled() && config::get().is_on(config::features::antivirus_check_progress_visible);
    }

    bool isExpandedGalleryEnabled()
    {
        return Omicron::_o(omicron::keys::expanded_gallery_enabled, config::get().is_on(config::features::expanded_gallery));
    }

    bool hasWebEngine()
    {
        if (!build::has_webengine())
            return false;

#ifdef BUILD_FOR_STORE
        if (QOperatingSystemVersion::current() <= QOperatingSystemVersion::MacOSHighSierra)
            return false;
#endif //BUILD_FOR_STORE

        return true;
    }

    std::chrono::seconds webPageReloadInterval()
    {
        const auto defaultValue = config::get().number<int64_t>(config::values::base_retry_interval_sec);
        const auto omicronValue = Omicron::_o(omicron::keys::base_retry_interval_sec, defaultValue.value_or(feature::default_base_retry_interval_sec()));
        return std::chrono::seconds(omicronValue);
    }

    QStringList getBotCommandsDisabledChats()
    {
        QStringList result;
        const auto comma = ql1c(',');

        std::string configValues(config::get().string(config::values::bots_commands_disabled));
        if (!configValues.empty())
            result.append(QString::fromStdString(configValues).split(comma));
        auto omicronValues = Omicron::_o(omicron::keys::bots_commands_disabled, qsl());
        if (!omicronValues.isEmpty())
            result.append(omicronValues.split(comma));

        result.removeDuplicates();
        return result;
    }

    bool isDeleteAccountViaAdmin()
    {
        return config::get().is_on(config::features::delete_account_via_admin);
    }

    bool isDeleteAccountEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::delete_account_enabled, omicron::keys::delete_account_enabled);
    }

    QString deleteAccountUrlEmail()
    {
        if (!isDeleteAccountEnabled())
            return QString();

        const static auto default_value = []() -> QString {
            const auto value = config::get().url(config::urls::delete_account_url_email);
            return QString::fromUtf8(value.data(), value.size());
        }();

        return Omicron::_o(omicron::keys::delete_account_url_email, default_value);
    }

    QString deleteAccountUrl(const QString& _aimId)
    {
        if (!isDeleteAccountEnabled())
            return QString();

        if (!Utils::isUin(_aimId))
            return deleteAccountUrlEmail();

        const static auto default_value = []() -> QString {
            const auto value = config::get().url(config::urls::delete_account_url);
            return QString::fromUtf8(value.data(), value.size());
        }();

        return Omicron::_o(omicron::keys::delete_account_url, default_value);
    }

    bool hasRegistryAbout()
    {
        return config::get().is_on(config::features::has_registry_about);
    }

    QString getServiceAppsOrder()
    {
        const QString defaultApps = []() -> QString
        {
            const std::string_view link = config::get().string(config::values::service_apps_order);
            return QString::fromUtf8(link.data(), link.size());
        }();

        return Omicron::_o_json(omicron::keys::service_apps_order, defaultApps);
    }

    std::string getServiceAppsJson()
    {
        return config::get().string(config::values::service_apps_config).data();
    }

    std::string getServiceAppsDesktopJson()
    {
        return config::get().string(config::values::service_apps_desktop).data();
    }

    std::string getCustomAppsJson()
    {
        return config::get().string(config::values::custom_miniapps).data();
    }

    // TODO: remove when deprecated in server configs
    [[deprecated("Use Logic::GetAppsContainer()->isAppEnabled(Utils::MiniApps::getOrgstructureId())")]]
    bool isContactsEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::organization_structure_enabled, omicron::keys::organization_structure_enabled);
    }

    [[deprecated("Use Logic::GetAppsContainer()->isAppEnabled(Utils::MiniApps::getTasksId())")]]
    bool isTasksEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::tasks_enabled, omicron::keys::tasks_enabled);
    }

    [[deprecated("Use Logic::GetAppsContainer()->isAppEnabled(Utils::MiniApps::getCalendarId())")]]
    bool isCalendarEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::calendar_enabled, omicron::keys::calendar_enabled);
    }

    [[deprecated("Use Logic::GetAppsContainer()->isAppEnabled(Utils::MiniApps::getMailId())")]]
    bool isMailEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::mail_enabled, omicron::keys::mail_enabled);
    }

    [[deprecated("Use Logic::GetAppsContainer()->isAppEnabled(Utils::MiniApps::getTarmMailId())")]]
    bool isTarmMailEnabled()
    {
        return config::get().is_on(config::features::tarm_mail);
    }

    [[deprecated("Use Logic::GetAppsContainer()->isAppEnabled(Utils::MiniApps::getTarmCallsId())")]]
    bool isTarmCallsEnabled()
    {
        return config::get().is_on(config::features::tarm_calls);
    }

    [[deprecated("Use Logic::GetAppsContainer()->isAppEnabled(Utils::MiniApps::getTarmCloudId())")]]
    bool isTarmCloudEnabled()
    {
        return config::get().is_on(config::features::tarm_cloud);
    }

    bool calendarSelfAuth()
    {
        return config::get().is_on(config::features::calendar_self_auth);
    }

    bool mailSelfAuth()
    {
        return config::get().is_on(config::features::mail_self_auth);
    }

    bool cloudSelfAuth()
    {
        return config::get().is_on(config::features::cloud_self_auth);
    }

    QString getDigitalAssistantAimid()
    {
        const std::string configValues(config::get().string(config::values::digital_assistant_bot_aimid));
        if (!configValues.empty())
            return QString::fromStdString(configValues);

        return Omicron::_o(omicron::keys::digital_assistant_bot_aimid, qsl());

    }

    bool isDigitalAssistantPinEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::digital_assistant_search_positioning, omicron::keys::digital_assistant_search_positioning);
    }

    QStringList getAdditionalTheme()
    {
        const std::string configValues(config::get().string(config::values::additional_theme));
        if (!configValues.empty())
            return QString::fromStdString(configValues).split(qsl(","));

        auto tmp = Omicron::_o(omicron::keys::additional_theme, qsl());
        return (tmp == qsl("")) ? QStringList() : tmp.split(qsl(","));
    }

    bool leadingLastName()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::leading_last_name, omicron::keys::delete_account_enabled);
    }

    bool isReportMessagesEnabled()
    {
        return myteamConfigOrOmicronFeatureEnabled(config::features::report_messages_enabled, omicron::keys::report_messages_enabled);
    }
}
