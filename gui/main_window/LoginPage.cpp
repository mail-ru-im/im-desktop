#include "stdafx.h"
#include "LoginPage.h"

#include "../common.shared/string_utils.h"
#include "../core_dispatcher.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../controls/ConnectionSettingsWidget.h"

#include "../controls/TransparentScrollBar.h"
#include "../controls/LabelEx.h"
#include "../controls/LineEditEx.h"
#include "../controls/CommonInput.h"
#include "../controls/TextEditEx.h"
#include "../controls/GeneralDialog.h"
#include "../controls/CustomButton.h"
#include "../controls/DialogButton.h"
#include "../controls/CheckboxList.h"
#include "../controls/TextWidget.h"
#include "../controls/ContextMenu.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../omicron/omicron_helper.h"
#include "../utils/features.h"
#include "TermsPrivacyWidget.h"
#include "UserAgreementWidget.h"
#include "MainWindow.h"
#include "app_config.h"
#include "../styles/ThemeParameters.h"
#include "styles/StyleSheetContainer.h"
#include "styles/StyleSheetGenerator.h"
#include "../common.shared/config/config.h"
#include "sidebar/SidebarUtils.h"
#include "../cache/emoji/Emoji.h"
#include "../cache/countries.h"
#include "utils/PhoneFormatter.h"
#include "IntroduceYourself.h"
#include "settings/ContactUs.h"
#include "url_config.h"

#ifdef HAS_OAUTH2
#include "OAuth2Dialog.h"
#endif //HAS_OAUTH2

namespace
{
    enum Pages
    {
        LOGIN,
        GDPR,
        USER_AGREEMENT
    };

    qint64 phoneInfoLastRequestSpecialId_ = 0; // using for settings only (last entered uin/phone)
    qint64 phoneInfoLastRequestId_ = 0;
    int phoneInfoRequestsCount_ = 0;

    constexpr QSize kOAuthLogoSize(200, 200);
    constexpr QSize kOAuthButtonSize(240, 40);
    constexpr QSize kOAuthButtonIconSize(24, 24);
    constexpr QColor kOAuthButtonBackColor(0, 95, 249);
    constexpr QColor kOAuthButtonHoverColor(0, 88, 229);
    constexpr QColor kOAuthButtonTextColor(255, 255, 255);
    constexpr QColor kOAuthButtonLogoColor(255, 158, 58);


    int controlsWidth() noexcept
    {
        return Utils::scale_value(300);
    }

    auto getResendButtonStyle()
    {
        const auto style =
            qsl("QPushButton:disabled { font-size: 16dip; background-color: %4; min-height: 30dip; max-height: 30dip; min-width: 30dip; max-width: %1dip; color: %2; border-style: none; outline: none; } "
                "QPushButton:enabled { font-size: 16dip; background-color:%4; min-height: 30dip; max-height: 30dip; min-width: 30dip; max-width: %1dip; color: %3; border-style: none; outline: none; } ")
                .arg(QString::number(Utils::unscale_value(controlsWidth())));
        return std::make_unique<Styling::ArrayStyleSheetGenerator>(
            style,
            std::vector<Styling::ThemeColorKey> {
                                                  Styling::ThemeColorKey { Styling::StyleVariable::BASE_PRIMARY },
                                                  Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY},
                                                  Styling::ThemeColorKey { Styling::StyleVariable::BASE_GLOBALWHITE } });
    }

    QString getPasswordLink()
    {
        return Features::passwordRecoveryLink();
    }

    enum class HintType
    {
        EnterEmailOrUin,
        EnterEmail,
        EnterPhone,
        EnterSMSCode,
        EnterPhoneCode,
        EnterEmailOAuth2,
        EnterOTPEmail,
        EnterOTPPassword,
        LoginByPhoneCall
    };

    QString hintLabelText(HintType _type)
    {
        switch (_type) {
        case HintType::EnterEmailOrUin:
            return QT_TRANSLATE_NOOP("login_page", "Enter your login and password");
        case HintType::EnterEmail:
        {
            if (config::get().is_on(config::features::explained_forgot_password))
                return QT_TRANSLATE_NOOP("login_page", "To log in use you corporate account created in");
            else
                return QT_TRANSLATE_NOOP("login_page", "Enter your Email");
        }
        case HintType::EnterPhone:
            return QT_TRANSLATE_NOOP("login_page", "Check the country code and enter your phone number for registration and authorization");
        case HintType::EnterSMSCode:
            return QT_TRANSLATE_NOOP("login_page", "Verification code was sent to number");
        case HintType::EnterEmailOAuth2:
            return QT_TRANSLATE_NOOP("login_page", "Enter your corporate email");
        case HintType::EnterOTPEmail:
            return QT_TRANSLATE_NOOP("login_page", "Enter your corporate email, a password will be sent to it");
        case HintType::EnterOTPPassword:
            return QT_TRANSLATE_NOOP("login_page", "Enter one-time password sent to email");
        case HintType::EnterPhoneCode:
        case HintType::LoginByPhoneCall:
            return QT_TRANSLATE_NOOP("login_page", "We called the number");
        default:
            break;
        }

        return QString();
    }

    QString getTitleText(HintType _type)
    {
        switch (_type) {
        case HintType::EnterEmailOrUin:
        case HintType::EnterEmail:
            return QT_TRANSLATE_NOOP("login_page", "Sign in by password");
        case HintType::EnterPhone:
            return QT_TRANSLATE_NOOP("login_page", "Phone");
        case HintType::EnterSMSCode:
            return QT_TRANSLATE_NOOP("login_page", "SMS code");
        case HintType::EnterPhoneCode:
            return QT_TRANSLATE_NOOP("login_page", "Phone code");
        case HintType::EnterEmailOAuth2:
        case HintType::EnterOTPEmail:
            return QT_TRANSLATE_NOOP("login_page", "Enter email");
        case HintType::EnterOTPPassword:
            return QT_TRANSLATE_NOOP("login_page", "Enter password");
        case HintType::LoginByPhoneCall:
            return QT_TRANSLATE_NOOP("login_page", "Enter last 6 digits of number that called you");
        default:
            break;
        }

        return QString();
    }

    int errorLabelMinHeight(const int _linesCount)
    {
        return Utils::scale_value(24) * _linesCount;
    }

    QString getFormattedTime(int _seconds)
    {
        int minutes = _seconds / 60;
        int seconds = _seconds % 60;

        return QString::number(minutes) % u':' % QString(qsl("%1")).arg(seconds, 2, 10, ql1c('0'));
    }

    const auto fontSize = 16;
    const auto titleFontSize = 23;

    QFont getHintFont()
    {
        return Fonts::appFontScaled(fontSize);
    }

    int getLineSpacing()
    {
        return 0.44*fontSize;
    }

    int getMinWindowWidth()
    {
        return Utils::scale_value(380);
    }

    constexpr int backIconSize = 20;
    constexpr int buttonSize = 24;
    constexpr int buttonSizeBig = 32;

    int buttonHorMargin()
    {
        return Utils::scale_value(32);
    }

    int topButtonWidgetHeight()
    {
        return Utils::scale_value(44);
    }

    QString loginMailLink()
    {
        if (const auto mail_win = Ui::getUrlConfig().getUrlMailWin();
                !mail_win.isEmpty() &&
                config::get().is_on(config::features::otp_login_open_mail_link))
            return u"https://" % mail_win;

        return {};
    }
}

namespace Ui
{
    LoginPage::LoginPage(QWidget* _parent)
        : QWidget(_parent)
        , termsWidget_(nullptr)
        , timer_(new QTimer(this))
        , remainingSeconds_(0)
        , changePageButton_(nullptr)
        , sendSeq_(0)
        , codeLength_(6)
        , phoneChangedAuto_(false)
        , gdprAccepted_(false)
        , userAgreementAccepted_(false)
        , loggedIn_(false)
        , smsCodeSendCount_(0)
    {
        Styling::setStyleSheet(this, Styling::getParameters().getLoginPageQss());
        auto loginPageLayout = Utils::emptyHLayout(this);

        mainStakedWidget_ = new QStackedWidget();
        mainStakedWidget_->setMinimumWidth(getMinWindowWidth());
        mainStakedWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        Testing::setAccessibleName(mainStakedWidget_, qsl("AS LoginPage mainStakedWidget"));
        loginPageLayout->addWidget(mainStakedWidget_);

        QWidget* loginWidget = new QWidget(this);
        Testing::setAccessibleName(loginWidget, qsl("AS LoginPage loginWidget"));
        QVBoxLayout* verticalLayout = Utils::emptyVLayout(loginWidget);

        auto topButtonWidget = new QWidget(this);
        Testing::setAccessibleName(topButtonWidget, qsl("AS LoginPage topButtonWidget"));
        auto topButtonLayout = Utils::emptyHLayout(topButtonWidget);
        topButtonWidget->setStyleSheet(qsl("border: none; background-color: transparent;"));
        topButtonWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        topButtonWidget->setFixedHeight(topButtonWidgetHeight());
        topButtonLayout->setContentsMargins(Utils::scale_value(20), Utils::scale_value(12), Utils::scale_value(12), 0);
        topButtonLayout->setSpacing(Utils::scale_value(8));

        changePageButton_ = new CustomButton(topButtonWidget);
        Styling::Buttons::setButtonDefaultColors(changePageButton_);
        changePageButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        changePageButton_->setMinimumSize(Utils::scale_value(QSize(buttonSize, buttonSize)));
        changePageButton_->setFont(Fonts::appFontScaled(15));
        Testing::setAccessibleName(changePageButton_, qsl("AS LoginPage changePageButton"));
        topButtonLayout->addWidget(changePageButton_);

        {
            auto buttonsSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            topButtonLayout->addItem(buttonsSpacer);
        }

        proxySettingsButton_ = new CustomButton(topButtonWidget, qsl(":/settings_icon_big"), QSize(buttonSizeBig, buttonSizeBig));
        proxySettingsButton_->setObjectName(qsl("settingsButton"));
        proxySettingsButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        proxySettingsButton_->setFocusPolicy(Qt::NoFocus);
        Styling::Buttons::setButtonDefaultColors(proxySettingsButton_);
        Testing::setAccessibleName(proxySettingsButton_, qsl("AS LoginPage proxySettingsButton"));
        topButtonLayout->addWidget(proxySettingsButton_);

        reportButton_ = new CustomButton(topButtonWidget, qsl(":/settings/report"), QSize(buttonSize, buttonSize));
        reportButton_->setObjectName(qsl("settingsButton"));
        reportButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        reportButton_->setFocusPolicy(Qt::NoFocus);
        Styling::Buttons::setButtonDefaultColors(reportButton_);
        Testing::setAccessibleName(reportButton_, qsl("AS LoginPage reportButton"));
        topButtonLayout->addWidget(reportButton_);
        verticalLayout->addWidget(topButtonWidget);

        QWidget* mainVerWidget = new QWidget(this);
        Testing::setAccessibleName(mainVerWidget, qsl("AS LoginPage mainVerWidget"));
        QVBoxLayout* verticalIntroLayout = Utils::emptyVLayout(mainVerWidget);
        topSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        verticalIntroLayout->addItem(topSpacer_);

        auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(loginWidget);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet(qsl("background: transparent; border: none;"));
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::grabTouchWidget(scrollArea->viewport(), true);
        Testing::setAccessibleName(scrollArea, qsl("AS LoginPage scrollArea"));
        verticalLayout->addWidget(scrollArea);
        scrollArea->setWidget(mainVerWidget);

        auto mainWidget = new QWidget(mainVerWidget);
        mainWidget->setObjectName(qsl("mainWidget"));
        mainWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto mainLayout = Utils::emptyHLayout(mainWidget);

        controlsWidget_ = new QWidget(mainWidget);
        controlsWidget_->setObjectName(qsl("controlsWidget"));
        controlsWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        Testing::setAccessibleName(controlsWidget_, qsl("AS LoginPage controlsWidget"));
        auto controlsLayout = Utils::emptyVLayout(controlsWidget_);

        hintsWidget_ = new QWidget(controlsWidget_);
        hintsWidget_->setObjectName(qsl("hintsWidget"));
        hintsWidget_->setMinimumWidth(controlsWidth());
        hintsWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        Testing::setAccessibleName(hintsWidget_, qsl("AS LoginPage hintsWidget"));
        auto hintsLayout = Utils::emptyVLayout(hintsWidget_);

        titleLabel_ = new TextWidget(hintsWidget_, getTitleText(HintType::EnterEmailOrUin));
        titleLabel_->init({ Fonts::appFontScaled(titleFontSize, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
        titleLabel_->setObjectName(qsl("hint"));
        titleLabel_->setAlignment(TextRendering::HorAligment::CENTER);
        titleLabel_->setFixedWidth(controlsWidth());
        Testing::setAccessibleName(titleLabel_, qsl("AS LoginPage titleLabel"));
        hintsLayout->addWidget(titleLabel_);
        hintsLayout->setAlignment(titleLabel_, Qt::AlignBottom | Qt::AlignHCenter);
        hintsLayout->addItem(new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Fixed, QSizePolicy::Fixed));

        entryHint_ = new EntryHintWidget(hintsWidget_, hintLabelText(HintType::EnterEmailOrUin));
        Testing::setAccessibleName(entryHint_, qsl("AS LoginPage entryHint"));
        hintsLayout->addWidget(entryHint_);

        controlsLayout->addWidget(hintsWidget_);

        QWidget* passwordLoginWidget = new QWidget(this);
        passwordLoginWidget->setObjectName(qsl("passwordLoginWidget"));
        passwordLoginWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
        passwordLoginWidget->setMinimumWidth(controlsWidth() + Utils::scale_value(14));
        Testing::setAccessibleName(passwordLoginWidget, qsl("AS LoginPage passwordLoginWidget"));
        auto passwordLoginLayout = Utils::emptyVLayout(passwordLoginWidget);

        passwordLoginLayout->addItem(new QSpacerItem(0, Utils::scale_value(17), QSizePolicy::Minimum, QSizePolicy::Fixed));
        uinInput_ = new CommonInputContainer(passwordLoginWidget);
        uinInput_->setPlaceholderText(QT_TRANSLATE_NOOP("login_page", "Login"));
        uinInput_->setText(get_gui_settings()->get_value(login_page_last_entered_uin, QString()));
        uinInput_->setAttribute(Qt::WA_MacShowFocusRect, false);
        uinInput_->setFixedWidth(controlsWidth());
        Testing::setAccessibleName(uinInput_, qsl("AS LoginPage uinInput"));
        passwordLoginLayout->addWidget(uinInput_);
        passwordLoginLayout->setAlignment(uinInput_, Qt::AlignTop | Qt::AlignHCenter);
        passwordLoginLayout->addItem(new QSpacerItem(0, Utils::scale_value(6), QSizePolicy::Minimum, QSizePolicy::Fixed));

        passwordInput_ = new CommonInputContainer(passwordLoginWidget, Ui::InputRole::Password);
        passwordInput_->setPlaceholderText(QT_TRANSLATE_NOOP("login_page", "Password"));
        passwordInput_->setFixedWidth(controlsWidth());
        Testing::setAccessibleName(passwordInput_, qsl("AS LoginPage passwordInput"));
        passwordLoginLayout->addWidget(passwordInput_);
        passwordLoginLayout->setAlignment(passwordInput_, Qt::AlignTop | Qt::AlignHCenter);

        buttonsWidget_ = new QWidget(controlsWidget_);
        buttonsWidget_->setObjectName(qsl("buttonsWidget"));
        buttonsWidget_->setMinimumWidth(controlsWidth());
        buttonsWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        Testing::setAccessibleName(buttonsWidget_, qsl("AS LoginPage buttonsWidget"));
        auto buttonsLayout = Utils::emptyVLayout(buttonsWidget_);
        buttonsLayout->setAlignment(Qt::AlignHCenter);
        buttonsLayout->addItem(new QSpacerItem(0, Utils::scale_value(5), QSizePolicy::Minimum, QSizePolicy::Fixed));

        errorWidget_ = new QWidget(buttonsWidget_);
        errorWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        Testing::setAccessibleName(errorWidget_, qsl("AS LoginPage errorWidget"));
        auto errorLayout = Utils::emptyVLayout(errorWidget_);
        errorLayout->setAlignment(Qt::AlignCenter);

        errorLabel_ = new LabelEx(errorWidget_);
        errorLabel_->setFixedWidth(controlsWidth());
        errorLabel_->setAlignment(Qt::AlignCenter);
        errorLabel_->setWordWrap(true);
        errorLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        errorLabel_->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        errorLabel_->setOpenExternalLinks(true);
        errorLabel_->setFont(Fonts::appFontScaled(15));
        errorLabel_->setColor(Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION });

        Testing::setAccessibleName(errorLabel_, qsl("AS LoginPage errorLabel"));
        errorLayout->addWidget(errorLabel_);
        buttonsLayout->addWidget(errorWidget_);
        buttonsLayout->addItem(new QSpacerItem(0, Utils::scale_value(12), QSizePolicy::Minimum, QSizePolicy::Fixed));

        resendButton_ = new QPushButton(buttonsWidget_);
        Styling::setStyleSheet(resendButton_, getResendButtonStyle());
        resendButton_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        resendButton_->setCursor(Qt::PointingHandCursor);

        resendMenu_ = new ContextMenu(this, 20);
        resendMenu_->addActionWithIcon(qsl(":/context_menu/notice"), QT_TRANSLATE_NOOP("login_page", "Send code"), this, [this]() { sendCode(); });
        resendMenu_->addActionWithIcon(qsl(":/context_menu/call"), QT_TRANSLATE_NOOP("login_page", "Dictate over the phone"), this, [this]() { callPhone(); });
        Testing::setAccessibleName(resendButton_, qsl("AS LoginPage resendCodeButton"));
        Testing::setAccessibleName(resendMenu_, qsl("AS LoginPage resendCodeMenu"));
        buttonsLayout->addWidget(resendButton_);
        buttonsLayout->setAlignment(resendMenu_, Qt::AlignHCenter);

        keepLoggedWidget_ = new QWidget(buttonsWidget_);
        keepLoggedWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        Testing::setAccessibleName(keepLoggedWidget_, qsl("AS LoginPage keepLoggedWidget"));
        auto keepLoggedLayout = Utils::emptyVLayout(keepLoggedWidget_);
        keepLoggedLayout->setAlignment(Qt::AlignHCenter);

        keepLogged_ = new Ui::CheckBox(keepLoggedWidget_, Qt::AlignHCenter);
        keepLogged_->setText(QT_TRANSLATE_NOOP("login_page", "Keep me signed in"));
        if (Features::isOAuth2LoginAllowed())
        {
            keepLogged_->setChecked(true);
            updateKeedLoggedInSetting(true);
        }
        else
        {
            keepLogged_->setChecked(get_gui_settings()->get_value(settings_keep_logged_in, settings_keep_logged_in_default()));
        }
        keepLogged_->setFixedWidth(controlsWidth());

        Testing::setAccessibleName(keepLogged_, qsl("AS LoginPage keepLogged"));
        keepLoggedLayout->addWidget(keepLogged_);
        keepLoggedLayout->addItem(new QSpacerItem(0, Utils::scale_value(28), QSizePolicy::Minimum, QSizePolicy::Fixed));
        buttonsLayout->addWidget(keepLoggedWidget_);

        nextButton_ = new DialogButton(buttonsWidget_, QT_TRANSLATE_NOOP("login_page", "Sign in"), Ui::DialogButtonRole::DEFAULT);
        nextButton_->setCursor(QCursor(Qt::PointingHandCursor));
        nextButton_->setFixedHeight(Utils::scale_value(40));
        nextButton_->setFont(Fonts::appFontScaled(fontSize, Fonts::FontWeight::Medium));
        nextButton_->setContentsMargins(buttonHorMargin(), Utils::scale_value(9), buttonHorMargin(), Utils::scale_value(11));
        nextButton_->adjustSize();
        Testing::setAccessibleName(nextButton_, qsl("AS LoginPage nextButton"));
        buttonsLayout->addWidget(nextButton_);
        buttonsLayout->setAlignment(nextButton_, Qt::AlignHCenter);

        forgotPasswordWidget_ = new QWidget(buttonsWidget_);
        forgotPasswordWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto forgotPasswordLayout = Utils::emptyVLayout(forgotPasswordWidget_);
        forgotPasswordLayout->setAlignment(Qt::AlignHCenter);
        forgotPasswordLayout->addItem(new QSpacerItem(0, Utils::scale_value(24), QSizePolicy::Minimum, QSizePolicy::Fixed));

        forgotPasswordLabel_ = new LabelEx(forgotPasswordWidget_);
        forgotPasswordLabel_->setText(QT_TRANSLATE_NOOP("login_page", "Forgot password?"));
        forgotPasswordLabel_->setFont(Fonts::appFontScaled(fontSize));
        forgotPasswordLabel_->setColors(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY },
            Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_HOVER },
            Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_ACTIVE });
        forgotPasswordLabel_->setCursor(Qt::PointingHandCursor);

        Testing::setAccessibleName(forgotPasswordLabel_, qsl("AS LoginPage forgotPasswordLabel"));
        forgotPasswordLayout->addWidget(forgotPasswordLabel_);
        forgotPasswordLayout->setAlignment(forgotPasswordLabel_, Qt::AlignHCenter);
        buttonsLayout->addWidget(forgotPasswordWidget_);
        buttonsLayout->setAlignment(forgotPasswordWidget_, Qt::AlignHCenter);

        QWidget* phoneLoginWidget = new QWidget(this);
        phoneLoginWidget->setObjectName(qsl("phoneLoginWidget"));
        phoneLoginWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
        phoneLoginWidget->setMinimumWidth(controlsWidth() + Utils::scale_value(14));
        auto phoneLoginLayout = Utils::emptyVLayout(phoneLoginWidget);

        phone_ = new PhoneInputContainer(phoneLoginWidget);
        phone_->setFixedWidth(controlsWidth());
        phoneLoginLayout->addItem(new QSpacerItem(0, Utils::scale_value(14), QSizePolicy::Minimum, QSizePolicy::Fixed));
        Testing::setAccessibleName(phone_, qsl("AS LoginPage phoneContainer"));
        phoneLoginLayout->addWidget(phone_);
        phoneLoginLayout->setAlignment(phone_, Qt::AlignTop | Qt::AlignHCenter);

        QWidget* phoneConfirmWidget = new QWidget(this);
        phoneConfirmWidget->setObjectName(qsl("phoneConfirmWidget"));
        phoneConfirmWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
        phoneConfirmWidget->setMinimumWidth(controlsWidth() + Utils::scale_value(14));
        auto phoneConfirmLayout = Utils::emptyVLayout(phoneConfirmWidget);

        codeInput_ = new CodeInputContainer(phoneConfirmWidget);
        codeInput_->setObjectName(qsl("code"));
        codeInput_->setFixedWidth(controlsWidth());
        codeInput_->setAttribute(Qt::WA_MacShowFocusRect, false);
        Testing::setAccessibleName(codeInput_, qsl("AS LoginPage smsCodeInput"));
        phoneConfirmLayout->addItem(new QSpacerItem(0, Utils::scale_value(14), QSizePolicy::Minimum, QSizePolicy::Fixed));
        phoneConfirmLayout->addWidget(codeInput_);
        phoneConfirmLayout->setAlignment(codeInput_, Qt::AlignTop | Qt::AlignHCenter);

        QWidget* introduceWidget = new QWidget(this);
        Testing::setAccessibleName(introduceWidget, qsl("AS LoginPage introduceWidget"));
        auto introduceLayout = Utils::emptyVLayout(introduceWidget);
        introduceWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
        introduceMyselfWidget_ = new IntroduceYourself(MyInfo()->aimId(), QString(), introduceWidget);
        introduceWidget->setMinimumWidth(controlsWidth() + Utils::scale_value(14));
        Testing::setAccessibleName(introduceMyselfWidget_, qsl("AS LoginPage introduceMyselfWidget"));
        introduceLayout->addWidget(introduceMyselfWidget_);
        introduceLayout->setAlignment(introduceMyselfWidget_, Qt::AlignCenter);

        QWidget* reportWidget = new QWidget(this);
        Testing::setAccessibleName(reportWidget, qsl("AS LoginPage reportWidget"));
        auto reportLayout = Utils::emptyVLayout(reportWidget);
        reportWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
        contactUsWidget_ = new ContactUsWidget(reportWidget, true);
        Testing::setAccessibleName(contactUsWidget_, qsl("AS ContactUsPage"));
        reportWidget->setMinimumWidth(controlsWidth() + Utils::scale_value(14));
        contactUsWidget_->setFixedHeight(Utils::scale_value(410)); // crutch bc of scrollarea in ContactUsWidget
        reportLayout->addWidget(contactUsWidget_);
        reportLayout->setAlignment(contactUsWidget_, Qt::AlignCenter);

        QWidget* oauth2LoginWidget = new QWidget(this);
        oauth2LoginWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        Testing::setAccessibleName(oauth2LoginWidget, qsl("AS LoginPage oauth2LoginWidget"));

        QLabel* logoLabel = new QLabel(qsl("OAuth2"), oauth2LoginWidget);
        logoLabel->setScaledContents(true);
        logoLabel->setFixedSize(Utils::scale_value(kOAuthLogoSize));
        logoLabel->setFrameShape(QFrame::NoFrame);
        logoLabel->setAlignment(Qt::AlignCenter);
        logoLabel->setPixmap(QIcon(qsl(":/oauth2_logo")).pixmap(Utils::scale_value(kOAuthLogoSize)));
        logoLabel->setFixedSize(Utils::scale_value(kOAuthLogoSize));

        loginButton_ = new CustomButton(oauth2LoginWidget, qsl(":/logo_icon"));
        loginButton_->setSpinnerImage(qsl(":/spin_icon"));
        loginButton_->setIconSize(Utils::scale_value(kOAuthButtonIconSize));
        loginButton_->setIconAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        loginButton_->setIconOffsets(Utils::scale_value(32), 0);
        loginButton_->setTextLeftOffset(Utils::scale_value(8));
        loginButton_->setShape(ButtonShape::ROUNDED_RECTANGLE);
        loginButton_->setAttribute(Qt::WA_Hover);
        loginButton_->setNormalTextColor(Styling::ColorParameter{ kOAuthButtonTextColor });
        loginButton_->setHoveredTextColor(Styling::ColorParameter{ kOAuthButtonTextColor });
        loginButton_->setPressedTextColor(Styling::ColorParameter{ kOAuthButtonTextColor });

        loginButton_->setDefaultColor(Styling::ColorParameter{ kOAuthButtonLogoColor });
        loginButton_->setActiveColor(Styling::ColorParameter{ kOAuthButtonLogoColor });
        loginButton_->setDisabledColor(Styling::ColorParameter{ kOAuthButtonLogoColor });
        loginButton_->setHoverColor(Styling::ColorParameter{ kOAuthButtonLogoColor });
        loginButton_->setPressedColor(Styling::ColorParameter{ kOAuthButtonLogoColor });

        loginButton_->setBackgroundHovered(Styling::ColorParameter{ kOAuthButtonHoverColor });
        loginButton_->setBackgroundNormal(Styling::ColorParameter{ kOAuthButtonBackColor });
        loginButton_->setBackgroundPressed(Styling::ColorParameter{ kOAuthButtonBackColor });
        loginButton_->setBackgroundDisabled(Styling::ColorParameter{ kOAuthButtonBackColor });

        loginButton_->setText(QT_TRANSLATE_NOOP("login_page", "Sign in with Mail.ru"));
        loginButton_->setCursor(Qt::PointingHandCursor);
        loginButton_->setFont(Fonts::appFontScaled(15, Fonts::FontWeight::Medium));
        loginButton_->setFixedHeight(Utils::scale_value(kOAuthButtonSize.height()));
        loginButton_->setMinimumSize(Utils::scale_value(kOAuthButtonSize));
        connect(loginButton_, &QPushButton::clicked, this, &LoginPage::openOAuth2Dialog);

        QVBoxLayout* oauth2LoginLayout = new QVBoxLayout(oauth2LoginWidget);
        oauth2LoginLayout->setContentsMargins(QMargins());
        oauth2LoginLayout->setSpacing(0);
        oauth2LoginLayout->addWidget(logoLabel, 0, Qt::AlignHCenter);
        oauth2LoginLayout->addSpacing(Utils::scale_value(52));
        oauth2LoginLayout->addWidget(loginButton_, 0, Qt::AlignHCenter);
        oauth2LoginLayout->addSpacing(Utils::scale_value(4));

        loginStakedWidget_ = new QStackedWidget(controlsWidget_);
        loginStakedWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        loginStakedWidget_->setObjectName(qsl("stackedWidget"));
        loginStakedWidget_->installEventFilter(this);
        Testing::setAccessibleName(phoneLoginWidget, qsl("AS LoginPage phoneLoginWidget"));
        loginStakedWidget_->addWidget(phoneLoginWidget);
        Testing::setAccessibleName(phoneConfirmWidget, qsl("AS LoginPage phoneConfirmWidget"));
        loginStakedWidget_->addWidget(phoneConfirmWidget);
        Testing::setAccessibleName(passwordLoginWidget, qsl("AS LoginPage passwordLoginWidget"));
        loginStakedWidget_->addWidget(passwordLoginWidget);
        Testing::setAccessibleName(introduceWidget, qsl("AS LoginPage introduceWidget"));
        loginStakedWidget_->addWidget(introduceWidget);
        Testing::setAccessibleName(reportWidget, qsl("AS LoginPage reportWidget"));
        loginStakedWidget_->addWidget(reportWidget);

        loginStakedWidget_->addWidget(oauth2LoginWidget);
        loginStakedWidget_->setCurrentIndex(0);


        Testing::setAccessibleName(loginStakedWidget_, qsl("AS LoginPage loginStakedWidget"));
        controlsLayout->addWidget(loginStakedWidget_);
        controlsLayout->setAlignment(loginStakedWidget_, Qt::AlignCenter);

        Testing::setAccessibleName(buttonsWidget_, qsl("AS LoginPage buttonsWidget"));
        controlsLayout->addWidget(buttonsWidget_);
        controlsLayout->setAlignment(buttonsWidget_, Qt::AlignHCenter);
        controlsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        mainLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
        mainLayout->addWidget(controlsWidget_);
        mainLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

        Testing::setAccessibleName(mainWidget, qsl("AS LoginPage mainWidget"));
        verticalIntroLayout->addWidget(mainWidget);
        bottomSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        verticalIntroLayout->addItem(bottomSpacer_);

        Testing::setAccessibleName(loginWidget, qsl("AS LoginPage loginWidget"));
        mainStakedWidget_->addWidget(loginWidget);

        {
            QWidget* gdprWidget = new QWidget(this);
            auto gdprLayout = Utils::emptyHLayout(gdprWidget);
            gdprLayout->setAlignment(Qt::AlignCenter);

            TermsPrivacyWidget::Options options;
            options.isGdprUpdate_ = false;
            const auto product = config::get().translation(config::translations::gdpr_welcome);

            termsWidget_ = new TermsPrivacyWidget(QT_TRANSLATE_NOOP("terms_privacy_widget", product.data()),
                QT_TRANSLATE_NOOP("terms_privacy_widget",
                    "By clicking \"Accept and agree\", you confirm that you have read carefully "
                    "and agree to our <a href=\"%1\">Terms</a> "
                    "and <a href=\"%2\">Privacy Policy</a>")
                .arg(legalTermsUrl(), privacyPolicyUrl()),
                options);
            if (termsWidget_)
            {
                termsWidget_->init();
                Testing::setAccessibleName(termsWidget_, qsl("AS GDPR termsWidget"));
                gdprLayout->addWidget(termsWidget_);
            }
            Testing::setAccessibleName(gdprWidget, qsl("AS GDPR gdprWidget"));
            mainStakedWidget_->addWidget(gdprWidget);
            mainStakedWidget_->layout()->setAlignment(gdprWidget, Qt::AlignHCenter);
        }

        connect(keepLogged_, &QCheckBox::toggled, &LoginPage::updateKeedLoggedInSetting);

        init();
        setFocus();
        nextButton_->setDefault(false);
        updateGeometry();
    }

    void LoginPage::keyPressEvent(QKeyEvent* _event)
    {
        if ((_event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter) &&
            (currentPage() == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX || currentPage() == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX))
        {
            if (nextButton_->isEnabled())
                nextButton_->click();
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

    void LoginPage::resizeEvent(QResizeEvent * _event)
    {
        QWidget::resizeEvent(_event);
        resizeSpacers();
    }

    void LoginPage::init()
    {
        connect(changePageButton_, &Ui::CustomButton::clicked, this, &LoginPage::switchLoginType);

        connect(loginStakedWidget_, &QStackedWidget::currentChanged, this, [this]()
        {
            updatePage();
        });

        connect(nextButton_, &DialogButton::clicked, this, &LoginPage::nextPage);
        connect(resendButton_, &QPushButton::clicked, this, &LoginPage::resendCode);

        connect(timer_, &QTimer::timeout, this, &LoginPage::updateTimer);

        connect(proxySettingsButton_, &QPushButton::clicked, this, &LoginPage::openProxySettings);
        connect(reportButton_, &QPushButton::clicked, this, [this]()
        {
            initLoginSubPage(LoginSubpage::SUBPAGE_REPORT);
        });
        connect(forgotPasswordLabel_, &LabelEx::clicked, this, []()
        {
            QDesktopServices::openUrl(QUrl(getPasswordLink()));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::login_forgot_password);
        });

        phone_->setAttribute(Qt::WA_MacShowFocusRect, false);

        connect(uinInput_, &Ui::CommonInputContainer::textChanged, this, [this]() { clearErrors(); }, Qt::DirectConnection);
        connect(passwordInput_, &Ui::CommonInputContainer::textEdited, this, [this]() { clearErrors(); }, Qt::DirectConnection);
        connect(passwordInput_, &Ui::CommonInputContainer::textChanged, this, &LoginPage::updateNextButton, Qt::DirectConnection);
        connect(codeInput_, &Ui::CodeInputContainer::textChanged, this, &LoginPage::codeEditChanged, Qt::DirectConnection);
        connect(phone_, &Ui::PhoneInputContainer::textChanged, this, &LoginPage::phoneTextChanged, Qt::DirectConnection);
        connect(entryHint_, &Ui::EntryHintWidget::changeClicked, this, [this]()
        {
            prevPage();
            stats_edit_phone();
        }, Qt::DirectConnection);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::getSmsResult, this, &LoginPage::getSmsResult, Qt::DirectConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::oauthRequired, this, &LoginPage::oauthRequired, Qt::DirectConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::loginResult, this, &LoginPage::loginResult, Qt::DirectConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::loginResultAttachPhone, this, &LoginPage::loginResultAttachPhone, Qt::DirectConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::phoneInfoResult, this, &LoginPage::phoneInfoResult, Qt::DirectConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, this, &LoginPage::onUrlConfigError);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::authError, this, &LoginPage::authError, Qt::QueuedConnection);

        connect(introduceMyselfWidget_, &IntroduceYourself::profileUpdated, this, [this]()
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_correct_name, {{ "type", statType(lastPage_) }});
            login();
            Logic::updatePlaceholders();
        });
        connect(introduceMyselfWidget_, &IntroduceYourself::avatarSet, this, [this]()
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_set_avatar, { {"do", "photo"}, {"type", statType(lastPage_)} });
        });

        errorLabel_->hide();
        phone_->setFocus();

        initGDPR();

        initLoginSubPage(LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX);

        const auto lastLoginPage = get_gui_settings()->get_value<int>(login_page_last_login_type, (int)LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX);
        lastPage_ = (LoginSubpage)lastLoginPage;
        if (!Features::isOAuth2LoginAllowed() && lastPage_ == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX)
            lastPage_ = LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX;
        if (Features::isOAuth2LoginAllowed())
            lastPage_ = LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX;
        else if (config::get().is_on(config::features::login_by_mail_default))
            lastPage_ = LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX;

        if (Features::loginMethod() == Features::LoginMethod::OTPViaEmail)
        {
            otpState_ = OTPAuthState::Email;
            if (Features::isOAuth2LoginAllowed())
                lastPage_ = LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX;
        }

        initLoginSubPage(lastPage_);

        prepareLoginByPhone();

        connect(MyInfo(), &my_info::received, this, [this]()
        {
            introduceMyselfWidget_->UpdateParams(MyInfo()->aimId(), MyInfo()->friendly());
            get_gui_settings()->set_value<bool>(login_page_need_fill_profile, MyInfo()->friendly().isEmpty());
            if (!MyInfo()->friendly().isEmpty())
                login();
        });

        connect(GetDispatcher(), &core_dispatcher::sessionStarted, this, [this](const QString& _aimId)
        {
            introduceMyselfWidget_->UpdateParams(_aimId, MyInfo()->friendly());
        });

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::guiSettings, this, [this]()
        {
            uinInput_->setText(get_gui_settings()->get_value(login_page_last_entered_uin, QString()));
            prepareLoginByPhone();
        });
    }

    void LoginPage::login()
    {
        core::stats::event_props_type props;
        props.push_back({ "type", statType(lastPage_) });
        props.push_back({ "how", lastPage_ == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX
                                 ? "password"
                                 : isCallCheck(checks_) ? "call_number" : "sms" });
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_login_success, props);

        get_gui_settings()->set_value<int>(login_page_last_login_type, (int)lastPage_);

        clearErrors();
        Q_EMIT loggedIn();
        get_gui_settings()->set_value<bool>(login_page_need_fill_profile, false);
        if (otpState_)
            updateOTPState(OTPAuthState::Email);

        phoneInfoRequestsCount_ = 0;
        phoneInfoLastRequestId_ = 0;
        receivedPhoneInfo_ = Data::PhoneInfo();
        loggedIn_ = true;
    }

    void LoginPage::initLoginSubPage(const LoginSubpage _index)
    {
        if (_index == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX || _index == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX || _index == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX)
            lastPage_ = _index;

        changePageButton_->setVisible((config::get().is_on(config::features::login_by_phone_allowed) && _index != LoginSubpage::SUBPAGE_PHONE_CONF_INDEX) || _index == LoginSubpage::SUBPAGE_REPORT);
        proxySettingsButton_->setVisible(_index == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX || _index == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX || _index == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX);
        reportButton_->setVisible(!Features::isContactUsViaBackend() && (_index == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX || _index == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX || _index == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX));
        titleLabel_->setVisible(true);
        entryHint_->setVisible(true);
        buttonsWidget_->setVisible(true);
        nextButton_->setVisible(_index != LoginSubpage::SUBPAGE_PHONE_CONF_INDEX && _index != LoginSubpage::SUBPAGE_INTRODUCEYOURSELF);
        codeInput_->clear();
        codeInput_->stopSpinner();
        keepLoggedWidget_->setVisible(false);
        forgotPasswordWidget_->setVisible(false);
        errorLabel_->setVisible(false);
        resendButton_->setVisible(_index == LoginSubpage::SUBPAGE_PHONE_CONF_INDEX);
        errorLabel_->setMinimumHeight(errorLabelMinHeight(1));
        contactUsWidget_->resetState(ClearData::Yes);
        hintsWidget_->setVisible(true);

        if (loginStakedWidget_->currentWidget())
        {
            loginStakedWidget_->currentWidget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            loginStakedWidget_->currentWidget()->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
        }

        switch (_index)
        {
        case LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX:
            titleLabel_->setText(getTitleText(HintType::EnterPhone));
            entryHint_->setText(hintLabelText(HintType::EnterPhone));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_page_phone);
            break;

        case LoginSubpage::SUBPAGE_PHONE_CONF_INDEX:
            titleLabel_->setText(getTitleText(isCallCheck(checks_) ? HintType::LoginByPhoneCall : HintType::EnterSMSCode));
            entryHint_->setText(hintLabelText(isCallCheck(checks_) ? HintType::LoginByPhoneCall : HintType::EnterSMSCode));
            entryHint_->appendPhone(PhoneFormatter::formatted(phone_->getPhoneCode() % phone_->getPhone()));
            entryHint_->appendLink(QT_TRANSLATE_NOOP("login_page", "Change"), ql1s("#"));
            codeInput_->setCodeLength(codeLength_);
            if (isCallCheck(checks_))
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_page_callui);
            break;

        case LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX:
            if (otpState_)
            {
                updateOTPState(otpState_ == OTPAuthState::Email ? OTPAuthState::Email : OTPAuthState::Password);
            }
            else
            {
                titleLabel_->setText(getTitleText(config::get().is_on(config::features::login_by_mail_default) ? HintType::EnterEmail : HintType::EnterEmailOrUin));
                entryHint_->setText(hintLabelText(config::get().is_on(config::features::login_by_mail_default) ? HintType::EnterEmail : HintType::EnterEmailOrUin));
                if (config::get().is_on(config::features::explained_forgot_password))
                {
                    entryHint_->appendLink(QT_TRANSLATE_NOOP("login_page", "Mail.ru for business"), getPasswordLink());
                }
                forgotPasswordWidget_->setVisible(config::get().is_on(config::features::forgot_password));

                keepLoggedWidget_->setVisible(true);
            }
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_page_uin);
            break;
        case LoginSubpage::SUBPAGE_INTRODUCEYOURSELF:
            introduceMyselfWidget_->UpdateParams(MyInfo()->aimId(), MyInfo()->friendly());
            titleLabel_->setVisible(false);
            entryHint_->setVisible(false);
            buttonsWidget_->setVisible(false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_page_set_name);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_page_set_photo);
            break;
        case LoginSubpage::SUBPAGE_REPORT:
            titleLabel_->setText( QT_TRANSLATE_NOOP("login_page", "Write us"));
            entryHint_->setVisible(false);
            buttonsWidget_->setVisible(false);
            break;
        case LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX:
            hintsWidget_->setVisible(false);
            titleLabel_->setText(QString());
            titleLabel_->setVisible(false);
            entryHint_->setVisible(false);
            buttonsWidget_->setVisible(false);
            keepLoggedWidget_->setVisible(false);
            break;
        }

        const auto idx = (int)_index;
        if (loginStakedWidget_->widget(idx))
        {
            loginStakedWidget_->widget(idx)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            loginStakedWidget_->widget(idx)->updateGeometry();
        }
        titleLabel_->setMaxWidthAndResize(controlsWidth());
        hintsWidget_->updateGeometry();
        if (_index == LoginSubpage::SUBPAGE_PHONE_CONF_INDEX || _index == LoginSubpage::SUBPAGE_REPORT)
        {
            controlsWidget_->updateGeometry();
            resizeSpacers();
        }
        loginStakedWidget_->setEnabled(true);
        loginStakedWidget_->setCurrentIndex(idx);
        updateButtonsWidgetHeight();
        updatePage();
    }

    bool LoginPage::needUserAgreementPage() const
    {
        return (config::get().is_on(config::features::external_user_agreement) &&
                config::get().is_on(config::features::user_agreement_enabled) &&
                !userAgreementAcceptedThisSession());
    }

    void LoginPage::openUserAgreement(const QString& _confidentialLink, const QString& _pdLink)
    {
        if (!userAgreementWidget_)
        {
            QWidget* userAgreementWidget = new QWidget(this);
            if (userAgreementWidget)
            {
                auto userAgreementLayout = Utils::emptyHLayout(userAgreementWidget);
                userAgreementLayout->setAlignment(Qt::AlignCenter);

                userAgreementWidget_ = new UserAgreementWidget(_confidentialLink, _pdLink);
                if (userAgreementWidget_)
                {
                    userAgreementWidget_->init();
                    Testing::setAccessibleName(userAgreementWidget_, qsl("AS UserAgreement userAgreementWidgetLocal"));
                    userAgreementLayout->addWidget(userAgreementWidget_);

                    connect(userAgreementWidget_, &UserAgreementWidget::userAgreementAccepted, this,
                            &LoginPage::loginAfterUserAgreementAccept);
                }
                connect(&Utils::InterConnector::instance(), &Utils::InterConnector::logout, this,
                        &LoginPage::onLogout);

                Testing::setAccessibleName(userAgreementWidget, qsl("AS UserAgreement userAgreementWidget"));
                mainStakedWidget_->addWidget(userAgreementWidget);
                mainStakedWidget_->layout()->setAlignment(userAgreementWidget, Qt::AlignHCenter);
            }
        }
        userAgreementSwitchPages(true);
    }

    void LoginPage::initGDPR()
    {
        connectGDPR();

        const auto idx = needGDPRPage() ? Pages::GDPR : Pages::LOGIN;
        mainStakedWidget_->widget(Pages::LOGIN)->setEnabled(!needGDPRPage());
        mainStakedWidget_->widget(idx)->updateGeometry();
        mainStakedWidget_->setCurrentIndex(idx);
    }

    bool LoginPage::needGDPRPage() const
    {
        return config::get().is_on(config::features::need_gdpr_window);
        /*return !GetAppConfig().GDPR_UserHasAgreed()
                && !GetAppConfig().GDPR_UserHasLoggedInEver();*/
    }

    void LoginPage::connectGDPR()
    {
        if (termsWidget_ != nullptr)
        {
            Testing::setAccessibleName(termsWidget_, qsl("AS GDPR termsWidget"));
            connect(termsWidget_, &TermsPrivacyWidget::agreementAccepted,
                this, [this]()
            {
                if (!Features::isOAuth2LoginAllowed() && lastPage_ == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX)
                    lastPage_ = LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX;
                setGDPRacceptedThisSession(true);
                mainStakedWidget_->widget(Pages::LOGIN)->setEnabled(true);
                mainStakedWidget_->widget(Pages::LOGIN)->updateGeometry();
                initLoginSubPage(lastPage_);
                mainStakedWidget_->setCurrentIndex(Pages::LOGIN);
            });
        }

        connect(mainStakedWidget_, &QStackedWidget::currentChanged, this, [](int _page)
        {
            if (_page == Pages::GDPR)
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::gdpr_show_start);
        });

        connect(this, &LoginPage::loggedIn,
            this, [this]()
        {
            // report to core
            if (!gdprAcceptedThisSession())
                return;

            auto appConfig = GetAppConfig();
            appConfig.SetGDPR_AgreementReportedToServer(AppConfig::GDPR_Report_To_Server_State::Sent_to_Core);
            ModifyAppConfig(appConfig, nullptr, nullptr, false);

            gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            GetDispatcher()->post_message_to_core("agreement/gdpr", collection.get());
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::logout,
            this, [this]()
        {
            if (!Features::isOAuth2LoginAllowed() && lastPage_ == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX)
                lastPage_ = LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX;
            setGDPRacceptedThisSession(false);
            const auto idx = needGDPRPage() ? Pages::GDPR : Pages::LOGIN;
            mainStakedWidget_->widget(Pages::LOGIN)->setEnabled(!needGDPRPage());
            mainStakedWidget_->widget(idx)->updateGeometry();
            initLoginSubPage(lastPage_);
            mainStakedWidget_->setCurrentIndex(idx);
            loggedIn_ = false;
        });
    }

    void LoginPage::userAgreementSwitchPages(bool _needEnable)
    {
        bool needUserAgreement = needUserAgreementPage();

        const auto idx = needUserAgreement ? Pages::USER_AGREEMENT : Pages::LOGIN;
        const auto otherIdx = needUserAgreement ? Pages::LOGIN : Pages::USER_AGREEMENT;

        mainStakedWidget_->widget(otherIdx)->setEnabled(!_needEnable);
        mainStakedWidget_->widget(idx)->setEnabled(_needEnable);
        mainStakedWidget_->widget(idx)->updateGeometry();
        mainStakedWidget_->setCurrentIndex(idx);
    }

    void LoginPage::enableLoginPage(bool _needInitSubPage)
    {
        mainStakedWidget_->setCurrentIndex(Pages::LOGIN);
        mainStakedWidget_->widget(Pages::LOGIN)->setEnabled(true);

        if (_needInitSubPage)
        {
            if (!Features::isOAuth2LoginAllowed() && lastPage_ == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX)
                lastPage_ = LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX;
            initLoginSubPage(lastPage_);
        }
    }

    void LoginPage::logoutAfterUserAgreementAccept()
    {
        setUserAgreementAcceptedThisSession(false);
        enableLoginPage();
        if (userAgreementWidget_)
            userAgreementWidget_->resetChecks();
        loggedIn_ = false;
    }

    void LoginPage::loginAfterUserAgreementAccept()
    {
        setUserAgreementAcceptedThisSession(true);
        doLogin();
    }

    void LoginPage::doLogin()
    {
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("login", uinInput_->text().trimmed());
        collection.set_value_as_qstring("password", passwordInput_->text());
        collection.set_value_as_bool("save_auth_data", keepLogged_->isChecked());
        collection.set_value_as_bool("user_agreement_accepted", userAgreementAcceptedThisSession());

        uinInput_->setEnabled(true);

        if (!otpState_)
            passwordInput_->setEnabled(true);

        if (otpState_ && otpState_ == OTPAuthState::Email)
            collection.set_value_as_enum("token_type", core::token_type::otp_via_email);

        collection.set_value_as_bool("not_log", true);
        sendSeq_ = GetDispatcher()->post_message_to_core("login_by_password", collection.get());

        if (config::get().is_on(config::features::external_url_config))
            nextButton_->setEnabled(false);
    }

    void LoginPage::nextPage()
    {
        static const auto waitServerResponseForPhoneNumberCheckInMsec = 7500;
        static const auto checkServerResponseEachNthMsec = 50;
        static const auto maxTriesOfCheckingForServerResponse = (waitServerResponseForPhoneNumberCheckInMsec / checkServerResponseEachNthMsec);
        static int triesPhoneAuth = 0;

        updateKeedLoggedInSetting(keepLogged_->isChecked());

        if (currentPage() == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX)
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
                    errorLabel_->setVisible(true);
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
        if (currentPage() == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX && receivedPhoneInfo_.isValid())
        {
            bool isMobile = false;
            for (const auto& status : receivedPhoneInfo_.prefix_state_)
            {
                if (status.toLower() == u"mobile")
                {
                    isMobile = true;
                    break;
                }
            }
            const auto isOk = QString::fromStdString(receivedPhoneInfo_.status_).toLower() == u"ok";
            const auto message = !receivedPhoneInfo_.printable_.empty() ? receivedPhoneInfo_.printable_.front() : QString();
            if ((!isMobile || !isOk) && !message.isEmpty())
            {
                errorLabel_->setVisible(true);
                setErrorText(message);
                phone_->setFocus();
                return;
            }
            else
            {
                errorLabel_->setVisible(false);
            }
        }
        setFocus();
        clearErrors(true);
        if (currentPage() == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX)
        {
            smsCodeSendCount_ = 0;
            phone_->setEnabled(true);
            resendCode();
        }
        else if (currentPage() == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)
        {
            doLogin();
        }
    }

    void LoginPage::prevPage()
    {
        clearErrors();
        initLoginSubPage(LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX);
    }

    void LoginPage::switchLoginType()
    {
        setFocus();
        clearErrors();

        const auto curPage = currentPage();

        auto nextPage = (Features::isOAuth2LoginAllowed() ? LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX : LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX);
        if (otpState_ && curPage == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)
        {
            setUserAgreementAcceptedThisSession(false);
            if(userAgreementWidget_)
                userAgreementWidget_->resetChecks();

            updateOTPState(OTPAuthState::Email);
            return;
        }

        switch(curPage)
        {
        case LoginSubpage::SUBPAGE_PHONE_CONF_INDEX:
        case LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX:
            nextPage = LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX;
            break;
        case LoginSubpage::SUBPAGE_INTRODUCEYOURSELF:
            get_gui_settings()->set_value(settings_feedback_email, QString());
            GetDispatcher()->post_message_to_core("logout", nullptr);
            Q_EMIT Utils::InterConnector::instance().logout();

            nextPage = lastPage_;
            break;
        case LoginSubpage::SUBPAGE_REPORT:
            nextPage = lastPage_;
            break;
        case LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX:
            nextPage = LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX;
            break;
        default:
            break;
        }

        if (curPage == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX && nextPage == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_page_switch);

        initLoginSubPage(nextPage);
    }

    void LoginPage::stats_edit_phone()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_edit_phone);
    }

    void LoginPage::phoneTextChanged()
    {
        clearErrors();
        if (!phoneChangedAuto_)
        {
            updateNextButton();
        }
    }

    void LoginPage::postSmsSendStats()
    {
        ++smsCodeSendCount_;
        const auto eventName = smsCodeSendCount_ == 1 ? core::stats::stats_event_names::reg_sms_first_send : core::stats::stats_event_names::reg_sms_resend;
        GetDispatcher()->post_stats_to_core(eventName, { {"how", isCallCheck(checks_) ? "call_number" : "sms"} });
        GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::reg_sms_resend);
    }

    void LoginPage::updateTimer()
    {
        resendButton_->setEnabled(false);
        const auto callCheck = isCallCheck(checks_);
        QString text = callCheck ? QT_TRANSLATE_NOOP("login_page", "Recall") : QT_TRANSLATE_NOOP("login_page", "Resend code");
        if (remainingSeconds_ > 0)
        {
            const auto formattedTime = getFormattedTime(remainingSeconds_);
            text = callCheck ? QT_TRANSLATE_NOOP("login_page", "Recall in %1").arg(formattedTime) : QT_TRANSLATE_NOOP("login_page", "Resend code in %1").arg(formattedTime);
        }
        else
        {
            resendButton_->setEnabled(true);
        }
        resendButton_->setText(text);

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

    void LoginPage::resendCode()
    {
        if (Features::IvrLoginEnabled() && smsCodeSendCount_ >= Features::IvrResendCountToShow())
        {
            const auto btnCenter = resendButton_->mapToGlobal(resendButton_->rect().center());
            auto p = resendButton_->mapToGlobal(resendButton_->rect().bottomLeft());
            auto y = p.y() + Utils::scale_value(8);
            auto x = btnCenter.x() - resendButton_->sizeHint().width() / 2 - Utils::scale_value(8);
            p.setX(x);
            p.setY(y);
            resendMenu_->popup(p);
        }
        else
        {
            sendCode();
        }
    }

    void LoginPage::sendCode()
    {
        if (loggedIn_)
        {
            im_assert(false);
            return;
        }

        ivrUrl_.clear();

        if (smsCodeSendCount_ != 0)
        {
            const auto hintType = isCallCheck(checks_) ? HintType::LoginByPhoneCall : HintType::EnterSMSCode;
            titleLabel_->setText(getTitleText(hintType));
            titleLabel_->setMaxWidthAndResize(controlsWidth());
            entryHint_->setText(hintLabelText(hintType));
            entryHint_->appendPhone(PhoneFormatter::formatted(phone_->getPhoneCode() % phone_->getPhone()));
            entryHint_->appendLink(QT_TRANSLATE_NOOP("login_page", "Change"), ql1s("#"));
            updatePage();
        }

        codeInput_->clear();
        timer_->stop();
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("country", phone_->getPhoneCode());
        collection.set_value_as_qstring("phone", phone_->getPhone());
        collection.set_value_as_qstring("locale", Utils::GetTranslator()->getLang());
        collection.set_value_as_bool("is_login", true);

        sendSeq_ = GetDispatcher()->post_message_to_core("login_get_sms_code", collection.get());
        remainingSeconds_ = Features::getSMSResultTime();
        updateTimer();
    }

    void LoginPage::callPhone()
    {
        if (ivrUrl_.isEmpty())
            return;

        titleLabel_->setText(getTitleText(HintType::EnterPhoneCode));
        titleLabel_->setMaxWidthAndResize(controlsWidth());
        entryHint_->setText(hintLabelText(HintType::EnterPhoneCode));
        entryHint_->appendPhone(PhoneFormatter::formatted(phone_->getPhoneCode() % phone_->getPhone()));
        entryHint_->appendLink(QT_TRANSLATE_NOOP("login_page", "Change"), ql1s("#"));
        updatePage();

        Ui::GetDispatcher()->getCodeByPhoneCall(ivrUrl_);

        remainingSeconds_ = Features::getSMSResultTime();
        codeInput_->clear();
        updateTimer();
    }

    void LoginPage::getSmsResult(int64_t _seq, int _result, int _codeLength, const QString& _ivrUrl, const QString& _checks)
    {
        if (_seq != sendSeq_)
            return;

        errorLabel_->setVisible(_result);
        setErrorText(_result);
        if (_result == core::le_success)
        {
            if (_codeLength != 0)
                codeLength_ = _codeLength;
            codeInput_->setCodeLength(codeLength_);
            checks_ = _checks;
            ivrUrl_ = _ivrUrl;

            postSmsSendStats();

            clearErrors();
            if (currentPage() != LoginSubpage::SUBPAGE_PHONE_CONF_INDEX)
                initLoginSubPage(LoginSubpage::SUBPAGE_PHONE_CONF_INDEX);
        }
        codeInput_->setFocus();
    }

    void LoginPage::updateErrors(int _result)
    {
        loginButton_->setSpinning(false);

        auto pageIndex = currentPage();
        if (currentPage() == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX)
        {
            buttonsWidget_->setVisible(_result > 0);
            nextButton_->setVisible(false);
        }
        else
        {
            buttonsWidget_->setVisible(pageIndex == LoginSubpage::SUBPAGE_PHONE_CONF_INDEX || pageIndex == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX || pageIndex == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX);
            nextButton_->setVisible(pageIndex != LoginSubpage::SUBPAGE_PHONE_CONF_INDEX && pageIndex != LoginSubpage::SUBPAGE_INTRODUCEYOURSELF);
        }

        codeInput_->setEnabled(true);
        uinInput_->setEnabled(true);
        passwordInput_->setEnabled(true);
        errorLabel_->setVisible(_result);
        setErrorText(_result);

        if (currentPage() == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)
        {
            passwordInput_->clear();
            setLoginPageFocus();
        }
        else
        {
            codeInput_->setFocus();
        }
    }

    void Ui::LoginPage::updateKeedLoggedInSetting(bool _keep_logged_in)
    {
        if (get_gui_settings()->get_value(settings_keep_logged_in, settings_keep_logged_in_default()) != _keep_logged_in)
            get_gui_settings()->set_value(settings_keep_logged_in, _keep_logged_in);
    }

    void LoginPage::prepareLoginByPhone()
    {
        if (config::get().is_on(config::features::login_by_phone_allowed))
        {
            if (auto lastPhone = get_gui_settings()->get_value(login_page_last_entered_phone, QString()); !lastPhone.isEmpty())
            {
                QTimer::singleShot(500, this, [lastPhone]() // workaround: phoneinfo/set_can_change_hosts_scheme ( https://jira.mail.ru/browse/IMDESKTOP-3773 )
                {
                    phoneInfoRequestsCount_++;
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("phone", lastPhone);
                    collection.set_value_as_qstring("gui_locale", get_gui_settings()->get_value(settings_language, QString()));
                    phoneInfoLastRequestSpecialId_ = GetDispatcher()->post_message_to_core("phoneinfo", collection.get());
                });
            }
            else
            {
                phone_->setPhone(QString());
            }
        }
    }

    void LoginPage::updateBackButton()
    {
        QString buttonText;
        if (!otpState_ && (currentPage() == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX || currentPage() == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX || currentPage() == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX))
        {
            switch (currentPage())
            {
            case LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX:
                if (Features::isOAuth2LoginAllowed())
                    buttonText = QT_TRANSLATE_NOOP("login_page", "Sign in with Mail.ru");
                else
                    buttonText = QT_TRANSLATE_NOOP("login_page", "Sign in by password");
                break;
            case LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX:
                buttonText = QT_TRANSLATE_NOOP("login_page", "Sign in via phone");
                break;
            case LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX:
                buttonText = QT_TRANSLATE_NOOP("login_page", "Sign in via phone");
                break;
            default:
                break;
            }
            changePageButton_->clearIcon();
            changePageButton_->setText(buttonText);
            changePageButton_->setNormalTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
            changePageButton_->setHoveredTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_HOVER });
            changePageButton_->setPressedTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_ACTIVE });
        }
        else
        {
            changePageButton_->setText(QString());
            changePageButton_->setDefaultImage(qsl(":/controls/back_icon_thin"), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY }, QSize(backIconSize, backIconSize));
            Styling::Buttons::setButtonDefaultColors(changePageButton_);
        }

        changePageButton_->update();
        if (config::get().is_on(config::features::login_by_phone_allowed) || (otpState_ && otpState_ == OTPAuthState::Password) || currentPage() == LoginSubpage::SUBPAGE_REPORT)
            changePageButton_->show();
    }

    void LoginPage::updateNextButton()
    {
        bool enabled = false;
        const auto isOTP = otpState_ && otpState_ == OTPAuthState::Email;
        if (currentPage() == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)
            enabled = !uinInput_->text().isEmpty() && (isOTP || !passwordInput_->text().isEmpty());
        else if (currentPage() == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX)
            enabled = !phone_->getPhone().isEmpty();

        nextButton_->changeRole(enabled ? DialogButtonRole::CONFIRM : DialogButtonRole::DISABLED);
        nextButton_->updateWidth();
    }

    void LoginPage::updateInputFocus()
    {
        if (currentPage() == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)
            setLoginPageFocus();
        else if (currentPage() == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX)
            phone_->setFocus();
        else if (currentPage() == LoginSubpage::SUBPAGE_REPORT)
            contactUsWidget_->setFocus();
        else
            codeInput_->setFocus();

    }

    void LoginPage::updatePage(UpdateMode _mode)
    {
        const auto updateFunc = [this]()
        {
            updateInputFocus();
            updateBackButton();
            updateNextButton();

            errorLabel_->setText(QString());
            errorLabel_->setMinimumHeight(errorLabelMinHeight(1));
            errorLabel_->adjustSize();
            errorLabel_->updateGeometry();
            errorWidget_->setFixedHeight(errorLabel_->height());
        };

        if (_mode == UpdateMode::Delayed)
            QTimer::singleShot(0, this, updateFunc);
        else
            updateFunc();
    }

    void LoginPage::setLoginPageFocus() const
    {
        if (uinInput_->text().isEmpty())
            uinInput_->setFocus();
        else
            passwordInput_->setFocus();
    }

    LoginPage::LoginSubpage LoginPage::currentPage() const
    {
        return (LoginSubpage)loginStakedWidget_->currentIndex();
    }

    void LoginPage::updateOTPState(OTPAuthState _state)
    {
        otpState_ = _state;
        const bool isPassPage = _state == OTPAuthState::Password;
        uinInput_->setVisible(!isPassPage);
        passwordInput_->setVisible(isPassPage);
        passwordInput_->clear();
        keepLoggedWidget_->setVisible(!isPassPage);

        if (!Features::isOAuth2LoginAllowed())
        {
            titleLabel_->setText(getTitleText(isPassPage ? HintType::EnterOTPPassword : HintType::EnterOTPEmail));
            entryHint_->setText(hintLabelText(isPassPage ? HintType::EnterOTPPassword : HintType::EnterOTPEmail));
        }
        else
        {
            titleLabel_->setText(getTitleText(HintType::EnterEmailOAuth2));
            entryHint_->setText(hintLabelText(HintType::EnterEmailOAuth2));
        }

        if (!isPassPage && config::get().is_on(config::features::explained_forgot_password))
            entryHint_->appendLink(QT_TRANSLATE_NOOP("login_page", "Mail.ru for business"), getPasswordLink());
        else if (isPassPage)
            entryHint_->appendLink(uinInput_->text(), loginMailLink(), true);

        if (!isPassPage)
            uinInput_->setFocus();
        else
            passwordInput_->setFocus();

        updatePage(UpdateMode::Instant);
        changePageButton_->setVisible(isPassPage);
        loginStakedWidget_->widget((int)LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        loginStakedWidget_->widget((int)LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)->updateGeometry();
        loginStakedWidget_->updateGeometry();
        updateButtonsWidgetHeight();
    }

    std::string LoginPage::statType(const LoginSubpage _page) const
    {
        return _page == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX ? "email" : "phone";
    }

    bool LoginPage::isCallCheck(const QString& _checks)
    {
        return _checks == u"callui";
    }

    void LoginPage::loginResult(int64_t _seq, int _result, bool _fillProfile)
    {
        loginButton_->setSpinning(false);

        if (sendSeq_ > 0 && _seq != sendSeq_)
            return;

        updateErrors(_result);
        lastEnteredCode_.clear();

        if (_result == 0)
        {
            const auto needFillProfile = config::get().is_on(config::features::need_introduce_window) && _fillProfile;
            get_gui_settings()->set_value<bool>(login_page_need_fill_profile, needFillProfile);
            if (otpState_ && otpState_ == OTPAuthState::Email)
            {
                enableLoginPage();
                if (Features::isOAuth2LoginAllowed() && !needFillProfile)
                    login();
                else
                    updateOTPState(OTPAuthState::Password);
                return;
            }

            if (needFillProfile)
            {
                initLoginSubPage(LoginSubpage::SUBPAGE_INTRODUCEYOURSELF);
            }
            else if (currentPage() == LoginSubpage::SUBPAGE_PHONE_CONF_INDEX)
            {
                codeInput_->clear();
                initLoginSubPage(LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX);
            }

            if (!needFillProfile)
                login();
        }
        else if (otpState_)
        {
            switch (*otpState_)
            {
            case OTPAuthState::Password:
                updateOTPState(OTPAuthState::Password);
                setErrorText(QT_TRANSLATE_NOOP("login_page", "Wrong password"));
                break;
            case OTPAuthState::Email:
                updateOTPState(OTPAuthState::Email);
                setErrorText(QT_TRANSLATE_NOOP("login_page", "Wrong Email"));
                break;
            default:
                updateOTPState(OTPAuthState::Email);
                setErrorText(QT_TRANSLATE_NOOP("login_page", "Wrong Email or password"));
                const auto idx = needGDPRPage() ? Pages::GDPR : Pages::LOGIN;
                mainStakedWidget_->widget(Pages::LOGIN)->setEnabled(!needGDPRPage());
                mainStakedWidget_->widget(idx)->updateGeometry();
                mainStakedWidget_->setCurrentIndex(idx);
                loggedIn_ = false;
                break;
            }
        }
    }

    void LoginPage::oauthRequired()
    {
        if (Features::isOAuth2LoginAllowed())
            openOAuth2Dialog();
    }

    void LoginPage::authError(const int _result)
    {
        if (_result != 0)
            loggedIn_ = false;
        loginButton_->setSpinning(false);
        updateErrors(_result);
    }

    void LoginPage::onUrlConfigError(const int _error)
    {
        nextButton_->setEnabled(true);

        const auto err = (core::ext_url_config_error)_error;
        switch (err)
        {
        case core::ext_url_config_error::ok:
            break;

        case core::ext_url_config_error::config_host_invalid:
        case core::ext_url_config_error::invalid_http_code:
            showUrlConfigHostDialog();
            break;

        case core::ext_url_config_error::answer_not_enough_fields:
            Utils::GetConfirmationWithOneButton(
                QT_TRANSLATE_NOOP("login_page", "OK"),
                QT_TRANSLATE_NOOP("login_page", "It is necessary to supplement the server configuration for the application to work. Contact system administrator"),
                QT_TRANSLATE_NOOP("login_page", "Configuration error"),
                nullptr);
            break;
        case core::ext_url_config_error::answer_parse_error:
        case core::ext_url_config_error::empty_response:
            Utils::GetConfirmationWithOneButton(
                QT_TRANSLATE_NOOP("login_page", "OK"),
                QT_TRANSLATE_NOOP("login_page", "Invalid server configuration format. Contact system administrator"),
                QT_TRANSLATE_NOOP("login_page", "Configuration error"),
                nullptr);
            break;

        default:
            im_assert(false);
            Utils::GetConfirmationWithOneButton(
                QT_TRANSLATE_NOOP("login_page", "OK"),
                QT_TRANSLATE_NOOP("login_page", "Unknown error. Contact system administrator"),
                QT_TRANSLATE_NOOP("login_page", "Configuration error"),
                nullptr);
            break;
        }
    }

    void LoginPage::showUrlConfigHostDialog()
    {
        auto host = new QWidget(nullptr);

        auto edit = new LineEditEx(host);
        edit->setCustomPlaceholder(QT_TRANSLATE_NOOP("login_page", "Enter hostname"));
        edit->changeTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
        edit->setCustomPlaceholderColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });

        if (const auto suggested = config::get().string(config::values::external_config_preset_url); !suggested.empty())
            edit->setText(QString::fromUtf8(suggested.data(), suggested.size()));

        Utils::ApplyStyle(edit, Styling::getParameters().getLineEditCommonQss() % ql1s("QLineEdit{background-color:%1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_BRIGHT, 0.5)));

        auto layout = Utils::emptyVLayout(host);
        layout->setContentsMargins(Utils::scale_value(8), Utils::scale_value(16), Utils::scale_value(8), 0);
        layout->addWidget(edit);

        GeneralDialog::Options opt;
        opt.ignoreKeyPressEvents_ = true;
        GeneralDialog dialog(host, Utils::InterConnector::instance().getMainWindow(), opt);
        dialog.addLabel(QT_TRANSLATE_NOOP("login_page", "Configuration error"));
        dialog.addText(QT_TRANSLATE_NOOP("login_page", "Invalid server configuration hostname. Contact system administrator"), Utils::scale_value(12));
        dialog.addButtonsPair(QT_TRANSLATE_NOOP("login_page", "Cancel"), QT_TRANSLATE_NOOP("login_page", "OK"));

        connect(edit, &LineEditEx::enter, &dialog, &GeneralDialog::accept);

        if (dialog.execute())
        {
            if (const auto host = edit->text().trimmed(); !host.isEmpty())
            {
                auto loginText = uinInput_->text().trimmed();
                if (const auto idx = loginText.indexOf(u'#'); idx != -1)
                    loginText = std::move(loginText).left(idx);

                uinInput_->setText(loginText % u'#' % host);
                nextButton_->click();
            }
        }
    }

    void LoginPage::loginResultAttachPhone(int64_t _seq, int _result)
    {
        if (_seq != sendSeq_)
            return;
        updateErrors(_result);
        if (_result == 0)
        {
            Q_EMIT attached();
        }
    }

    void LoginPage::clearErrors(bool ignorePhoneInfo/* = false*/)
    {
        errorLabel_->hide();
        errorWidget_->setFixedHeight(errorLabelMinHeight(1));

        if (currentPage() == LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX && !ignorePhoneInfo)
        {
            if (phone_->getPhone().size() >= 3)
            {
                phoneInfoRequestsCount_++;

                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("phone", phone_->getPhoneCode() % phone_->getPhone());
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
        updateNextButton();
    }

    void LoginPage::phoneInfoResult(qint64 _seq, const Data::PhoneInfo& _data)
    {
        phoneInfoRequestsCount_--;

        if (phoneInfoLastRequestId_ == _seq || phoneInfoLastRequestSpecialId_ == _seq)
        {
            receivedPhoneInfo_ = _data;

            if (receivedPhoneInfo_.isValid() && !receivedPhoneInfo_.modified_phone_number_.empty())
            {
                auto code = phone_->getPhoneCode();
                if (receivedPhoneInfo_.info_iso_country_.empty() && !receivedPhoneInfo_.modified_prefix_.empty())
                {
                    auto modified_prefix = QString::fromStdString(receivedPhoneInfo_.modified_prefix_);
                    if (code != modified_prefix)
                        code = modified_prefix;
                }

                phoneChangedAuto_ = true;
                phone_->setPhoneCode(code);
                const auto newPhone = QString::fromStdString(receivedPhoneInfo_.modified_phone_number_).remove(0, code.length());
                if (newPhone != phone_->getPhone())
                    phone_->setPhone(newPhone);
                phoneChangedAuto_ = false;
            }
        }

        if (phoneInfoLastRequestSpecialId_ == _seq)
        {
            phoneInfoLastRequestSpecialId_ = 0;
        }
        updateNextButton();
    }

    void LoginPage::setErrorText(int _result)
    {
        setFocus();
        codeInput_->stopSpinner();
        const auto wrongEmailError = config::get().is_on(config::features::login_by_uin_allowed)
            ? QT_TRANSLATE_NOOP("login_page", "Wrong login or password")
            : QT_TRANSLATE_NOOP("login_page", "Wrong Email or password");
        const auto commonError = QT_TRANSLATE_NOOP("login_page", "Error occurred, try again later");
        errorLabel_->setMinimumHeight(errorLabelMinHeight(1));

        const auto page = currentPage();
        core::stats::event_props_type props;
        props.push_back({ "type", statType(page) });

        switch (_result)
        {
        case core::le_wrong_login:
            errorLabel_->setText(page == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX
                ? (otpState_ ? commonError : wrongEmailError)
                : QT_TRANSLATE_NOOP("login_page", "Incorrect code"));

            props.push_back({ "how", page == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX
                                     ? "password"
                                     : isCallCheck(checks_) ? "call_number" : "sms" });

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_error_login, props);
            if (page == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)
            {
                GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::reg_error_uin);
            }
            else
            {
                GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::reg_error_code);
                GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::reg_error_code_sms);
            }
            break;
        case core::le_wrong_login_2x_factor:
            errorLabel_->setText(page == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX ?
                QT_TRANSLATE_NOOP("login_page", "Two-factor authentication is on, please create an app password <a href=\"https://e.mail.ru/settings/2-step-auth\">here</a> to login")
                : QT_TRANSLATE_NOOP("login_page", "You have entered an invalid code. Please try again."));
            errorLabel_->setMinimumHeight(errorLabelMinHeight(page == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX ? 4 : 2));

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_error_login, props);
            if (page == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)
            {
                GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::reg_error_uin);
            }
            else
            {
                GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::reg_error_code);
                GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::reg_error_code_2fa);
            }
            break;
        case core::le_error_validate_phone:
            errorLabel_->setText(QT_TRANSLATE_NOOP("login_page", "Invalid phone number"));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_error_phone);
            break;
        case core::le_success:
            errorLabel_->setText(QString());
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_correct_phone);
            break;
        case core::le_attach_error_busy_phone:
            errorLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "This phone number is already attached to another account.\nPlease edit phone number and try again."));
            errorLabel_->setMinimumHeight(errorLabelMinHeight(2));
            timer_->stop();
            resendButton_->setText(QT_TRANSLATE_NOOP("login_page", "Resend code"));
            resendButton_->setEnabled(false);
            break;
        default:
            errorLabel_->setText(commonError);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_error_other);
            break;
        }
        errorLabel_->adjustSize();
        errorLabel_->updateGeometry();
        errorWidget_->setFixedHeight(errorLabel_->height());
    }

    void LoginPage::setErrorText(const QString& _customError)
    {
        errorLabel_->setText(_customError);
        errorLabel_->setMinimumHeight(errorLabelMinHeight(1));
        errorLabel_->adjustSize();
        errorLabel_->updateGeometry();
        errorWidget_->setFixedHeight(errorLabel_->height());
    }

    void LoginPage::codeEditChanged(const QString& _code)
    {
        if (lastEnteredCode_ != _code)
            clearErrors();
        if (_code.size() == codeLength_ && lastEnteredCode_ != _code)
        {
            setFocus();
            codeInput_->startSpinner();
            codeInput_->setEnabled(false);
            get_gui_settings()->set_value(settings_keep_logged_in, true);
            gui_coll_helper collection(GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("phone", phone_->getPhoneCode() % phone_->getPhone());
            collection.set_value_as_qstring("sms_code", _code);
            collection.set_value_as_bool("save_auth_data", true);
            collection.set_value_as_bool("is_login", true);
            sendSeq_ = GetDispatcher()->post_message_to_core("login_by_phone", collection.get());
            lastEnteredCode_ = _code;
        }
    }

    void LoginPage::openProxySettings()
    {
        auto connection_settings_widget_ = new ConnectionSettingsWidget(this);
        connection_settings_widget_->show();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::proxy_open);
    }

    void LoginPage::openOAuth2Dialog()
    {
#ifdef HAS_OAUTH2
        clearErrors();

        loginButton_->setSpinning(true);

        auto redirectUrl = getUrlConfig().getRedirectUri();
        if (Features::getOAuthType() == ql1s("oauth2_mail"))
            redirectUrl.replace(ql1s("https"), ql1s("http"));

        auto dialog = new OAuth2Dialog(this);
        dialog->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose);
        dialog->setWindowModality(Qt::WindowModality::NonModal);

        dialog->setWindowTitle(qsl(" "));
        dialog->setAuthUrl(getUrlConfig().getAuthUrl());
        dialog->setClientId(Features::getClientId());
        dialog->setRedirectUrl(redirectUrl);
        dialog->setScope(Features::getOAuthScope());

        connect(dialog, &OAuth2Dialog::authCodeReceived, this, &LoginPage::onAuthCodeReceived);
        connect(dialog, &OAuth2Dialog::errorOccured, this, &LoginPage::updateErrors);
        connect(dialog, &OAuth2Dialog::finished, this, &LoginPage::onAuthDialogResult);
        connect(dialog, &OAuth2Dialog::quitApp, Utils::InterConnector::instance().getMainWindow(), &MainWindow::exit, Qt::QueuedConnection);

        if (auto mw = Utils::InterConnector::instance().getMainWindow())
        {
            connect(mw, &MainWindow::windowHiddenToTray, dialog, &QDialog::hide);
            connect(mw, &MainWindow::windowUnhiddenFromTray, dialog, &QDialog::show);
        }

        dialog->start();
        dialog->show();

        nextButton_->setEnabled(false);
#endif// HAS_OAUTH2
    }

    void LoginPage::onAuthCodeReceived(const QString& _authcode)
    {
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("authcode", _authcode);
        sendSeq_ = GetDispatcher()->post_message_to_core("login_by_oauth2", collection.get());
    }

    void Ui::LoginPage::onAuthDialogResult(int _result)
    {
        loginButton_->setSpinning(_result == QDialog::Accepted);
        nextButton_->setEnabled(true);
    }

    void Ui::LoginPage::onLogout()
    {
        // reset everything
        errorLabel_->hide();
        phone_->setFocus();

        initGDPR();

        initLoginSubPage(LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX);

        const auto lastLoginPage = (int)LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX;
        lastPage_ = (LoginSubpage)lastLoginPage;
        if (!Features::isOAuth2LoginAllowed() && lastPage_ == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX)
            lastPage_ = LoginSubpage::SUBPAGE_PHONE_LOGIN_INDEX;
        if (Features::isOAuth2LoginAllowed())
            lastPage_ = LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX;
        else if (config::get().is_on(config::features::login_by_mail_default))
            lastPage_ = LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX;

        if (Features::loginMethod() == Features::LoginMethod::OTPViaEmail)
            otpState_ = OTPAuthState::Email;

        initLoginSubPage(lastPage_);

        prepareLoginByPhone();
        logoutAfterUserAgreementAccept();

        mainStakedWidget_->widget(Pages::LOGIN)->setEnabled(true);
        setEnabled(true);
        nextButton_->setEnabled(true);
    }

    void LoginPage::resizeSpacers()
    {
        controlsWidget_->updateGeometry();
        const auto spacerH = (height() - controlsWidget_->height()) / 2;
        if (currentPage() == LoginSubpage::SUBPAGE_OAUTH2_LOGIN_INDEX)
        {
            topSpacer_->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
            bottomSpacer_->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
        else
        {
            topSpacer_->changeSize(0, spacerH - topButtonWidgetHeight(), QSizePolicy::Expanding, QSizePolicy::Fixed);
            bottomSpacer_->changeSize(0, spacerH, QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    }

    bool LoginPage::eventFilter(QObject *_obj, QEvent *_event)
    {
        if (_obj == loginStakedWidget_ && _event->type() == QEvent::Resize)
        {
            controlsWidget_->adjustSize();
            controlsWidget_->updateGeometry();
            resizeSpacers();
            return true;
        }

        return QWidget::eventFilter(_obj, _event);
    }

    void LoginPage::updateButtonsWidgetHeight()
    {
        const auto curPage = currentPage();
        auto buttonsWidgetHeight = errorWidget_->height();

        if (curPage != LoginSubpage::SUBPAGE_PHONE_CONF_INDEX)
            buttonsWidgetHeight += nextButton_->height();
        else
            buttonsWidgetHeight += resendButton_->height();

        if (curPage == LoginSubpage::SUBPAGE_UIN_LOGIN_INDEX)
        {
            if (keepLoggedWidget_->isVisible())
            buttonsWidgetHeight += keepLoggedWidget_->height();
            if (forgotPasswordWidget_->isVisible())
                buttonsWidgetHeight += forgotPasswordWidget_->height();
        }

        buttonsWidget_->setMinimumHeight(buttonsWidgetHeight);
        buttonsWidget_->updateGeometry();
    }

    EntryHintWidget::EntryHintWidget(QWidget * _parent, const QString& _initialText)
        : LinkAccessibleWidget(_parent)
        , explained_(config::get().is_on(config::features::explained_forgot_password))
    {
        const auto text = explained_
            ? QT_TRANSLATE_NOOP("login_page", "To login use you corporative account created at")
            : _initialText;

        textUnit_ = TextRendering::MakeTextUnit(text, {});
        TextRendering::TextUnit::InitializeParameters params{ getHintFont(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
        params.linkColor_ = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY };
        params.align_ = TextRendering::HorAligment::CENTER;
        textUnit_->init(params);
        textUnit_->setLineSpacing(Utils::scale_value(7));

        if (explained_)
            appendLink(getPasswordLink(), getPasswordLink());

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        setMouseTracking(true);
    }

    void EntryHintWidget::setText(const QString & _text)
    {
        textUnit_->setText(_text);
        updateSize();
    }

    void Ui::EntryHintWidget::appendPhone(const QString & _text)
    {
        const auto delim = (textUnit_->getLinesCount() > 1) ? u' ' : u'\n';
        textUnit_->setText(textUnit_->getText() % delim % _text);
        updateSize();
    }

    void EntryHintWidget::appendLink(const QString& _text, const QString& _link, bool _forceShowLink)
    {
        const QString text = u' ' % _text;
        const auto showLinks = _link.isEmpty()
            ? TextRendering::LinksVisible::DONT_SHOW_LINKS
            : TextRendering::LinksVisible::SHOW_LINKS;

        TextRendering::TextUnitPtr link = TextRendering::MakeTextUnit(text, {}, showLinks);

        TextRendering::TextUnit::InitializeParameters params{ getHintFont(), _forceShowLink ? Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY } : Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
        params.linkColor_ = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY };
        params.align_ = TextRendering::HorAligment::CENTER;

        link->init(params);
        textUnit_->setLineSpacing(getLineSpacing());

        textUnit_->append(std::move(link));
        if (!_link.isEmpty())
            textUnit_->applyLinks({ {_text, _link} });

        updateSize();
    }

    void EntryHintWidget::updateSize()
    {
        const auto desiredWidth = std::min(parentWidget()->width(), controlsWidth());
        setFixedHeight(textUnit_->getHeight(desiredWidth, TextRendering::CallType::FORCE));

        update();
    }

    void EntryHintWidget::paintEvent(QPaintEvent * _event)
    {
        QPainter p(this);
        textUnit_->setOffsets({ (width() - textUnit_->cachedSize().width()) / 2, 0 });
        textUnit_->draw(p);

        QWidget::paintEvent(_event);
    }

    void EntryHintWidget::mouseReleaseEvent(QMouseEvent * _event)
    {
        if (const auto link = textUnit_->getLink(_event->pos()).url_; !link.isEmpty())
        {
            if (u"#" == link)
            {
                Q_EMIT changeClicked();
            }
            else
            {
                if (link == getPasswordLink())
                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::login_forgot_password);

                QDesktopServices::openUrl(QUrl(link));
            }
        }

        QWidget::mouseReleaseEvent(_event);
    }

    void EntryHintWidget::mouseMoveEvent(QMouseEvent * _event)
    {
        const bool overLink = textUnit_->isOverLink(_event->pos());
        const auto cursorPointer = overLink ? Qt::PointingHandCursor : Qt::ArrowCursor;
        setCursor(cursorPointer);
        QWidget::mouseMoveEvent(_event);
    }
}
