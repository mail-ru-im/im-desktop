#pragma once

#include "../types/common_phone.h"

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
    class TextEditEx;
    class CountrySearchCombobox;
    class CustomButton;

    class LoginPage : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void country(const QString&);
        void loggedIn();
        void attached();

    private Q_SLOTS:
        void countrySelected(const QString&);
        void nextPage();
        void updateTimer();
        void sendCode();
        void setPhoneFocusIn();
        void setPhoneFocusOut();
        void getSmsResult(int64_t, int, int, const QString&, const QString& _checks);
        void loginResult(int64_t, int);
        void loginResultAttachUin(int64_t, int);
        void loginResultAttachPhone(int64_t, int);
        void clearErrors(bool ignorePhoneInfo = false);
        void codeEditChanged(const QString&);
        void countryCodeChanged(const QString&);
        void redrawCountryCode();
        void emptyPhoneRemove();
        void stats_edit_phone();
        void stats_resend_sms();
        void phoneInfoResult(qint64, const Data::PhoneInfo&);
        void phoneTextChanged();
        void authError(const int _result);
        void showGDPR();

    public Q_SLOTS:
        void prevPage();
        void switchLoginType();
        void updateFocus();
        void openProxySettings();

    public:
        LoginPage(QWidget* _parent, bool _isLogin);
        ~LoginPage();

        static bool isCallCheck(const QString& _checks);

    protected:
        virtual void keyPressEvent(QKeyEvent* _event) override;
        virtual void paintEvent(QPaintEvent* _event) override;

    private:
        void init();
        void initLoginSubPage(int _index);
        void initGDPRpopup();
        bool gdprPopupNeeded() const;
        void connectGDPR();
        void launchGDPRpopup(int _timeout);
        void setGDPRacceptedThisSession(bool _accepted);
        bool userAcceptedGDPR() const;
        bool gdprAcceptedThisSession() const;
        void setErrorText(int _result);
        void setErrorText(const QString& _customError);
        void updateErrors(int _result);

        void updateTextOnGetSmsResult();

        enum class OTPAuthState
        {
            Email,
            Password
        };

        void updateOTPState(OTPAuthState _state);

    private:
        QTimer*                 timer_;
        LineEditEx*             countryCode_;
        LineEditEx*             phone_;
        CountrySearchCombobox*  combobox_;
        unsigned int            remainingSeconds_;
        QString                 prevCountryCode_;

        QStackedWidget*         loginStakedWidget;
        QPushButton*            nextButton;
        QWidget*                countrySearchWidget;
        CustomButton*           backButton;
        QPushButton*            proxySettingsButton;
        QPushButton*            editPhoneButton;
        LabelEx*                switchButton;
        QPushButton*            resendButton;
        QFrame*                 phoneWidget;
        LineEditEx*             uinEdit;
        LineEditEx*             passwordEdit;
        QCheckBox*              keepLogged;
        LineEditEx*             codeEdit;
        LabelEx*                errorLabel;
        QLabel*                 enteredPhone;
        QLabel*                 hintLabel;
        QLabel*                 phoneHintLabel_;
        LabelEx*                passwordForgottenLabel;
        bool                    isLogin_;
        int64_t                 sendSeq_;
        int                     codeLength_;
        bool                    phoneChangedAuto_;
        bool                    gdprAccepted_;

        Data::PhoneInfo         receivedPhoneInfo_;
        QString                 checks_;

        std::optional<OTPAuthState> otpState_;
    };
}
