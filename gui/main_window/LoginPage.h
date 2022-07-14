#pragma once

#include "../types/common_phone.h"
#include "../controls/TextUnit.h"
#include "accessibility/LinkAccessibleWidget.h"

namespace core
{
    namespace stats
    {
        enum class stats_event_names;
    }
}

namespace Ui
{
    class LabelEx;
    class LineEditEx;
    class CommonInputContainer;
    class PhoneInputContainer;
    class CodeInputContainer;
    class TextEditEx;
    class CustomButton;
    class DialogButton;
    class TermsPrivacyWidget;
    class UserAgreementWidget;
    class IntroduceYourself;
    class TextWidget;
    class ContextMenu;
    class ContactUsWidget;

    class EntryHintWidget : public LinkAccessibleWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void changeClicked();
        private:
            TextRendering::TextUnitPtr textUnit_;
            bool explained_;
        private:
            void updateSize();
        protected:
            void paintEvent(QPaintEvent* _event) override;
            void mouseReleaseEvent(QMouseEvent* _event) override;
            void mouseMoveEvent(QMouseEvent* _event) override;
        public:
            explicit EntryHintWidget(QWidget* _parent, const QString& _initialText);
            void setText(const QString& _text);
            void appendPhone(const QString& _text);
            void appendLink(const QString& _text, const QString& _link, bool _forceShowLink = false);
            const TextRendering::TextUnitPtr& getTextUnit() const override { return textUnit_; }
    };

    class LoginPage : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void country(const QString&);
        void loggedIn();
        void attached();

    private Q_SLOTS:
        void nextPage();
        void updateTimer();
        void resendCode();
        void sendCode();
        void callPhone();
        void getSmsResult(int64_t, int, int, const QString& _ivrUrl, const QString& _checks);
        void oauthRequired();
        void loginResult(int64_t, int, bool);
        void loginResultAttachPhone(int64_t, int);
        void clearErrors(bool ignorePhoneInfo = false);
        void codeEditChanged(const QString&);
        void stats_edit_phone();
        void postSmsSendStats();
        void phoneInfoResult(qint64, const Data::PhoneInfo&);
        void phoneTextChanged();
        void authError(const int _result);
        void onUrlConfigError(const int _error);

    public Q_SLOTS:
        void loginAfterUserAgreementAccept();
        void logoutAfterUserAgreementAccept();
        void doLogin();
        void prevPage();
        void switchLoginType();
        void openProxySettings();
        void openUserAgreement(const QString& _confidentialLink, const QString& _pdLink);

   private Q_SLOTS:
        void openOAuth2Dialog();
        void onAuthCodeReceived(const QString& _token);
        void onAuthDialogResult(int _result);
        void onLogout();

    public:
        LoginPage(QWidget* _parent);
        ~LoginPage() = default;
        void enableLoginPage(bool _needInitSubPage = false);
        void userAgreementSwitchPages(bool _needEnable);

        static bool isCallCheck(const QString& _checks);

    protected:
        void keyPressEvent(QKeyEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        enum class LoginSubpage
        {
            SUBPAGE_PHONE_LOGIN_INDEX = 0,
            SUBPAGE_PHONE_CONF_INDEX = 1,
            SUBPAGE_UIN_LOGIN_INDEX = 2,
            SUBPAGE_INTRODUCEYOURSELF = 3,
            SUBPAGE_REPORT = 4,
            SUBPAGE_OAUTH2_LOGIN_INDEX = 5
        };

        void init();
        void login();
        void initLoginSubPage(const LoginSubpage _index);
        void initGDPR();
        bool needGDPRPage() const;
        void connectGDPR();
        void setGDPRacceptedThisSession(bool _accepted) { gdprAccepted_ = _accepted; };
        bool gdprAcceptedThisSession() const{ return gdprAccepted_; };
        bool needUserAgreementPage() const;
        void setUserAgreementAcceptedThisSession(bool _accepted) { userAgreementAccepted_ = _accepted; };
        bool userAgreementAcceptedThisSession() const { return userAgreementAccepted_; }
        void setErrorText(int _result);
        void setErrorText(const QString& _customError);
        void updateErrors(int _result);
        static void updateKeedLoggedInSetting(bool _keep_logged_in);

        void prepareLoginByPhone();

        void updateBackButton();
        void updateNextButton();
        void updateInputFocus();

        enum class UpdateMode
        {
            Delayed,
            Instant
        };
        void updatePage(UpdateMode _mode = UpdateMode::Delayed);
        void updateButtonsWidgetHeight();
        void resizeSpacers();

        void setLoginPageFocus() const;

        enum class OTPAuthState
        {
            Email,
            Password
        };

        LoginSubpage currentPage() const;

        void updateOTPState(OTPAuthState _state);
        std::string statType(const LoginSubpage _page) const;

        void showUrlConfigHostDialog();

    private:

        QStackedWidget*         mainStakedWidget_ = nullptr;

        TermsPrivacyWidget*     termsWidget_ = nullptr;
        UserAgreementWidget*    userAgreementWidget_ = nullptr;
        QTimer*                 timer_;
        LineEditEx*             countryCode_;
        PhoneInputContainer*    phone_;
        unsigned int            remainingSeconds_;

        QStackedWidget*         loginStakedWidget_;
        QWidget*                controlsWidget_;
        QWidget*                hintsWidget_;

        CustomButton*           loginButton_;
        DialogButton*           nextButton_;
        CustomButton*           changePageButton_;
        CustomButton*           proxySettingsButton_;
        CustomButton*           reportButton_;
        QPushButton*            resendButton_;
        CommonInputContainer*   uinInput_;
        CommonInputContainer*   passwordInput_;
        QWidget*                keepLoggedWidget_;
        QWidget*                errorWidget_;
        QWidget*                buttonsWidget_;
        QCheckBox*              keepLogged_;
        CodeInputContainer*     codeInput_;
        LabelEx*                errorLabel_;
        TextWidget*             titleLabel_;
        EntryHintWidget*        entryHint_;
        QWidget*                forgotPasswordWidget_;
        LabelEx*                forgotPasswordLabel_;
        bool                    isLogin_;
        int64_t                 sendSeq_;
        int                     codeLength_;
        bool                    phoneChangedAuto_;
        bool                    gdprAccepted_;
        bool                    userAgreementAccepted_;
        bool                    loggedIn_;

        Data::PhoneInfo         receivedPhoneInfo_;
        QString                 checks_;
        QString                 ivrUrl_;

        IntroduceYourself* introduceMyselfWidget_;
        ContactUsWidget* contactUsWidget_;

        std::optional<OTPAuthState> otpState_;

        LoginSubpage lastPage_;
        int smsCodeSendCount_;
        QSpacerItem* topSpacer_;
        QSpacerItem* bottomSpacer_;
        ContextMenu* resendMenu_;
        QString lastEnteredCode_;
    };
}
