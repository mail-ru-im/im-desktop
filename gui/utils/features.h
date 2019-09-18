#pragma once

namespace Features
{
    bool isNicksEnabled();
    QString getProfileDomain();
    QString getProfileDomainAgent();
    size_t maximumUndoStackSize();
    bool useAppleEmoji();
    bool opensOnClick();
    bool forceShowChatPopup();
    bool bizPhoneAllowed();

    QString dataVisibilityLink();
    QString passwordRecoveryLink();
    QString securityCallLink();

    enum class LoginMethod
    {
        Basic,
        OTPViaEmail
    };

    LoginMethod loginMethod();
}
