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
#include "../common.shared/smartreply/smartreply_config.h"
#include "cache/emoji/EmojiCode.h"

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
        if (config::get().is_on(config::features::store_version))
            return defaultValue && omicronValue;
        return omicronValue;
    }

    bool externalPhoneAttachment()
    {
        return Omicron::_o(omicron::keys::external_phone_attachment, config::get().is_on(config::features::external_phone_attachment));
    }

    bool showNotificationsTextSettings()
    {
        return Omicron::_o(omicron::keys::show_notification_text, config::get().is_on(config::features::show_notification_text));
    }

    bool showNotificationsText()
    {
        if (showNotificationsTextSettings())
            return !(Ui::get_gui_settings()->get_value<bool>(settings_hide_message_notification, false));
        return false;
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
        return config::get().is_on(config::features::smartreplies) && Omicron::_o(feature::smartreply::is_enabled().name(), feature::smartreply::is_enabled().def<bool>());
    }

    bool isSmartreplyForQuoteEnabled()
    {
        return isSmartreplyEnabled() && Omicron::_o(feature::smartreply::is_enabled_for_quotes().name(), feature::smartreply::is_enabled_for_quotes().def<bool>());
    }

    std::chrono::milliseconds smartreplyHideTime()
    {
        return std::chrono::milliseconds(Omicron::_o(feature::smartreply::click_hide_timeout().name(), feature::smartreply::click_hide_timeout().def<int>()));
    }

    int smartreplyMsgidCacheSize()
    {
        return Omicron::_o(feature::smartreply::msgid_cache_size().name(), feature::smartreply::msgid_cache_size().def<int>());
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
        auto value = config::get().is_on(config::features::avatar_change_allowed);
        value = config::is_overridden(config::features::avatar_change_allowed) ? value : Omicron::_o(omicron::keys::avatar_change_allowed, value);
        return value;
    }

    bool clRemoveContactsAllowed()
    {
        return Omicron::_o(omicron::keys::cl_remove_contacts_allowed, config::get().is_on(config::features::remove_contact));
    }

    bool changeNameAllowed()
    {
        auto value = config::get().is_on(config::features::changeable_name);
        value = config::is_overridden(config::features::changeable_name) ? value : Omicron::_o(omicron::keys::changeable_name, value);
        return value;
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
        auto value = config::get().is_on(config::features::vcs_call_by_link_enabled);
        value = config::is_overridden(config::features::vcs_call_by_link_enabled) ? value : Omicron::_o(omicron::keys::vcs_call_by_link_enabled, value);
        return value;
    }

    bool isVcsWebinarEnabled()
    {
        auto value = config::get().is_on(config::features::vcs_webinar_enabled);
        value = config::is_overridden(config::features::vcs_webinar_enabled) ? value : Omicron::_o(omicron::keys::vcs_webinar_enabled, value);
        return value;
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

    bool isYourInvitesButtonVisible()
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
        const auto disabled = Omicron::_o(omicron::keys::animated_stickers_in_picker_disabled, false);
        return !disabled;
    }

    bool isAnimatedStickersInChatAllowed()
    {
        const auto disabled = Omicron::_o(omicron::keys::animated_stickers_in_chat_disabled, false);
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
}
