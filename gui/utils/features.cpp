#include "stdafx.h"
#include "features.h"
#include "my_info.h"
#include "translator.h"

#include "omicron/omicron_helper.h"
#include "../gui_settings.h"

#include "../app_config.h"
#include "../url_config.h"
#include "../common.shared/config/config.h"
#include "../common.shared/omicron_keys.h"
#include "../common.shared/smartreply/smartreply_config.h"

namespace Features
{
    bool isNicksEnabled()
    {
        return Omicron::_o(omicron::keys::profile_nickname_allowed, feature::default_profile_nickname_allowed());
    }

    QString getProfileDomain()
    {
        const static QString defaultDomain = []() -> QString {
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

    bool forceShowChatPopup()
    {
        return Omicron::_o(omicron::keys::force_show_chat_popup, config::get().is_on(config::features::force_show_chat_popup_default));
    }

    bool phoneAllowed()
    {
        return Omicron::_o(omicron::keys::phone_allowed, config::get().is_on(config::features::phone_allowed));
    }

    bool externalPhoneAttachment()
    {
        return Omicron::_o("external_phone_attachment", config::get().is_on(config::features::external_phone_attachment));
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
            return ql1s("https://") % QString::fromUtf8(link.data(), link.size()) % ql1c('/');
        }();

        return Omicron::_o(omicron::keys::data_visibility_link, link);
    }

    QString passwordRecoveryLink()
    {
        const static QString link = []() -> QString {
            const std::string_view link = config::get().url(config::urls::password_recovery);
            return ql1s("https://") % QString::fromUtf8(link.data(), link.size()) % ql1c('/');
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
            return ql1s("https://") % QString::fromUtf8(link.data(), link.size()) % ql1c('/');
        }();

        return Omicron::_o("attach_phone_url", link);
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
        const static bool isRuLang = Utils::GetTranslator()->getLang().toLower() == ql1s("ru");
        const static bool value = !(config::get().is_on(config::features::attach_phone_number_popup_modal) && isRuLang);

        return Omicron::_o(omicron::keys::attach_phone_number_popup_close_btn, value);
    }

    int showTimeoutAttachPhoneNumberPopup()
    {
        const static bool isRuLang = Utils::GetTranslator()->getLang().toLower() == ql1s("ru");
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
        return !build::is_store();
    }

    bool avatarChangeAllowed()
    {
        return Omicron::_o(omicron::keys::avatar_change_allowed, config::get().is_on(config::features::avatar_change_allowed));
    }

    bool clRemoveContactsAllowed()
    {
        return Omicron::_o(omicron::keys::cl_remove_contacts_allowed, config::get().is_on(config::features::remove_contact));
    }

    bool changeNameAvailable()
    {
        return Omicron::_o(omicron::keys::changeable_name, config::get().is_on(config::features::changeable_name));
    }

    bool pollsEnabled()
    {
        return Omicron::_o(omicron::keys::polls_enabled, feature::default_polls_enabled());
    }

    int getFsIDLength()
    {
        return Omicron::_o(omicron::keys::fs_id_length, feature::default_fs_id_length());
    }
}
