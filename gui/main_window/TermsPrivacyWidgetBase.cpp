#include "stdafx.h"
#include "TermsPrivacyWidgetBase.h"
#include "controls/PictureWidget.h"
#include "controls/TransparentScrollBar.h"
#include "controls/DialogButton.h"
#include "utils/utils.h"
#include "utils/Text.h"
#include "utils/gui_coll_helper.h"
#include "fonts.h"
#include "styles/ThemeParameters.h"
#include "core_dispatcher.h"
#include "../gui_settings.h"
#include "../../common.shared/config/config.h"
#if defined(__APPLE__)
#include "utils/macos/mac_support.h"
#endif

#include "../styles/ThemesContainer.h"
#include "styles/StyleSheetContainer.h"
#include "styles/StyleSheetGenerator.h"

namespace
{
    auto iconStyleSheet()
    {
        return std::make_unique<Styling::ArrayStyleSheetGenerator>(
            ql1s("background-color: %1;"),
            std::vector<Styling::ThemeColorKey> { Styling::ThemeColorKey { Styling::StyleVariable::BASE_BRIGHT_INVERSE } });
    }
}

UI_NS_BEGIN

const auto BUTTON_HEIGHT = 40;
const auto BUTTON_SHIFT_LEFT = 32;
const auto BUTTON_SHIFT_RIGHT = 8;
const auto BUTTON_SHIFT_TOP = 32;
const auto BUTTON_SHIFT_BOTTOM = 12;
const auto BUTTON_FONT_SIZE = 18;
const auto LINK_FONT_SIZE = 15;

TermsPrivacyWidgetBase::TermsPrivacyWidgetBase(const QString& _buttonText,
                                               bool _isActive,
                                               QWidget* _parent)
                                            : QWidget(_parent)
                                            , buttonText_(_buttonText)
                                            , isActive_(_isActive)
                                            , layout_(new QVBoxLayout(this))
                                            , testWidgetName_(qsl("BASE"))
{
#if defined(__APPLE__)
    macMenuBlocker_ = std::make_unique<MacMenuBlocker>();
#endif
    setFocus();
}

TermsPrivacyWidgetBase::~TermsPrivacyWidgetBase() = default;

void TermsPrivacyWidgetBase::init()
{
    addMainWidget();
    initContent();
    initIcon(getImagePath(), getImageSize());
    initButton(buttonText_, isActive_);
    addAllToLayout();
    showAll();
}

void TermsPrivacyWidgetBase::addAllToLayout()
{
    addIconToLayout();
    addContentToLayout();
    addAgreeButtonToLayout();

    if (verticalLayout_)
        verticalLayout_->addWidget(bottomWidget_);
}

void TermsPrivacyWidgetBase::showAll()
{
    if (agreeButton_)
        agreeButton_->show();
    if (icon_)
        icon_->show();
}

void TermsPrivacyWidgetBase::addMainWidget()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainWidget_ = new QWidget();
    if (mainWidget_)
    {
        Testing::setAccessibleName(mainWidget_, qsl("AS %1").arg(testWidgetName_));
        mainWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        verticalLayout_ = Utils::emptyVLayout(mainWidget_);
        if (verticalLayout_)
            verticalLayout_->setAlignment(Qt::AlignCenter);

        addScrollArea(mainWidget_);

        layout_->setContentsMargins(QMargins());
        addBottomWidget(mainWidget_);
    }
}

void TermsPrivacyWidgetBase::initIcon(const QString& _iconPath, const QSize& _iconSize)
{
    if (!mainWidget_)
        return;
    iconWidget_ = addIcon(_iconPath, _iconSize, mainWidget_);
    if (iconWidget_)
        Testing::setAccessibleName(iconWidget_, qsl("AS %1 iconWidget").arg(testWidgetName_));
}

void TermsPrivacyWidgetBase::initButton(const QString& _buttonText, bool _isActive)
{
    if (!mainWidget_)
        return;
    addAgreeButton(_buttonText, mainWidget_);
    if (agreeButton_)
    {
        agreeButton_->setEnabled(_isActive);
        connect(agreeButton_, &QPushButton::clicked, this, &TermsPrivacyWidgetBase::onAgreeClicked);
    }
}

void TermsPrivacyWidgetBase::addAgreeButtonToLayout()
{
    if (!agreeButton_ || !bottomLayout_)
        return;

    bottomLayout_->addWidget(agreeButton_);
    bottomLayout_->setAlignment(agreeButton_, Qt::AlignHCenter | Qt::AlignTop);
    addVSpacer();
}

void TermsPrivacyWidgetBase::addVSpacer(int _height)
{
    bottomLayout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(_height), QSizePolicy::Fixed, _height == 0 ? QSizePolicy::Expanding : QSizePolicy::Fixed));
}

void TermsPrivacyWidgetBase::addBottomWidget(QWidget* _parent)
{
    if (!bottomWidget_)
        bottomWidget_ = new QWidget(_parent);
    if (bottomWidget_)
    {
        Testing::setAccessibleName(bottomWidget_, qsl("AS %1 bottomWidget").arg(testWidgetName_));
        bottomWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        if (!bottomLayout_)
            bottomLayout_ = Utils::emptyVLayout(bottomWidget_);
    }
}

void TermsPrivacyWidgetBase::addIconToLayout()
{
    if(!iconWidget_ || !verticalLayout_)
        return;
    verticalLayout_->addWidget(iconWidget_);
}

void TermsPrivacyWidgetBase::addScrollArea(QWidget* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(this);
    if (scrollArea)
    {
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet(qsl("background: transparent; border: none;"));
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::grabTouchWidget(scrollArea->viewport(), true);
        Testing::setAccessibleName(scrollArea, qsl("AS %1 scrollArea").arg(testWidgetName_));
        if (layout_)
            layout_->addWidget(scrollArea);
        scrollArea->setWidget(_parent);
    }
}

QWidget* TermsPrivacyWidgetBase::addIcon(const QString& _iconPath, const QSize& _iconSize, QWidget* _parent)
{
    QWidget* iconWidget = new QWidget(_parent);
    if (iconWidget)
    {
        iconWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        Styling::setStyleSheet(iconWidget, iconStyleSheet(), Styling::StyleSheetPolicy::UseSetStyleSheet);

        if (!iconLayout_)
            iconLayout_ = Utils::emptyVLayout(iconWidget);
        if (!icon_)
            icon_ = new PictureWidget(iconWidget, _iconPath);
        if (icon_ && iconLayout_)
        {
            icon_->setFixedSize(Utils::scale_value(_iconSize));
            iconLayout_->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
            iconLayout_->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
            Testing::setAccessibleName(icon_, qsl("AS %1 icon").arg(testWidgetName_));
            iconLayout_->addWidget(icon_);
            iconLayout_->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
        }
    }
    return iconWidget;
}

void TermsPrivacyWidgetBase::addAgreeButton(const QString& _text, QWidget* _parent)
{
    if (!agreeButton_)
        agreeButton_ = new DialogButton(_parent, _text, DialogButtonRole::CONFIRM);
    if (agreeButton_)
    {
        agreeButton_->setFixedHeight(Utils::scale_value(BUTTON_HEIGHT));
        agreeButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        agreeButton_->setContentsMargins(Utils::scale_value(BUTTON_SHIFT_LEFT), Utils::scale_value(BUTTON_SHIFT_RIGHT),
                                         Utils::scale_value(BUTTON_SHIFT_TOP), Utils::scale_value(BUTTON_SHIFT_BOTTOM));
        agreeButton_->setCursor(QCursor(Qt::PointingHandCursor));
        agreeButton_->setFont(Fonts::appFontScaled(BUTTON_FONT_SIZE, Fonts::FontWeight::Medium));
        agreeButton_->setEnabled(true);
        agreeButton_->updateWidth();
        Testing::setAccessibleName(agreeButton_, qsl("AS %1 agreeButton").arg(testWidgetName_));
    }
}

TextBrowserEx::Options TermsPrivacyWidgetBase::getLinkOptions() const
{
    TextBrowserEx::Options linkOptions;
    linkOptions.font_ = Fonts::appFontScaled(LINK_FONT_SIZE);
    linkOptions.linksUnderline_ = TextBrowserEx::Options::LinksUnderline::ThemeDependent;
    linkOptions.useDocumentMargin_ = true;
    linkOptions.openExternalLinks_ = false;
    return linkOptions;
}

void TermsPrivacyWidgetBase::addLink(TextBrowserEx*& _linkLabel, const QString& _link, int _minWidth, int _maxWidth,
                                     QWidget* _parent, const QString& _testName)
{
    TextBrowserEx::Options descriptionOptions = getLinkOptions();
    addLinkWithOptions(descriptionOptions, _linkLabel, _link, _minWidth, _maxWidth, _parent, _testName);
}

void TermsPrivacyWidgetBase::addLinkWithOptions(const TextBrowserEx::Options& _options, TextBrowserEx*& _linkLabel, const QString& _link,
                                                int _minWidth, int _maxWidth, QWidget* _parent, const QString& _testName, bool _isClickable)
{
    if (!_linkLabel)
        _linkLabel = new TextBrowserEx(_options, _parent);
    if (_linkLabel)
    {
        _linkLabel->setHtmlSource(_link);
        _linkLabel->setMinimumWidth(Utils::scale_value(_minWidth));
        _linkLabel->setMaximumWidth(Utils::scale_value(_maxWidth));
        _linkLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        _linkLabel->setPlaceholderText(QString());
        _linkLabel->setAutoFillBackground(false);
        _linkLabel->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _linkLabel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _linkLabel->setAlignment(Qt::AlignCenter);
        _linkLabel->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
        _linkLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        TextBrowserExUtils::setAppropriateHeight(*_linkLabel);
        if (_isClickable)
            connect(_linkLabel, &QTextBrowser::anchorClicked, this, &TermsPrivacyWidgetBase::onAnchorClicked);
        Testing::setAccessibleName(_linkLabel, qsl("AS %1 %2").arg(testWidgetName_, _testName));
    }
}

void TermsPrivacyWidgetBase::onAgreeClicked()
{
#if defined(__APPLE__)
    macMenuBlocker_->unblock();
#endif
}

void TermsPrivacyWidgetBase::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Escape || _event->key() == Qt::Key_Down || _event->key() == Qt::Key_Up)
    {
        setFocus();
        return;
    }

    return QWidget::keyPressEvent(_event);
}

void TermsPrivacyWidgetBase::showEvent(QShowEvent* _event)
{
#if defined(__APPLE__)
    macMenuBlocker_->block();
#endif
    QWidget::showEvent(_event);
    setFocus();
}

void TermsPrivacyWidgetBase::onAnchorClicked(const QUrl& _link) const
{
    if (!_link.isValid())
        return;

    if (const auto linkStr = _link.toString(); !linkStr.isEmpty())
    {
        reportLink(linkStr);

        Utils::openUrl(linkStr);
    }
}

UI_NS_END


