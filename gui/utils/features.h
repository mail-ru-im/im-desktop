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
    bool phoneAllowed();
    bool externalPhoneAttachment();
    bool showNotificationsText();
    bool showNotificationsTextSettings();

    QString dataVisibilityLink();
    QString passwordRecoveryLink();
    QString securityCallLink();
    QString attachPhoneUrl();

    enum class LoginMethod
    {
        Basic,
        OTPViaEmail
    };

    LoginMethod loginMethod();
    size_t getSMSResultTime();

    bool showAttachPhoneNumberPopup();
    bool closeBtnAttachPhoneNumberPopup();
    int showTimeoutAttachPhoneNumberPopup();
    QString textAttachPhoneNumberPopup();
    QString titleAttachPhoneNumberPopup();

    bool isSmartreplyEnabled();
    std::chrono::milliseconds smartreplyHideTime();
    int smartreplyMsgidCacheSize();

    bool isSuggestsEnabled();
    std::chrono::milliseconds getSuggestTimerInterval();
    bool isServerSuggestsEnabled();
    int maxAllowedLocalSuggestChars();
    int maxAllowedSuggestChars();
    int maxAllowedLocalSuggestWords();
    int maxAllowedSuggestWords();

    bool betaUpdateAllowed();
    bool updateAllowed();
    bool avatarChangeAllowed();
    bool clRemoveContactsAllowed();
    bool changeNameAvailable();

    bool pollsEnabled();

    int getFsIDLength();
}
