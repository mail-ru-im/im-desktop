#include "stdafx.h"
#include "LoginPage.h"

#include "../core_dispatcher.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../controls/ConnectionSettingsWidget.h"
#include "../controls/CountrySearchCombobox.h"

#include "../controls/LabelEx.h"
#include "../controls/LineEditEx.h"
#include "../controls/TextEditEx.h"
#include "../controls/GeneralDialog.h"
#include "../controls/CustomButton.h"
#include "../controls/DialogButton.h"
#include "../controls/CheckboxList.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../omicron/omicron_helper.h"
#include "../utils/features.h"
#include "TermsPrivacyWidget.h"
#include "AcceptAgreementInfo.h"
#include "MainWindow.h"
#include "app_config.h"
#include "../styles/ThemeParameters.h"

namespace
{
    enum LoginPagesIndex
    {
        SUBPAGE_PHONE_LOGIN_INDEX = 0,
        SUBPAGE_PHONE_CONF_INDEX = 1,
        SUBPAGE_UIN_LOGIN_INDEX = 2,
    };

    qint64 phoneInfoLastRequestSpecialId_ = 0; // using for settings only (last entered uin/phone)
    qint64 phoneInfoLastRequestId_ = 0;
    int phoneInfoRequestsCount_ = 0;

    QString getEditPhoneStyle()
    {
        return qsl(
            "QPushButton { background-color: %1; color: %2; margin-left: 16dip; "
            "min-height: 40dip; max-height: 40dip; min-width: 100dip;border-style: none; }")
            .arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE),
                Styling::getParameters().getColorHex(Styling::StyleVariable::PRIMARY));
    }

    QString getResendButtonStyle()
    {
        return qsl(
            "QPushButton:disabled { background-color: %3; min-height: 30dip; max-height: 30dip; color: %1; border-style: none; } "
            "QPushButton:enabled { background-color:%3; min-height: 30dip; max-height: 30dip; color: %2; border-style: none; } ")
            .arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_PRIMARY),
                Styling::getParameters().getColorHex(Styling::StyleVariable::PRIMARY),
                 Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE));
    }

    QString getCountryComboboxStyle()
    {
        return qsl(
            "QLineEdit { background-position: right; margin-right: 12dip; background-repeat: no-repeat; font-size: 18dip; color: %1; } "
            "QLineEdit:focus { padding-left: 32dip; margin-right: 0dip; background-repeat: no-repeat; color: %1;} "
        ).arg(Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID));
    }

    QString getPasswordLink()
    {
        if (build::is_biz())
        {
            constexpr std::string_view link = feature::default_no_account_link();
            const static auto defaultLink = QString::fromUtf8(link.data(), link.size());
            return Omicron::_o("biz_no_account_link", defaultLink);
        }

        return Features::passwordRecoveryLink();
    }

    enum class HintType
    {
        EnterEmailOrUin,
        EnterEmail,
        EnterPhone,
        EnterSMSCode,
        EnterOTPFromEmail,
        LoginByPhoneCall
    };

    QString hintLabelText(HintType _type)
    {
        switch (_type) {
            case HintType::EnterEmailOrUin:
                return QT_TRANSLATE_NOOP("login_page","Enter UIN or Email");
            case HintType::EnterEmail:
                return QT_TRANSLATE_NOOP("login_page", "Enter your Email");
            case HintType::EnterPhone:
                return QT_TRANSLATE_NOOP("login_page","Enter phone number");
            case HintType::EnterSMSCode:
                return QT_TRANSLATE_NOOP("login_page", "Enter code from SMS");
            case HintType::EnterOTPFromEmail:
                return QT_TRANSLATE_NOOP("login_page", "Enter one-time password received by email");
            case HintType::LoginByPhoneCall:
                return QT_TRANSLATE_NOOP("login_page", "Login by phone call");
            default:
                break;
        }

        return QString();
    }

    int hintLabelMinHeight(const int _linesCount)
    {
        return Utils::scale_value(48) * _linesCount;
    }
}

namespace Ui
{
    LoginPage::LoginPage(QWidget* _parent, bool _isLogin)
        : QWidget(_parent)
        , timer_(new QTimer(this))
        , countryCode_(new LineEditEx(this))
        , phone_(new LineEditEx(this))
        , combobox_(new CountrySearchCombobox(this))
        , remainingSeconds_(0)
        , isLogin_(_isLogin)
        , sendSeq_(0)
        , codeLength_(4)
        , phoneChangedAuto_(false)
        , gdprAccepted_(false)
    {
        Utils::ApplyStyle(this, Styling::getParameters().getLoginPageQss());
        QVBoxLayout* verticalLayout = Utils::emptyVLayout(this);

        auto backButtonWidget = new QWidget(this);
        auto backButtonLayout = Utils::emptyHLayout(backButtonWidget);
        Utils::ApplyStyle(backButtonWidget, qsl("background-color: transparent;"));
        backButtonWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        backButtonLayout->setContentsMargins(Utils::scale_value(14), Utils::scale_value(14), Utils::scale_value(14), Utils::scale_value(14));

        {
            backButton = new CustomButton(backButtonWidget, qsl(":/controls/back_icon"), QSize(20, 20));
            Styling::Buttons::setButtonDefaultColors(backButton );
            backButton->setFixedSize(Utils::scale_value(QSize(24, 24)));
            Testing::setAccessibleName(backButton, qsl("AS lp backButton"));
            backButtonLayout->addWidget(backButton);
        }

        {
            auto buttonsSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            backButtonLayout->addItem(buttonsSpacer);
        }

        {
            proxySettingsButton = new QPushButton(backButtonWidget);
            proxySettingsButton->setObjectName(qsl("settingsButton"));
            proxySettingsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            proxySettingsButton->setFlat(true);
            proxySettingsButton->setFocusPolicy(Qt::NoFocus);
            proxySettingsButton->setCursor(Qt::PointingHandCursor);
            Testing::setAccessibleName(proxySettingsButton, qsl("AS lp proxySettingsButton"));
            backButtonLayout->addWidget(proxySettingsButton);
        }

        Testing::setAccessibleName(backButtonWidget, qsl("AS lp backButtonWidget"));
        verticalLayout->addWidget(backButtonWidget);

        if (isLogin_)
        {
            QSpacerItem* verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            verticalLayout->addItem(verticalSpacer);
        }

        QWidget* mainWidget = new QWidget(this);
        mainWidget->setObjectName(qsl("mainWidget"));
        mainWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        QHBoxLayout* mainLayout = Utils::emptyHLayout(mainWidget);

        if (isLogin_)
        {
            QSpacerItem* horizontalSpacer_6 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            mainLayout->addItem(horizontalSpacer_6);
        }

        QWidget* controlsWidget = new QWidget(mainWidget);
        controlsWidget->setObjectName(qsl("controlsWidget"));
        controlsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        QVBoxLayout* controlsLayout = Utils::emptyVLayout(controlsWidget);

        if (isLogin_)
        {
            const auto size = Utils::scale_value(QSize(80, 80));
            auto logoLabel = new QLabel(controlsWidget);
            logoLabel->setPixmap(Utils::renderSvg(build::GetProductVariant(qsl(":/logo/logo_icq"), qsl(":/logo/logo_agent"), qsl(":/logo/logo_biz"), qsl(":/logo/logo_dit")), size));
            logoLabel->setFixedSize(size);
            logoLabel->setAlignment(Qt::AlignCenter);
            Testing::setAccessibleName(logoLabel, qsl("AS lp logoWidget"));
            controlsLayout->addWidget(logoLabel);
            controlsLayout->setAlignment(logoLabel, Qt::AlignHCenter);
        }

        hintLabel = new QLabel(controlsWidget);
        hintLabel->setWordWrap(true);
        hintLabel->setObjectName(qsl("hint"));
        hintLabel->setMinimumHeight(hintLabelMinHeight(Features::loginMethod() == Features::LoginMethod::OTPViaEmail ? 2 : 1));

        passwordForgottenLabel = new LabelEx(controlsWidget);
        passwordForgottenLabel->setFont(Fonts::appFontScaled(16));
        passwordForgottenLabel->setObjectName(qsl("passwordForgotten"));
        passwordForgottenLabel->setCursor(Qt::PointingHandCursor);
        passwordForgottenLabel->setColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_INVERSE));
        passwordForgottenLabel->setAlignment(Qt::AlignCenter);
        connect(passwordForgottenLabel, &LabelEx::clicked, this, []()
        {
            QDesktopServices::openUrl(QUrl(getPasswordLink()));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::login_forgot_password);
        });

        if (isLogin_)
        {
            hintLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        }

        Testing::setAccessibleName(hintLabel, qsl("AS lp hintLabel"));
        controlsLayout->addWidget(hintLabel);

        QWidget* centerWidget = new QWidget(controlsWidget);
        Testing::setAccessibleName(centerWidget, qsl("AS lp centerWidget"));
        centerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QHBoxLayout* horizontalLayout = Utils::emptyHLayout(centerWidget);

        if (isLogin_)
        {
            QSpacerItem* horizontalSpacer_9 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            horizontalLayout->addItem(horizontalSpacer_9);
        }

        loginStakedWidget = new QStackedWidget(centerWidget);
        loginStakedWidget->setObjectName(qsl("stackedWidget"));
        loginStakedWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QWidget* phoneLoginWidget = new QWidget();
        phoneLoginWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QVBoxLayout* phoneLoginLayout = Utils::emptyVLayout(phoneLoginWidget);

        countrySearchWidget = new QWidget(phoneLoginWidget);
        countrySearchWidget->setObjectName(qsl("countryWidget"));
        countrySearchWidget->setLayout(Utils::emptyVLayout(countrySearchWidget));

        Testing::setAccessibleName(countrySearchWidget, qsl("AS lp countrySearchWidget"));
        phoneLoginLayout->addWidget(countrySearchWidget);

        phoneWidget = new QFrame(phoneLoginWidget);
        phoneWidget->setObjectName(qsl("phoneWidget"));
        phoneWidget->setFocusPolicy(Qt::ClickFocus);
        phoneWidget->setFrameShape(QFrame::NoFrame);
        phoneWidget->setFrameShadow(QFrame::Plain);
        phoneWidget->setLineWidth(0);
        phoneWidget->setProperty("Common", true);
        phoneWidget->setLayout(Utils::emptyHLayout(phoneWidget));

        Testing::setAccessibleName(phoneWidget, qsl("AS lp phoneWidget"));
        phoneLoginLayout->addWidget(phoneWidget);

        QSpacerItem* verticalSpacer_3 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        phoneLoginLayout->addItem(verticalSpacer_3);

        Testing::setAccessibleName(phoneLoginWidget, qsl("AS lp phoneLoginWidget"));
        loginStakedWidget->addWidget(phoneLoginWidget);
        QWidget* phoneConfirmWidget = new QWidget();
        phoneConfirmWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QVBoxLayout* phoneConfirmLayout = Utils::emptyVLayout(phoneConfirmWidget);

        QWidget* enteredPhoneWidget = new QWidget(phoneConfirmWidget);
        enteredPhoneWidget->setObjectName(qsl("enteredPhone"));
        QHBoxLayout* enteredPhoneLayout = Utils::emptyHLayout(enteredPhoneWidget);
        QSpacerItem* horizontalSpacer_4 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        enteredPhoneLayout->addItem(horizontalSpacer_4);

        enteredPhone = new QLabel(enteredPhoneWidget);
        enteredPhone->setFont(Fonts::appFontScaled(24, Fonts::FontWeight::Light));
        QPalette pal(enteredPhone->palette());
        pal.setColor(QPalette::Foreground, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        enteredPhone->setPalette(pal);

        Testing::setAccessibleName(enteredPhone, qsl("AS lp enteredPhone"));
        enteredPhoneLayout->addWidget(enteredPhone);

        editPhoneButton = new QPushButton(enteredPhoneWidget);
        editPhoneButton->setCursor(QCursor(Qt::PointingHandCursor));
        editPhoneButton->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(editPhoneButton, getEditPhoneStyle());
        editPhoneButton->setText(QT_TRANSLATE_NOOP("login_page", "Change"));

        Testing::setAccessibleName(editPhoneButton, qsl("AS lp editPhoneButton"));
        enteredPhoneLayout->addWidget(editPhoneButton);

        QSpacerItem* horizontalSpacer_5 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        enteredPhoneLayout->addItem(horizontalSpacer_5);

        Testing::setAccessibleName(enteredPhoneWidget, qsl("AS lp enteredPhoneWidget"));
        phoneConfirmLayout->addWidget(enteredPhoneWidget);

        phoneHintLabel_ = new QLabel(enteredPhoneWidget);
        phoneHintLabel_->setObjectName(qsl("phoneHint"));
        phoneHintLabel_->setWordWrap(true);
        phoneHintLabel_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        phoneConfirmLayout->addWidget(phoneHintLabel_);
        phoneHintLabel_->hide();

        codeEdit = new LineEditEx(phoneConfirmWidget);
        codeEdit->setObjectName(qsl("code"));
        codeEdit->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(codeEdit, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
        codeEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
        codeEdit->setAlignment(Qt::AlignCenter);
        updateTextOnGetSmsResult();
        Testing::setAccessibleName(codeEdit, qsl("AS lp codeEdit"));
        phoneConfirmLayout->addWidget(codeEdit);

        auto spacer = new QSpacerItem(0, Utils::scale_value(20), QSizePolicy::Preferred, QSizePolicy::Fixed);
        phoneConfirmLayout->addSpacerItem(spacer);

        resendButton = new QPushButton(phoneConfirmWidget);
        resendButton->setCursor(QCursor(Qt::PointingHandCursor));
        resendButton->setFocusPolicy(Qt::StrongFocus);
        resendButton->setFont(Fonts::appFontScaled(15));
        Utils::ApplyStyle(resendButton, getResendButtonStyle());

        Testing::setAccessibleName(resendButton, qsl("AS lp resendButton"));
        phoneConfirmLayout->addWidget(resendButton);

        QSpacerItem* verticalSpacer_4 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        phoneConfirmLayout->addItem(verticalSpacer_4);

        Testing::setAccessibleName(phoneConfirmWidget, qsl("AS lp phoneConfirmWidget"));
        loginStakedWidget->addWidget(phoneConfirmWidget);
        QWidget* uinLoginWidget = new QWidget();
        uinLoginWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QVBoxLayout * uinLoginLayout = Utils::emptyVLayout(uinLoginWidget);

        uinEdit = new LineEditEx(uinLoginWidget);
        uinEdit->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(uinEdit, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
        uinEdit->setPlaceholderText(build::is_icq() ?
            QT_TRANSLATE_NOOP("login_page", "UIN or Email")
            : QT_TRANSLATE_NOOP("login_page", "Email")
        );
        uinEdit->setText(get_gui_settings()->get_value(login_page_last_entered_uin, QString()));
        uinEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

        Testing::setAccessibleName(uinEdit, qsl("StartWindowUinField"));
        uinLoginLayout->addWidget(uinEdit);

        passwordEdit = new LineEditEx(uinLoginWidget);
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(passwordEdit, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
        passwordEdit->setPlaceholderText(QT_TRANSLATE_NOOP("login_page", "Password"));
        passwordEdit->changeTextColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        passwordEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

        Testing::setAccessibleName(passwordEdit, qsl("StartWindowPasswordField"));
        uinLoginLayout->addWidget(passwordEdit);

        keepLogged = new Ui::CheckBox(uinLoginWidget);
        keepLogged->setVisible(isLogin_);
        keepLogged->setText(QT_TRANSLATE_NOOP("login_page", "Keep me signed in"));
        keepLogged->setChecked(get_gui_settings()->get_value(settings_keep_logged_in, true));

        Testing::setAccessibleName(keepLogged, qsl("AS lp keepLogged"));
        uinLoginLayout->addWidget(keepLogged);

        uinLoginLayout->addStretch();

        Testing::setAccessibleName(uinLoginWidget, qsl("AS lp uinLoginWidget"));
        loginStakedWidget->addWidget(uinLoginWidget);

        Testing::setAccessibleName(loginStakedWidget, qsl("AS lp loginStakedWidget"));
        horizontalLayout->addWidget(loginStakedWidget);

        QSpacerItem* horizontalSpacer_8 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_8);

        Testing::setAccessibleName(centerWidget, qsl("AS lp centerWidget"));
        controlsLayout->addWidget(centerWidget);
        Testing::setAccessibleName(passwordForgottenLabel, qsl("AS lp passwordForgottenLabel"));
        controlsLayout->addWidget(passwordForgottenLabel);

        QWidget* nextButtonWidget = new QWidget(controlsWidget);
        nextButtonWidget->setObjectName(qsl("nextButton"));

        QVBoxLayout* verticalLayout_8 = Utils::emptyVLayout(nextButtonWidget);
        nextButton = new DialogButton(nextButtonWidget, QT_TRANSLATE_NOOP("login_page", "NEXT"), DialogButtonRole::CONFIRM);
        nextButton->setCursor(QCursor(Qt::PointingHandCursor));
        nextButton->setAutoDefault(true);
        nextButton->setDefault(false);

        Testing::setAccessibleName(nextButton, qsl("StartWindowLoginButton"));
        verticalLayout_8->addWidget(nextButton);

        Testing::setAccessibleName(nextButtonWidget, qsl("AS lp nextButtonWidget"));
        controlsLayout->addWidget(nextButtonWidget);

        if (isLogin_)
        {
            controlsLayout->setAlignment(nextButtonWidget, Qt::AlignHCenter);
        }
        else
        {
            controlsLayout->setAlignment(nextButtonWidget, Qt::AlignLeft);
        }

        QWidget* widget = new QWidget(controlsWidget);
        widget->setObjectName(qsl("errorWidget"));
        QVBoxLayout* verticalLayout_7 = Utils::emptyVLayout(widget);
        errorLabel = new LabelEx(widget);
        errorLabel->setWordWrap(true);
        errorLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        errorLabel->setOpenExternalLinks(true);
        errorLabel->setFont(Fonts::appFontScaled(15));
        errorLabel->setColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));

        if (isLogin_)
        {
            errorLabel->setAlignment(Qt::AlignCenter);
        }
        else
        {
            errorLabel->setAlignment(Qt::AlignLeft);
        }

        Testing::setAccessibleName(errorLabel, qsl("AS login errorLabel"));
        verticalLayout_7->addWidget(errorLabel);
        QSpacerItem* verticalSpacer_E = new QSpacerItem(0, 3, QSizePolicy::Minimum, QSizePolicy::Expanding);
        verticalLayout_7->addItem(verticalSpacer_E);

        Testing::setAccessibleName(widget, qsl("AS login widget"));
        controlsLayout->addWidget(widget);

        Testing::setAccessibleName(controlsWidget, qsl("AS login controlsWidget"));
        mainLayout->addWidget(controlsWidget);

        if (isLogin_)
        {
            QSpacerItem* horizontalSpacer_7 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            mainLayout->addItem(horizontalSpacer_7);
        }
        else
        {
            mainLayout->setAlignment(Qt::AlignLeft);
        }

        Testing::setAccessibleName(mainWidget, qsl("AS login mainWidget"));
        verticalLayout->addWidget(mainWidget);

        QSpacerItem* verticalSpacer_2 = new QSpacerItem(0, 3, QSizePolicy::Minimum, QSizePolicy::Expanding);
        verticalLayout->addItem(verticalSpacer_2);

        {
            QWidget* switchLoginWidget = new QWidget(this);
            switchLoginWidget->setObjectName(qsl("switchLogin"));
            QHBoxLayout* switchLoginLayout = Utils::emptyHLayout(switchLoginWidget);
            QSpacerItem* horizontalSpacer = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

            switchLoginLayout->addItem(horizontalSpacer);

            switchButton = new LabelEx(switchLoginWidget);
            switchButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            switchButton->setCursor(QCursor(Qt::PointingHandCursor));
            switchButton->setFont(Fonts::appFontScaled(15));
            switchButton->setColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
            switchButton->setVisible(isLogin_ && !build::is_biz() && !build::is_dit());
            Testing::setAccessibleName(switchButton, qsl("StartWindowChangeLoginType"));
            switchLoginLayout->addWidget(switchButton);

            QSpacerItem* horizontalSpacer_2 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            switchLoginLayout->addItem(horizontalSpacer_2);
            Testing::setAccessibleName(switchLoginWidget, qsl("AS login switchLoginWidget"));
            verticalLayout->addWidget(switchLoginWidget);
        }

        loginStakedWidget->setCurrentIndex(2);

        connect(keepLogged, &QCheckBox::toggled, [](bool v)
        {
            if (get_gui_settings()->get_value(settings_keep_logged_in, true) != v)
                get_gui_settings()->set_value(settings_keep_logged_in, v);
        });

        Q_UNUSED(this);

        loginStakedWidget->setCurrentIndex(2);
        nextButton->setDefault(false);

        init();
    }

    LoginPage::~LoginPage()
    {
    }

    void LoginPage::keyPressEvent(QKeyEvent* _event)
    {
        if ((_event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter) &&
            (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX ||
                loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX))
        {
            nextButton->click();
        }
        return QWidget::keyPressEvent(_event);
    }

    void LoginPage::paintEvent(QPaintEvent* _event)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_event);
    }

    void LoginPage::init()
    {
        QMap<QString, QString> countryCodes = Utils::getCountryCodes();
        combobox_->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(combobox_, Styling::getParameters().getLineEditCommonQss() + getCountryComboboxStyle());
        combobox_->setComboboxViewClass("CountrySearchView");
        combobox_->setClass("CountySearchWidgetInternal");
        combobox_->setContextMenuPolicy(Qt::NoContextMenu);
        combobox_->setPlaceholder(QT_TRANSLATE_NOOP("login_page","Type country or code"));
        Testing::setAccessibleName(combobox_, qsl("AS lp combobox_"));
        countrySearchWidget->layout()->addWidget(combobox_);
        combobox_->setSources(countryCodes);

        connect(combobox_, &Ui::CountrySearchCombobox::selected, this, &LoginPage::countrySelected);
        connect(this, &LoginPage::country, this, &LoginPage::redrawCountryCode, Qt::QueuedConnection); //fix me
        connect(nextButton, &QPushButton::clicked, this, &LoginPage::nextPage);
        connect(backButton, &Ui::CustomButton::clicked, this, &LoginPage::prevPage);
        connect(editPhoneButton, &QPushButton::clicked, this, &LoginPage::prevPage);
        connect(editPhoneButton, &QPushButton::clicked, this, &LoginPage::stats_edit_phone);
        connect(switchButton, &Ui::LabelEx::clicked, this, &LoginPage::switchLoginType);
        connect(resendButton, &QPushButton::clicked, this, &LoginPage::sendCode);
        connect(resendButton, &QPushButton::clicked, this, &LoginPage::stats_resend_sms);
        connect(timer_, &QTimer::timeout, this, &LoginPage::updateTimer);

        connect(proxySettingsButton, &QPushButton::clicked, this, &LoginPage::openProxySettings);

        countryCode_->setObjectName(qsl("countryCodeEdit"));
        Utils::ApplyStyle(countryCode_, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
        phone_->setObjectName(qsl("phoneEdit"));
        phone_->setAttribute(Qt::WA_MacShowFocusRect, false);
        phone_->setPlaceholderText(QT_TRANSLATE_NOOP("login_page","phone number"));
        countryCode_->changeTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        phone_->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(phone_, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
        Testing::setAccessibleName(countryCode_, qsl("StartWindowCountryCodeField"));
        phoneWidget->layout()->addWidget(countryCode_);
        Testing::setAccessibleName(phone_, qsl("StartWindowPhoneNumberField"));
        phoneWidget->layout()->addWidget(phone_);

        connect(countryCode_, &Ui::LineEditEx::focusIn, this, &LoginPage::setPhoneFocusIn, Qt::QueuedConnection);
        connect(countryCode_, &Ui::LineEditEx::focusOut, this, &LoginPage::setPhoneFocusOut, Qt::QueuedConnection);
        connect(phone_, &Ui::LineEditEx::focusIn, this, &LoginPage::setPhoneFocusIn, Qt::QueuedConnection);
        connect(phone_, &Ui::LineEditEx::focusOut, this, &LoginPage::setPhoneFocusOut, Qt::QueuedConnection);

        connect(uinEdit, &Ui::LineEditEx::textChanged, this, [this]() { clearErrors(); }, Qt::DirectConnection);
        connect(passwordEdit, &Ui::LineEditEx::textEdited, this, [this]() { clearErrors(); }, Qt::DirectConnection);
        connect(codeEdit, &Ui::LineEditEx::textChanged, this, [this]() { clearErrors(); }, Qt::DirectConnection);
        connect(codeEdit, &Ui::LineEditEx::textChanged, this, &LoginPage::codeEditChanged, Qt::DirectConnection);
        connect(countryCode_, &Ui::LineEditEx::textChanged, this, [this]() { clearErrors(); }, Qt::DirectConnection);
        connect(countryCode_, &Ui::LineEditEx::textEdited, this, &LoginPage::countryCodeChanged, Qt::DirectConnection);
        connect(phone_, &Ui::LineEditEx::textChanged, this, &LoginPage::phoneTextChanged, Qt::DirectConnection);
        connect(phone_, &Ui::LineEditEx::emptyTextBackspace, this, &LoginPage::emptyPhoneRemove, Qt::QueuedConnection);

        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::getSmsResult, this, &LoginPage::getSmsResult, Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::loginResult, this, &LoginPage::loginResult, Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::loginResultAttachUin, this, &LoginPage::loginResultAttachUin, Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::loginResultAttachPhone, this, &LoginPage::loginResultAttachPhone, Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::phoneInfoResult, this, &LoginPage::phoneInfoResult, Qt::DirectConnection);
        QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::authError, this, &LoginPage::authError, Qt::QueuedConnection);

        countryCode_->setValidator(new QRegularExpressionValidator(QRegularExpression(qsl("[\\+\\d]\\d*"))));
        phone_->setValidator(new QRegularExpressionValidator(QRegularExpression(qsl("\\d*"))));
        codeEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(qsl("\\d*"))));

        combobox_->selectItem(Utils::GetTranslator()->getCurrentPhoneCode());
        errorLabel->hide();
        phone_->setFocus();
        countryCode_->setFocusPolicy(Qt::ClickFocus);
        countryCode_->setAttribute(Qt::WA_MacShowFocusRect, false);

        const auto lastLoginPage = get_gui_settings()->get_value<int>(login_page_last_login_type, SUBPAGE_PHONE_LOGIN_INDEX);
        initLoginSubPage(
            isLogin_ ? ((build::is_agent() || build::is_biz() || build::is_dit()) ? SUBPAGE_UIN_LOGIN_INDEX : lastLoginPage)
            : lastLoginPage
            );

        initGDPRpopup();

        if (Features::loginMethod() == Features::LoginMethod::OTPViaEmail)
            updateOTPState(OTPAuthState::Email);

        auto lastPhone = get_gui_settings()->get_value(login_page_last_entered_phone, QString());
        if (!lastPhone.isEmpty())
        {
            QTimer::singleShot(500, this, [=]() // workaround: phoneinfo/set_can_change_hosts_scheme ( https://jira.mail.ru/browse/IMDESKTOP-3773 )
            {
               phoneInfoRequestsCount_++;
               Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
               collection.set_value_as_qstring("phone", lastPhone);
               collection.set_value_as_qstring("gui_locale", get_gui_settings()->get_value(settings_language, QString()));
               phoneInfoLastRequestSpecialId_ = GetDispatcher()->post_message_to_core("phoneinfo", collection.get());
            });
        }
    }

    void LoginPage::updateFocus()
    {
        uinEdit->setFocus();
    }

    void LoginPage::initLoginSubPage(int _index)
    {
        setFocus();
        backButton->setVisible(isLogin_ && _index == SUBPAGE_PHONE_CONF_INDEX);
        proxySettingsButton->setVisible(isLogin_ && _index != SUBPAGE_PHONE_CONF_INDEX);
        nextButton->setVisible(_index != SUBPAGE_PHONE_CONF_INDEX);

        switchButton->setText(_index == SUBPAGE_UIN_LOGIN_INDEX ?
            QT_TRANSLATE_NOOP("login_page","Login via phone")
            : build::is_icq() ?
                QT_TRANSLATE_NOOP("login_page","Login with UIN/Email")
                : QT_TRANSLATE_NOOP("login_page", "Login with Email")
        );

        switch (_index)
        {
        case SUBPAGE_PHONE_LOGIN_INDEX:
            hintLabel->setText(hintLabelText(HintType::EnterPhone));
            passwordForgottenLabel->hide();
            phone_->setFocus();
            break;

        case SUBPAGE_PHONE_CONF_INDEX:
            hintLabel->setText(hintLabelText(isCallCheck(checks_) ? HintType::LoginByPhoneCall : HintType::EnterSMSCode));
            passwordForgottenLabel->hide();
            codeEdit->setFocus();
            break;

        case SUBPAGE_UIN_LOGIN_INDEX:
            if (isLogin_)
            {
                hintLabel->setText(hintLabelText(build::is_icq() ? HintType::EnterEmailOrUin : HintType::EnterEmail));
                passwordForgottenLabel->setText(build::is_biz() ? QT_TRANSLATE_NOOP("login_page","I don't have an account") : QT_TRANSLATE_NOOP("login_page", "Forgot password?"));
                passwordForgottenLabel->setColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
                passwordForgottenLabel->setFixedHeight(QFontMetrics(passwordForgottenLabel->font()).height() * 2);
                passwordForgottenLabel->setVisible(!build::is_dit());
            }
            else
            {
                hintLabel->setVisible(false);
                passwordForgottenLabel->hide();
            }
            uinEdit->setFocus();
            break;
        }
        loginStakedWidget->setCurrentIndex(_index);
    }

    void LoginPage::initGDPRpopup()
    {
        connectGDPR();

        if (gdprPopupNeeded())
            launchGDPRpopup(0);
    }

    bool LoginPage::gdprPopupNeeded() const
    {
        return isLogin_ && !GetAppConfig().GDPR_UserHasAgreed()
                && !GetAppConfig().GDPR_UserHasLoggedInEver();
    }

    void LoginPage::connectGDPR()
    {
        connect(this, &LoginPage::loggedIn,
            this, [this]()
        {
            // report to core
            if (!gdprAcceptedThisSession())
                return;

            gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_int("accept_flag", gdprAcceptedThisSession() ? static_cast<int32_t>(AcceptAgreementInfo::AgreementAction::Accept)
                                                                                 : static_cast<int32_t>(AcceptAgreementInfo::AgreementAction::Ignore));
            collection.set_value_as_bool("reset", false);

            auto appConfig = GetAppConfig();
            appConfig.SetGDPR_AgreementReportedToServer(AppConfig::GDPR_Report_To_Server_State::Sent_to_Core);
            ModifyAppConfig(appConfig, nullptr, nullptr, false);

            GetDispatcher()->post_message_to_core("agreement/gdpr", collection.get(), this,
                [](core::icollection* _coll) {
                Ui::gui_coll_helper coll(_coll, false);

                Q_UNUSED(coll);
            });

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::gdpr_accept_start);
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::logout,
                this, [this](){
            setGDPRacceptedThisSession(false);
        });
    }

    void LoginPage::launchGDPRpopup(int _timeout)
    {
        QTimer::singleShot(_timeout, this, &LoginPage::showGDPR);
    }

    void LoginPage::setGDPRacceptedThisSession(bool _accepted)
    {
        gdprAccepted_ = _accepted;
    }

    bool LoginPage::userAcceptedGDPR() const
    {
        return GetAppConfig().GDPR_UserHasAgreed() || gdprAcceptedThisSession();
    }

    bool LoginPage::gdprAcceptedThisSession() const
    {
        return gdprAccepted_;
    }

    void LoginPage::setPhoneFocusIn()
    {
        phoneWidget->setProperty("Focused", true);
        phoneWidget->setProperty("Error", false);
        phoneWidget->setProperty("Common", false);
        phoneWidget->setStyle(QApplication::style());
        emit country(countryCode_->text());
    }

    void LoginPage::setPhoneFocusOut()
    {
        phoneWidget->setProperty("Focused", false);
        phoneWidget->setProperty("Error", false);
        phoneWidget->setProperty("Common", true);
        phoneWidget->setStyle(QApplication::style());
        emit country(countryCode_->text());
    }

    void LoginPage::redrawCountryCode()
    {
        QFontMetrics fm = countryCode_->fontMetrics();
        int w = fm.boundingRect(countryCode_->text()).width() + 5;

        QRect content = phoneWidget->contentsRect();
        countryCode_->resize(w, countryCode_->height());
        phone_->resize(content.width() - w, phone_->height());
        countryCode_->move(content.x(), content.y());
        phone_->move(content.x() + w, content.y());
    }

    void LoginPage::countrySelected(const QString& _code)
    {
        if (prevCountryCode_ == _code)
            return;

        if (!prevCountryCode_.isEmpty())
        {
            core::stats::event_props_type props;
            props.emplace_back("prev_code", prevCountryCode_.toStdString());
            props.emplace_back("next_code", _code.toStdString());
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_edit_country, props);
        }

        prevCountryCode_ = _code;
        countryCode_->setText(_code);
        redrawCountryCode();

        if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX) {
            phone_->setFocus();
        }
    }

    void LoginPage::nextPage()
    {
        static const auto waitServerResponseForPhoneNumberCheckInMsec = 7500;
        static const auto checkServerResponseEachNthMsec = 50;
        static const auto maxTriesOfCheckingForServerResponse = (waitServerResponseForPhoneNumberCheckInMsec / checkServerResponseEachNthMsec);
        static int triesPhoneAuth = 0;
        if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX)
        {
            if (isEnabled())
            {
                setEnabled(false);
            }
            if (phoneInfoRequestsCount_ > 0)
            {
                ++triesPhoneAuth;
                if (triesPhoneAuth > maxTriesOfCheckingForServerResponse)
                {
                    triesPhoneAuth = 0;
                    setEnabled(true);
                    errorLabel->setVisible(true);
                    setErrorText(core::le_unknown_error);
                    phoneInfoRequestsCount_ = 0;
                    phoneInfoLastRequestId_ = 0;
                    return;
                }
                QTimer::singleShot(checkServerResponseEachNthMsec, this, &LoginPage::nextPage);
                return;
            }
        }
        triesPhoneAuth = 0;
        setEnabled(true);
        if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX && receivedPhoneInfo_.isValid())
        {
            bool isMobile = false;
            for (const auto& status: receivedPhoneInfo_.prefix_state_)
            {
                if (QString::fromStdString(status).toLower() == ql1s("mobile"))
                {
                    isMobile = true;
                    break;
                }
            }
            const auto isOk = QString::fromStdString(receivedPhoneInfo_.status_).toLower() == ql1s("ok");
            const auto message = !receivedPhoneInfo_.printable_.empty() ? QString::fromStdString(receivedPhoneInfo_.printable_[0]) : QString();
            if ((!isMobile || !isOk) && !message.isEmpty())
            {
                errorLabel->setVisible(true);
                setErrorText(message);
                phone_->setFocus();
                return;
            }
            else
            {
                errorLabel->setVisible(false);
            }
        }
        setFocus();
        clearErrors(true);
        if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX)
        {
            enteredPhone->setText(countryCode_->text() + phone_->text());
            enteredPhone->adjustSize();
            countryCode_->setEnabled(false);
            phone_->setEnabled(false);
            sendCode();
        }
        else if (loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX)
        {
            uinEdit->setEnabled(false);

            if (!otpState_)
                passwordEdit->setEnabled(false);

            gui_coll_helper collection(GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("login", uinEdit->text().trimmed());
            collection.set_value_as_qstring("password", passwordEdit->text());
            collection.set_value_as_bool("save_auth_data", keepLogged->isChecked());

            if (otpState_ && otpState_ ==  OTPAuthState::Email)
                collection.set_value_as_enum("token_type", core::token_type::otp_via_email);

            collection.set_value_as_bool("not_log", true);
            if (isLogin_)
            {
                sendSeq_ = GetDispatcher()->post_message_to_core("login_by_password", collection.get());
            }
            else
            {
                sendSeq_ = GetDispatcher()->post_message_to_core("login_by_password_for_attach_uin", collection.get());
            }
        }
    }

    void LoginPage::prevPage()
    {
        clearErrors();
        initLoginSubPage(SUBPAGE_PHONE_LOGIN_INDEX);
    }

    void LoginPage::switchLoginType()
    {
        setFocus();
        clearErrors();
        initLoginSubPage(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX ? SUBPAGE_PHONE_LOGIN_INDEX : SUBPAGE_UIN_LOGIN_INDEX);
        GetDispatcher()->post_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
            ? core::stats::stats_event_names::reg_page_uin : core::stats::stats_event_names::reg_page_phone);
    }

    void LoginPage::stats_edit_phone()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_edit_phone);
    }

    void LoginPage::phoneTextChanged()
    {
        if (!phoneChangedAuto_)
        {
            clearErrors();
        }
    }

    void LoginPage::stats_resend_sms()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_sms_resend);
        GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::reg_sms_resend);
    }

    void LoginPage::updateTimer()
    {
        resendButton->setEnabled(false);
        const auto callCheck = isCallCheck(checks_);
        QString text = callCheck ? QT_TRANSLATE_NOOP("login_page", "Recall") : QT_TRANSLATE_NOOP("login_page", "Resend code");
        if (remainingSeconds_ == 60)
        {
            text = callCheck ? QT_TRANSLATE_NOOP("login_page", "Recall in %1").arg(qsl("1:00")) : QT_TRANSLATE_NOOP("login_page", "Resend code in %1").arg(qsl("1:00"));
        }
        else if (remainingSeconds_ > 0)
        {
            const auto s = (remainingSeconds_ >= 10 ? qsl("0:%1") : qsl("0:0%1")).arg(remainingSeconds_);
            text = callCheck ? QT_TRANSLATE_NOOP("login_page", "Recall in %1").arg(s) :
                QT_TRANSLATE_NOOP("login_page", "Resend code in %1").arg(s);
        }
        else
        {
            resendButton->setEnabled(true);
        }
        resendButton->setText(text);

        if (remainingSeconds_)
        {
            --remainingSeconds_;
            timer_->start(1000);
        }
        else
        {
            timer_->stop();
        }
    }

    void LoginPage::sendCode()
    {
        timer_->stop();
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("country", countryCode_->text());
        collection.set_value_as_qstring("phone", phone_->text());
        collection.set_value_as_qstring("locale", Utils::GetTranslator()->getLang());
        collection.set_value_as_bool("is_login", isLogin_);

        sendSeq_ = GetDispatcher()->post_message_to_core("login_get_sms_code", collection.get());
        remainingSeconds_ = 60;
        updateTimer();
    }

    void LoginPage::getSmsResult(int64_t _seq, int _result, int _codeLength, const QString&, const QString& _checks)
    {
        if (_seq != sendSeq_)
            return;

        countryCode_->setEnabled(true);
        phone_->setEnabled(true);
        setErrorText(_result);
        errorLabel->setVisible(_result);
        if (_result == core::le_success)
        {
            if (_codeLength != 0)
                codeLength_ = _codeLength;
            checks_ = _checks;
            clearErrors();
            updateTextOnGetSmsResult();
            return initLoginSubPage(SUBPAGE_PHONE_CONF_INDEX);
        }

        phoneWidget->setProperty("Error", true);
        phoneWidget->setProperty("Focused", false);
        phoneWidget->setProperty("Common", false);
        phoneWidget->setStyle(QApplication::style());
        Utils::ApplyStyle(phone_, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
        emit country(countryCode_->text());
    }

    void LoginPage::updateErrors(int _result)
    {
        codeEdit->setEnabled(true);
        uinEdit->setEnabled(true);
        passwordEdit->setEnabled(true);
        setErrorText(_result);
        errorLabel->setVisible(_result);

        if (loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX)
        {
            Utils::ApplyStyle(uinEdit, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
            uinEdit->changeTextColor(Styling::getParameters().getColor(uinEdit->text().length() > 0 ? Styling::StyleVariable::TEXT_SOLID : Styling::StyleVariable::BASE_PRIMARY));
            passwordEdit->clear();
            uinEdit->setFocus();
        }
        else
        {
            Utils::ApplyStyle(codeEdit, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
            codeEdit->setFocus();
        }
    }

    void LoginPage::updateTextOnGetSmsResult()
    {
        auto getCallStr = [](auto codeLength)
        {
            return Utils::GetTranslator()->getNumberString(
                codeLength,
                QT_TRANSLATE_NOOP3("login_page", "Enter last %1 digit", "1"),
                QT_TRANSLATE_NOOP3("login_page", "Enter last %1 digits", "2"),
                QT_TRANSLATE_NOOP3("login_page", "Enter last %1 digits", "5"),
                QT_TRANSLATE_NOOP3("login_page", "Enter last %1 digits", "21")).arg(codeLength);
        };

        phoneHintLabel_->setVisible(isCallCheck(checks_));
        phoneHintLabel_->setText(getCallStr(codeLength_));
    }

    void LoginPage::updateOTPState(OTPAuthState _state)
    {
        otpState_ = _state;
        uinEdit->setDisabled(_state == OTPAuthState::Password);
        passwordEdit->setVisible(_state == OTPAuthState::Password);
        keepLogged->setVisible(_state == OTPAuthState::Password);

        hintLabel->setText(hintLabelText(_state == OTPAuthState::Password ? HintType::EnterOTPFromEmail : HintType::EnterEmail));

        if (_state == OTPAuthState::Email)
            uinEdit->setFocus();
        else
            passwordEdit->setFocus();
    }

    bool LoginPage::isCallCheck(const QString& _checks)
    {
        return _checks == ql1s("callui");
    }

    void LoginPage::loginResult(int64_t _seq, int _result)
    {
        if (sendSeq_ > 0 && _seq != sendSeq_)
            return;

        updateErrors(_result);

        if (_result == 0)
        {
            if (otpState_ && otpState_ == OTPAuthState::Email)
            {
                updateOTPState(OTPAuthState::Password);
                return;
            }

            if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_CONF_INDEX)
            {
                codeEdit->setText(QString());
                initLoginSubPage(SUBPAGE_PHONE_LOGIN_INDEX);
            }

            GetDispatcher()->post_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
                ? core::stats::stats_event_names::reg_login_uin
                : core::stats::stats_event_names::reg_login_phone);

            GetDispatcher()->post_im_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
                ? core::stats::im_stat_event_names::reg_login_uin
                : core::stats::im_stat_event_names::reg_login_phone);

            get_gui_settings()->set_value<int>(login_page_last_login_type, loginStakedWidget->currentIndex());

            clearErrors();
            emit loggedIn();

            phoneInfoRequestsCount_ = 0;
            phoneInfoLastRequestId_ = 0;
            receivedPhoneInfo_ = Data::PhoneInfo();
        }

        if (otpState_ && otpState_ == OTPAuthState::Password)
            updateOTPState(OTPAuthState::Email);
    }

    void LoginPage::authError(const int _result)
    {
        updateErrors(_result);
    }

    void LoginPage::showGDPR()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::gdpr_show_start);

        TermsPrivacyWidget::Options options;
        options.blockUntilAccepted_ = false;
        options.controlContainingDialog_ = false;

        auto termsWidget = new TermsPrivacyWidget(QT_TRANSLATE_NOOP("terms_privacy_widget", "Terms and Privacy Policy"),
                                                  QT_TRANSLATE_NOOP("terms_privacy_widget",
                                                                    "By clicking \"I Agree\", you confirm that you have read carefully "
                                                                    "and agree to our <a href=\"%1\">Terms</a> "
                                                                    "and <a href=\"%2\">Privacy Policy</a>.")
                                                  .arg(build::is_icq() ? TermsUrlICQ : TermsUrlAgent)
                                                  .arg(build::is_icq() ? PrivacyPolicyUrlICQ : PrivacyPolicyUrlAgent),
                                                  options,
                                                  nullptr);

        auto generalDialog = std::make_unique<GeneralDialog>(termsWidget,
                                                             Utils::InterConnector::instance().getMainWindow(),
                                                             true,
                                                             false,
                                                             false);

        Testing::setAccessibleName(termsWidget, qsl("AS lp termsWidget"));
        termsWidget->setContainingDialog(generalDialog.get());
        connect(termsWidget, &TermsPrivacyWidget::agreementAccepted,
                this, [this, &generalDialog](const TermsPrivacyWidget::AcceptParams & _params)
        {
            setGDPRacceptedThisSession(_params.accepted_);
            generalDialog->close();
        });

        generalDialog->setModal(true);
        generalDialog->setWindowModality(Qt::ApplicationModal);
        generalDialog->setIgnoreClicksInSemiWindow(true);
        generalDialog->setIgnoredKeys({ Qt::Key_Escape });
        generalDialog->showInCenter();
    }

    void LoginPage::loginResultAttachUin(int64_t _seq, int _result)
    {
        if (_seq != sendSeq_)
            return;
        updateErrors(_result);
        if (_result == 0)
        {
            emit attached();
        }
    }

    void LoginPage::loginResultAttachPhone(int64_t _seq, int _result)
    {
        if (_seq != sendSeq_)
            return;
        updateErrors(_result);
        if (_result == 0)
        {
            emit attached();
        }
    }

    void LoginPage::clearErrors(bool ignorePhoneInfo/* = false*/)
    {
        errorLabel->hide();

        Utils::ApplyStyle(uinEdit, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
        Utils::ApplyStyle(codeEdit, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));
        Utils::ApplyStyle(phone_, Styling::getParameters().getLineEditCustomQss(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT)));

        emit country(countryCode_->text());

        if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX && !ignorePhoneInfo)
        {
            if (phone_->text().length() >= 3)
            {
                phoneInfoRequestsCount_++;

                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("phone", countryCode_->text() % phone_->text());
                collection.set_value_as_qstring("gui_locale", get_gui_settings()->get_value(settings_language, QString()));
                phoneInfoLastRequestId_ = GetDispatcher()->post_message_to_core("phoneinfo", collection.get());
            }
            else
            {
                phoneInfoLastRequestId_ = 0;
                receivedPhoneInfo_ = Data::PhoneInfo();
            }
            QPalette pal(phone_->palette());
            pal.setColor(QPalette::Text, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            phone_->setPalette(pal);
        }
        else
        {
            phoneInfoRequestsCount_ = 0;
            phoneInfoLastRequestId_ = 0;
        }
    }

    void LoginPage::phoneInfoResult(qint64 _seq, const Data::PhoneInfo& _data)
    {
        phoneInfoRequestsCount_--;

        if (phoneInfoLastRequestId_ == _seq || phoneInfoLastRequestSpecialId_ == _seq)
        {
            receivedPhoneInfo_ = _data;

            if (receivedPhoneInfo_.isValid() && !receivedPhoneInfo_.modified_phone_number_.empty())
            {
                auto code = countryCode_->text();
                if (!receivedPhoneInfo_.info_iso_country_.empty())
                {
                    combobox_->selectItem(Utils::getCountryNameByCode(QString::fromStdString(receivedPhoneInfo_.info_iso_country_)));
                }
                else if (!receivedPhoneInfo_.modified_prefix_.empty())
                {
                    auto modified_prefix = QString::fromStdString(receivedPhoneInfo_.modified_prefix_);
                    if (code != modified_prefix)
                    {
                        code = modified_prefix;
                        combobox_->selectItem(modified_prefix.remove(ql1c('+')));
                    }

                }
                phoneChangedAuto_ = true;
                phone_->setText(QString::fromStdString(receivedPhoneInfo_.modified_phone_number_).remove(0, code.length()));
                phoneChangedAuto_ = false;
            }
        }

        if (phoneInfoLastRequestSpecialId_ == _seq)
        {
            phoneInfoLastRequestSpecialId_ = 0;
        }
    }

    void LoginPage::setErrorText(int _result)
    {
        setFocus();
        const auto wrongEmailError = (build::is_dit() || build::is_biz())
                                     ? QT_TRANSLATE_NOOP("login_page", "Wrong Email or password. Please try again.")
                                     : QT_TRANSLATE_NOOP("login_page", "Wrong UIN/Email or password. Please try again.");
        switch (_result)
        {
        case core::le_wrong_login:
            errorLabel->setText(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX ?
                wrongEmailError
                : QT_TRANSLATE_NOOP("login_page","You have entered an invalid code. Please try again."));
            GetDispatcher()->post_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
                ? core::stats::stats_event_names::reg_error_uin
                : core::stats::stats_event_names::reg_error_code);
            GetDispatcher()->post_im_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
                ? core::stats::im_stat_event_names::reg_error_uin
                : core::stats::im_stat_event_names::reg_error_code);
            break;
        case core::le_wrong_login_2x_factor:
            errorLabel->setText(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX ?
                QT_TRANSLATE_NOOP("login_page", "Two-factor authentication is on, please create an app password <a href=\"https://e.mail.ru/settings/2-step-auth\">here</a> to login")
                : QT_TRANSLATE_NOOP("login_page","You have entered an invalid code. Please try again."));
            GetDispatcher()->post_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
                ? core::stats::stats_event_names::reg_error_uin
                : core::stats::stats_event_names::reg_error_code);
            GetDispatcher()->post_im_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
                ? core::stats::im_stat_event_names::reg_error_uin
                : core::stats::im_stat_event_names::reg_error_code);
            break;
        case core::le_error_validate_phone:
            errorLabel->setText(QT_TRANSLATE_NOOP("login_page","Invalid phone number. Please try again."));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_error_phone);
            break;
        case core::le_success:
            errorLabel->setText(QString());
            break;
        case core::le_attach_error_busy_phone:
            errorLabel->setText(QT_TRANSLATE_NOOP("sidebar","This phone number is already attached to another account.\nPlease edit phone number and try again."));
            timer_->stop();
            resendButton->setText(QT_TRANSLATE_NOOP("login_page", "Resend code"));
            resendButton->setEnabled(false);
            break;
        default:
            errorLabel->setText(QT_TRANSLATE_NOOP("login_page","Error occurred, try again later"));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_error_other);
            break;
        }
    }

    void LoginPage::setErrorText(const QString& _customError)
    {
        errorLabel->setText(_customError);
    }

    void LoginPage::codeEditChanged(const QString& _code)
    {
        if (_code.length() == codeLength_)
        {
            setFocus();
            codeEdit->setEnabled(false);
            get_gui_settings()->set_value(settings_keep_logged_in, true);
            gui_coll_helper collection(GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("phone", phone_->text());
            collection.set_value_as_qstring("sms_code", _code);
            collection.set_value_as_bool("save_auth_data", true);
            collection.set_value_as_bool("is_login", isLogin_);
            sendSeq_ = GetDispatcher()->post_message_to_core("login_by_phone", collection.get());
        }
    }

    void LoginPage::countryCodeChanged(const QString& _text)
    {
        if (!_text.isEmpty() && _text != ql1s("+"))
        {
            combobox_->selectItem(_text);
            if (!combobox_->containsCode(_text))
            {
                countryCode_->setText(prevCountryCode_);
                if (phone_->text().isEmpty())
                    phone_->setText(_text.mid(prevCountryCode_.length(), _text.length() - prevCountryCode_.length()));
                phone_->setFocus();
            }
        }

        prevCountryCode_ = _text;
    }

    void LoginPage::emptyPhoneRemove()
    {
        countryCode_->setFocus();
        QString code = countryCode_->text();
        countryCode_->setText(code.isEmpty() ? code : code.left(code.length() - 1));
    }

    void LoginPage::openProxySettings()
    {
        auto connection_settings_widget_ = new ConnectionSettingsWidget(this);
        connection_settings_widget_->show();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::proxy_open);
    }
}
