#include "stdafx.h"
#include "features.h"

#include "omicron/omicron_helper.h"

#include "../app_config.h"

namespace Features
{
    bool isNicksEnabled()
    {
        return Omicron::_o("profile_nickname_allowed", feature::default_profile_nickname_allowed());
    }

    QString getProfileDomain()
    {
        constexpr std::string_view domain = feature::default_profile_domain();
        const static auto defaultDomain = QString::fromUtf8(domain.data(), domain.size());
        return Omicron::_o("profile_domain", defaultDomain);
    }

    QString getProfileDomainAgent()
    {
        if (build::is_biz())
        {
            const static std::string domain = feature::default_profile_myteam_domain();
            const static auto defaultDomain = QString::fromUtf8(domain.data(), domain.size());
            return Omicron::_o("profile_domain_myteam", defaultDomain);
        }

        const static std::string domain = Ui::GetAppConfig().getUrlAgentProfile();
        const static auto defaultDomain = QString::fromUtf8(domain.data(), domain.size());
        return Omicron::_o("profile_domain_agent", defaultDomain);
    }

    size_t maximumUndoStackSize()
    {
        return Omicron::_o("maximum_undo_size", 20);
    }

    bool useAppleEmoji()
    {
        return Omicron::_o("use_apple_emoji", feature::default_use_apple_emoji());
    }

    bool opensOnClick()
    {
        return Omicron::_o("open_file_on_click", feature::default_open_file_on_click());
    }

    bool forceShowChatPopup()
    {
        return Omicron::_o("force_show_chat_popup", feature::default_force_show_chat_popup());
    }

    bool bizPhoneAllowed()
    {
        return Omicron::_o("biz_phone_allowed", feature::default_biz_phone_allowed());
    }

    QString dataVisibilityLink()
    {
        constexpr std::string_view data_visibility_link_icq = feature::default_data_visibility_link_icq();
        const static auto link_icq = QString::fromUtf8(data_visibility_link_icq.data(), data_visibility_link_icq.size());
        constexpr std::string_view data_visibility_link_agent = feature::default_data_visibility_link_agent();
        const static auto link_agent = QString::fromUtf8(data_visibility_link_agent.data(), data_visibility_link_agent.size());

        return Omicron::_o("data_visibility_link", build::is_icq() ? link_icq : link_agent);
    }

    QString passwordRecoveryLink()
    {
        constexpr std::string_view password_recovery_link_icq = feature::default_password_recovery_link_icq();
        const static auto link_icq = QString::fromUtf8(password_recovery_link_icq.data(), password_recovery_link_icq.size());
        constexpr std::string_view password_recovery_link_agent = feature::default_password_recovery_link_agent();
        const static auto link_agent = QString::fromUtf8(password_recovery_link_agent.data(), password_recovery_link_agent.size());

        return Omicron::_o("data_visibility_link", build::is_icq() ? link_icq : link_agent);
    }

    QString securityCallLink()
    {
        constexpr std::string_view security_call_link = feature::default_security_call_link();
        const static auto link = QString::fromUtf8(security_call_link.data(), security_call_link.size());

        return Omicron::_o("security_call_link", link);
    }

    LoginMethod loginMethod()
    {
#ifdef USE_OTP_VIA_EMAIL
        return LoginMethod::OTPViaEmail;
#else
        return LoginMethod::Basic;
#endif
    }

}
