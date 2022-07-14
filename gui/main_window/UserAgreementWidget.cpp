#include "stdafx.h"
#include "../fonts.h"
#include "../../common.shared/string_utils.h"
#include "../../common.shared/message_processing/text_formatting.h"
#include "../styles/ThemeParameters.h"
#include "TermsPrivacyWidgetBase.h"
#include "UserAgreementWidget.h"
#include "MainWindow.h"
#include "LoginPage.h"
#include "utils/InterConnector.h"
#include "controls/CustomButton.h"
#include "controls/TextBrowserEx.h"
#include "controls/DialogButton.h"
#include "controls/PictureWidget.h"
#include "core_dispatcher.h"
#include <QSpacerItem>
#include "controls/CheckboxList.h"
#include "styles/ThemeParameters.h"
#include "../styles/ThemesContainer.h"
#include "../gui_settings.h"
#include "history_control/MessageStyle.h"

#if defined(__APPLE__)
#include "utils/macos/mac_support.h"
#endif

UI_NS_BEGIN

const auto DIALOG_MIN_WIDTH = 380;
const auto DIALOG_MAX_WIDTH = 460;
const auto BOTTOM_WIDGET_HEIGHT = 320;
const auto ICON_WIDTH = 380;
const auto ICON_HEIGHT = 280;
const auto SPACING = 10;
const auto LINKS_TOP_SHIFT = 70;
const auto BTN_TOP_SHIFT = 10;
const auto CHECKBOX_HEIGHT = 26;
const auto BACK_ICON_SIZE = 20;
const auto BACK_BTN_SIZE = 24;
const auto TOP_BTN_WIDGET_HEIGHT = 44;
const auto TOP_BTN_LSHIFT = 12;
const auto TOP_BTN_RSHIFT = 12;
const auto TOP_BTN_TOP_SHIFT = 12;
const auto TOP_BTN_BOTTOM_SHIFT = 0;
const auto AGREE_BTN_FONT_SIZE = 15;
const auto LINK_FONT_SIZE = 14;
const auto LINK_MIN_HEIGHT = 52;
const auto LINK_LINE_SPACING = 20;

QString makeLink(const QString& _link, const QString& _text)
{
    const auto agree_with = QT_TRANSLATE_NOOP("user_agreement_widget", "Agree with %1");
    QString link_text = qsl("<a href=\"%1\">%2</a>").arg(_link, _text.isEmpty() ? qsl("empty") : _text);
    return agree_with.arg(link_text);
}

void UserAgreementWidget::initContent()
{
    addBackButton();
    TermsPrivacyWidgetBase::initContent();
    if (bottomWidget_)
        bottomWidget_->setFixedHeight(Utils::scale_value(BOTTOM_WIDGET_HEIGHT));
    const auto confidentialText = QT_TRANSLATE_NOOP("user_agreement_widget", "processing confidential data");
    const auto confidentialTextLink = makeLink(confidentialLink_, confidentialText);
    addLink(confLabel_, confidentialTextLink, DIALOG_MIN_WIDTH, DIALOG_MAX_WIDTH, this, qsl("confidentialLabel"));
    confLabel_->setAlignment(Qt::AlignLeft);
    confLabel_->setMinimumHeight(Utils::scale_value(LINK_MIN_HEIGHT));
    confLabel_->setLineSpacing(LINK_LINE_SPACING);

    const auto pdText = QT_TRANSLATE_NOOP("user_agreement_widget", "using personal data");
    const auto pdTextLink = makeLink(personalDataLink_, pdText);
    addLink(pdLabel_, pdTextLink, DIALOG_MIN_WIDTH, DIALOG_MAX_WIDTH, this, qsl("pdLabel"));
    pdLabel_->setAlignment(Qt::AlignLeft);
    pdLabel_->setMinimumHeight(Utils::scale_value(LINK_MIN_HEIGHT));
    pdLabel_->setLineSpacing(LINK_LINE_SPACING);

    checkBoxWidget_ = new QWidget(this);
    if (checkBoxWidget_)
    {
        checkBoxWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        checkBoxWidget_->setMinimumWidth(Utils::scale_value(DIALOG_MIN_WIDTH));
        checkBoxWidget_->setMaximumWidth(Utils::scale_value(DIALOG_MAX_WIDTH));

        checkBoxLayout_ = Utils::emptyVLayout(checkBoxWidget_);
        if (checkBoxLayout_)
            checkBoxLayout_->setAlignment(Qt::AlignCenter);

        addCheckbox(confidentialCheckbox_, checkBoxWidget_, confLabel_, qsl("confidentialCheckbox"));
        addCheckbox(personalDataCheckbox_, checkBoxWidget_, pdLabel_, qsl("pdCheckbox"));
    }
}

void UserAgreementWidget::addBackButton()
{
    auto topButtonWidget = new QWidget(this);
    if (topButtonWidget)
    {
        Testing::setAccessibleName(topButtonWidget, qsl("AS %1 topButtonWidget").arg(testWidgetName_));
        topButtonWidget->setStyleSheet(qsl("border: none; background-color: transparent;"));
        topButtonWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        topButtonWidget->setFixedHeight(Utils::scale_value(TOP_BTN_WIDGET_HEIGHT));
        auto topButtonLayout = Utils::emptyHLayout(topButtonWidget);
        if (topButtonLayout)
        {
            topButtonLayout->setContentsMargins(Utils::scale_value(TOP_BTN_LSHIFT),
                                                Utils::scale_value(TOP_BTN_TOP_SHIFT),
                                                Utils::scale_value(TOP_BTN_RSHIFT),
                                                Utils::scale_value(TOP_BTN_BOTTOM_SHIFT));
        }


        backButton_ = new CustomButton(topButtonWidget);
        if (backButton_)
        {
            Styling::Buttons::setButtonDefaultColors(backButton_);
            backButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            backButton_->setMinimumSize(Utils::scale_value(QSize(BACK_BTN_SIZE, BACK_BTN_SIZE)));
            Testing::setAccessibleName(backButton_, qsl("AS %1 backButton").arg(testWidgetName_));
            topButtonLayout->addWidget(backButton_);
            topButtonLayout->setAlignment(backButton_, Qt::AlignLeft | Qt::AlignHCenter);

            connect(backButton_, &Ui::CustomButton::clicked, this, &UserAgreementWidget::goBack);

            backButton_->setDefaultImage(qsl(":/controls/back_icon_thin"),
                                         Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY},
                                         QSize(BACK_ICON_SIZE, BACK_ICON_SIZE));
            Styling::Buttons::setButtonDefaultColors(backButton_);
            backButton_->show();
        }
    }
}

void UserAgreementWidget::goBack()
{
    if (auto mainWindow = Utils::InterConnector::instance().getMainWindow())
    {
        mainWindow->loginPage()->userAgreementSwitchPages(false);
        mainWindow->loginPage()->enableLoginPage(true);
    }
    resetChecks();
}

UserAgreementWidget::UserAgreementWidget(const QString& _confidentialLink, const QString& _personalDataLink, QWidget* _parent)
    : TermsPrivacyWidgetBase(QT_TRANSLATE_NOOP("user_agreement_widget", "Accept"), false, _parent)
    , confidentialLink_(_confidentialLink)
    , personalDataLink_(_personalDataLink)
{
    setTestWidgetName(qsl("USER_AGREEMENT"));
}

UserAgreementWidget::~UserAgreementWidget() = default;

void UserAgreementWidget::addContentToLayout()
{
    TermsPrivacyWidgetBase::addContentToLayout();
    if (bottomLayout_)
    {
        addVSpacer(LINKS_TOP_SHIFT);
        bottomLayout_->addWidget(checkBoxWidget_);
        bottomLayout_->setAlignment(checkBoxWidget_, Qt::AlignHCenter | Qt::AlignTop);
        addVSpacer(BTN_TOP_SHIFT);
    }
}

void UserAgreementWidget::addCheckbox(QCheckBox*& _checkbox, QWidget* _parent, TextBrowserEx* _linkLabel, const QString& _testName)
{
    if (!_checkbox)
        _checkbox = new CheckBox(this, Qt::AlignTop);
    if (!_checkbox || ! _linkLabel || !_parent)
        return;
    _checkbox->setFixedHeight(Utils::scale_value(CHECKBOX_HEIGHT));
    _checkbox->setFocusPolicy(Qt::ClickFocus);
    QWidget *checkBoxWidget = new QWidget(_parent);
    if (checkBoxWidget)
    {
        checkBoxWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        checkBoxWidget->setMinimumWidth(Utils::scale_value(DIALOG_MIN_WIDTH));
        checkBoxWidget->setMaximumWidth(Utils::scale_value(DIALOG_MAX_WIDTH));
        QHBoxLayout *checkBoxLayout = Utils::emptyHLayout(checkBoxWidget);
        if (checkBoxLayout)
        {
            checkBoxLayout->setAlignment(Qt::AlignHCenter| Qt::AlignBottom);
            checkBoxLayout->addWidget(_checkbox, Utils::scale_value(SPACING), Qt::AlignTop);
            checkBoxLayout->addWidget(_linkLabel, Utils::scale_value(SPACING), Qt::AlignCenter);
            checkBoxLayout->setAlignment(_checkbox, Qt::AlignHCenter| Qt::AlignTop);
            checkBoxLayout->setSpacing(Utils::scale_value(SPACING));
        }
        if (checkBoxLayout_)
            checkBoxLayout_->addWidget(checkBoxWidget);
    }

    connect(_checkbox, &QCheckBox::stateChanged, this, &UserAgreementWidget::check);
    Testing::setAccessibleName(_checkbox, qsl("AS %1 %2").arg(testWidgetName_, _testName));
}

void UserAgreementWidget::resetChecks()
{
    if (!confidentialCheckbox_ || !personalDataCheckbox_)
        return;
    confidentialCheckbox_->setCheckState(Qt::Unchecked);
    personalDataCheckbox_->setCheckState(Qt::Unchecked);
}

void UserAgreementWidget::check()
{
    if (!confidentialCheckbox_ || !personalDataCheckbox_)
        return;

    const auto allChecked = confidentialCheckbox_->isChecked() && personalDataCheckbox_->isChecked();
    if (agreeButton_->isEnabled() != allChecked)
        agreeButton_->setEnabled(allChecked);
}

void UserAgreementWidget::onAgreeClicked()
{
    Q_EMIT userAgreementAccepted();
}

QSize UserAgreementWidget::getImageSize() const
{
    return QSize(ICON_WIDTH, ICON_HEIGHT);
}

QString UserAgreementWidget::getImagePath() const
{
    if (get_gui_settings()->get_value(settings_language, QString()) == u"ru")
        return qsl(":/gdpr/gdpr_100");

    return qsl(":/gdpr/gdpr_100_en");
}

void UserAgreementWidget::initButton(const QString& _buttonText, bool _isActive)
{
    TermsPrivacyWidgetBase::initButton(_buttonText, _isActive);
    if (!agreeButton_)
        return;
    agreeButton_->setFont(Fonts::appFontScaled(AGREE_BTN_FONT_SIZE, Fonts::FontWeight::Medium));
}

TextBrowserEx::Options UserAgreementWidget::getLinkOptions() const
{
    TextBrowserEx::Options linkOptions = TermsPrivacyWidgetBase::getLinkOptions();
    linkOptions.linksUnderline_ = TextBrowserEx::Options::LinksUnderline::NoUnderline;
    linkOptions.linkColor_ = Ui::MessageStyle::getLinkColorKey().style();
    linkOptions.font_ = Fonts::appFontScaled(LINK_FONT_SIZE, Fonts::FontWeight::Normal);
    return linkOptions;
}

UI_NS_END
