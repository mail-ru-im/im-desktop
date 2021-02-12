#pragma once

namespace Features
{
    bool isNicksEnabled();
    QString getProfileDomain();
    QString getProfileDomainAgent();
    size_t maximumUndoStackSize();
    bool useAppleEmoji();
    bool opensOnClick();
    bool phoneAllowed();
    bool externalPhoneAttachment();
    bool showNotificationsText();
    bool showNotificationsTextSettings();

    QString dataVisibilityLink();
    QString passwordRecoveryLink();
    QString securityCallLink();
    QString attachPhoneUrl();
    QString updateAppUrl();

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
    bool isSmartreplyForQuoteEnabled();
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

    bool changeNameAllowed();
    bool changeInfoAllowed();

    bool pollsEnabled();

    int getFsIDLength();

    bool isSpellCheckEnabled();
    size_t spellCheckMaxSuggestCount();

    QString favoritesImageIdEnglish();
    QString favoritesImageIdRussian();

    int getAsyncResponseTimeout();

    int getVoipCallUserLimit();
    int getVoipVideoUserLimit();
    int getVoipBigConferenceBoundary();

    bool isVcsCallByLinkEnabled();
    bool isVcsWebinarEnabled();

    QString getVcsRoomList();

    std::string getReactionsJson();
    bool reactionsEnabled();

    std::string getStatusJson();
    bool isStatusEnabled();
    bool isCustomStatusEnabled();

    bool isGlobalContactSearchAllowed();

    bool forceCheckMacUpdates();
    bool callRoomInfoEnabled();

    bool isYourInvitesButtonVisible();
    bool isGroupInvitesBlacklistEnabled();

    bool IvrLoginEnabled();
    int IvrResendCountToShow();

    bool isContactUsViaBackend();
    bool isUpdateFromBackendEnabled();

    bool isWebpScreenshotEnabled();
    qint64 maxFileSizeForWebpConvert();

    bool isInviteBySmsEnabled();
    bool shouldShowSmsNotifySetting();

    bool isAnimatedStickersInPickerAllowed();
    bool isAnimatedStickersInChatAllowed();

    bool isContactListSmoothScrollingEnabled();

    bool isBackgroundPttPlayEnabled();

    bool removeDeletedFromNotifications();

    QString statusBannerEmojis();
}
