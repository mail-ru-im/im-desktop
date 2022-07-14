#include "stdafx.h"

#include "CustomStatusWidget.h"
#include "StatusCommonUi.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "controls/DialogButton.h"
#include "controls/EmojiButton.h"
#include "controls/EmojiPickerDialog.h"
#include "controls/TextWidget.h"
#include "styles/ThemeParameters.h"
#include "controls/TextEditEx.h"
#include "core_dispatcher.h"
#include "fonts.h"

namespace
{

constexpr auto defaultStatusDuration() noexcept { return std::chrono::seconds(24 * 3600); }

int maxWidth() noexcept
{
    return Utils::scale_value(360);
}

QSize defaultEmojiWidgetSize() noexcept
{
    return Utils::scale_value(QSize(52, 52));
}

QFont headerLabelFont()
{
    if constexpr (platform::is_apple())
        return Fonts::appFontScaled(22, Fonts::FontWeight::Medium);
    else
        return Fonts::appFontScaled(22);
}

constexpr int maxDescriptionLength() noexcept
{
    return 120;
}

constexpr int maxLimitOverflow() noexcept
{
    return 99;
}

int remainedTopMargin() noexcept
{
    return Utils::scale_value(8);
}

int textMaxHeight() noexcept
{
    return Utils::scale_value(104);
}

QFont descriptionFont()
{
    return Fonts::appFontScaled(17);
}

Styling::ThemeColorKey remainedColor()
{
    return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
}

Styling::ThemeColorKey errorColor()
{
    return Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION };
}

int rightMargin() noexcept
{
    return Utils::scale_value(20);
}

int labelBottomMargin() noexcept
{
    if constexpr (platform::is_apple())
        return Utils::scale_value(12);

    return Utils::scale_value(16);
}

int labelTopMargin() noexcept
{
    if constexpr (platform::is_apple())
        return Utils::scale_value(18);

    return Utils::scale_value(14);
}

}

namespace Statuses
{

using namespace Ui;

//////////////////////////////////////////////////////////////////////////
// CreateCustomStatusWidget_p
//////////////////////////////////////////////////////////////////////////

class CustomStatusWidget_p
{
public:
    DialogButton* createButton(QWidget* _parent, const QString& _text, const DialogButtonRole _role, bool _enabled = true)
    {
        auto button = new DialogButton(_parent, _text, _role);
        button->setFixedWidth(Utils::scale_value(116));
        button->setEnabled(_enabled);
        return button;
    }

    int spaceForRemained(int _widgetWidth)
    {
        return _widgetWidth - rightMargin() - (errorVisible_ ? description_->geometry().left() + errorUnit_->cachedSize().width() : 0);
    }

    int spaceForError(int _widgetWidth)
    {
        return _widgetWidth - rightMargin() - remainedCount_->cachedSize().width() - description_->geometry().left();
    }

    void updateDescriptionHeight()
    {
        const auto height = description_->getTextHeight();
        const auto showScrollBar = height > textMaxHeight();
        static const auto minHeight = QFontMetrics(descriptionFont()).height() + Utils::scale_value(4);

        description_->setVerticalScrollBarPolicy(showScrollBar ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
        description_->setFixedHeight(std::clamp(height, minHeight, textMaxHeight()));
    }

    Status status_;
    TextEditEx* description_ = nullptr;
    EmojiButton* emojiWidget_ = nullptr;
    TextRendering::TextUnitPtr remainedCount_;
    TextRendering::TextUnitPtr errorUnit_;
    DialogButton* nextButton_ = nullptr;
    bool errorVisible_ = false;
    bool canAcceptStatus_ = false;
};

//////////////////////////////////////////////////////////////////////////
// CustomStatusWidget
//////////////////////////////////////////////////////////////////////////

CustomStatusWidget::CustomStatusWidget(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<CustomStatusWidget_p>())
{
    setFixedSize(maxWidth(), Utils::scale_value(244));

    auto layout = Utils::emptyVLayout(this);

    auto label = new TextWidget(this, QT_TRANSLATE_NOOP("status_popup", "Choose unique status"));
    TextRendering::TextUnit::InitializeParameters params{ headerLabelFont(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
    params.align_ = TextRendering::HorAligment::CENTER;
    label->init(params);
    label->setMaxWidthAndResize(maxWidth());

    layout->addSpacing(labelTopMargin());
    layout->addWidget(label);
    layout->addSpacing(labelBottomMargin());

    auto contentLayout = Utils::emptyHLayout();

    d->emojiWidget_ = new EmojiButton(this);
    Testing::setAccessibleName(d->emojiWidget_, qsl("AS CustomStatusWidget emojiWidget"));
    d->emojiWidget_->setFixedSize(defaultEmojiWidgetSize());

    auto emojiLayout = Utils::emptyVLayout();
    auto descriptionLayout = Utils::emptyVLayout();

    emojiLayout->addSpacing(Utils::scale_value(8));
    emojiLayout->addWidget(d->emojiWidget_);
    emojiLayout->addStretch();

    contentLayout->addSpacing(Utils::scale_value(6));
    contentLayout->addLayout(emojiLayout);
    contentLayout->addSpacing(Utils::scale_value(6));
    contentLayout->addLayout(descriptionLayout);
    contentLayout->addSpacing(rightMargin());

    d->description_ = new TextEditEx(this, descriptionFont(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID }, true, false);
    d->description_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->description_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->description_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
    d->description_->setMinimumWidth(Utils::scale_value(276));
    d->description_->setPlaceholderText(QT_TRANSLATE_NOOP("status_popup", "Enter status text"));
    d->description_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::FollowSettingsRules);
    d->description_->setTabChangesFocus(true);
    d->description_->setDocumentMargin(0);
    d->description_->setHeightSupplement(Utils::scale_value(4));
    d->description_->setCursor(Qt::IBeamCursor);
    d->description_->viewport()->setCursor(Qt::IBeamCursor);
    Testing::setAccessibleName(d->description_, qsl("AS CustomStatusWidget descriptionEdit"));

    connect(d->description_, &TextEditEx::textChanged, this, &CustomStatusWidget::onTextChanged);
    connect(d->description_, &TextEditEx::enter, this, &CustomStatusWidget::tryToConfirmStatus);
    Utils::ApplyStyle(d->description_, Styling::getParameters().getTextEditCommonQss(true));
    descriptionLayout->addWidget(d->description_);
    layout->addLayout(contentLayout);

    auto backButton = createButton(this, QT_TRANSLATE_NOOP("status_popup", "Back"), DialogButtonRole::CANCEL);
    backButton->setFocusPolicy(Qt::TabFocus);
    Testing::setAccessibleName(backButton, qsl("AS CustomStatusWidget backButton"));
    d->nextButton_ = createButton(this, QT_TRANSLATE_NOOP("status_popup", "Done"), DialogButtonRole::CONFIRM, false);
    d->nextButton_->setFocusPolicy(Qt::TabFocus);
    Testing::setAccessibleName(d->nextButton_, qsl("AS CustomStatusWidget doneButton"));

    auto buttonsLayout = Utils::emptyHLayout();
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(backButton);
    buttonsLayout->addSpacing(Utils::scale_value(20));
    buttonsLayout->addWidget(d->nextButton_);
    buttonsLayout->addStretch();

    layout->addStretch();
    layout->addLayout(buttonsLayout);
    layout->addSpacing(Utils::scale_value(16));

    connect(backButton, &DialogButton::clicked, this, [this](){ Q_EMIT backClicked(QPrivateSignal()); });
    connect(d->nextButton_, &DialogButton::clicked, this, &CustomStatusWidget::onNextClicked);

    d->remainedCount_ = TextRendering::MakeTextUnit(QString());
    d->remainedCount_->init({ Fonts::adjustedAppFont(15), remainedColor() });

    d->errorUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("status_popup", "Maximum length - %1 symbols").arg(maxDescriptionLength()));
    d->errorUnit_->init({ Fonts::adjustedAppFont(15), errorColor() });

    connect(d->emojiWidget_, &EmojiButton::clicked, this, [this]()
    {
        auto picker = new EmojiPickerDialog(nullptr, d->emojiWidget_, this);
        connect(picker, &EmojiPickerDialog::emojiSelected, this, [this](const Emoji::EmojiCode& _code)
        {
            d->emojiWidget_->setCurrent(_code);
            if (auto picker = qobject_cast<EmojiPickerDialog*>(sender()))
                picker->close();
        });
        picker->popup();
        d->description_->setFocus();
    });

    onTextChanged();
}

CustomStatusWidget::~CustomStatusWidget() = default;

void CustomStatusWidget::setStatus(const Status& _status)
{
    d->status_ = _status;
    d->description_->setText(_status.isEmpty() ? QString() : _status.getDescription());
    d->description_->moveCursor(QTextCursor::End);

    if (_status.isEmpty())
        d->emojiWidget_->reset();
    else
        d->emojiWidget_->setCurrent(Emoji::EmojiCode::fromQString(_status.toString()), EmojiButton::SkipAnimation::Yes);
}

void CustomStatusWidget::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);

    const auto& descriptionGeometry = d->description_->geometry();
    d->remainedCount_->setOffsets(width() - d->remainedCount_->desiredWidth() - Utils::scale_value(20), descriptionGeometry.bottom() + remainedTopMargin());
    d->remainedCount_->draw(p);

    if (d->errorVisible_)
    {
        d->errorUnit_->setOffsets(descriptionGeometry.left(), descriptionGeometry.bottom() + remainedTopMargin());
        d->errorUnit_->draw(p);
    }
}

void CustomStatusWidget::showEvent(QShowEvent* _event)
{
    d->description_->setFocus();
    QWidget::showEvent(_event);
}

void CustomStatusWidget::onTextChanged()
{
    auto text = d->description_->getPlainText();

    auto count = 0;
    auto finder = QTextBoundaryFinder(QTextBoundaryFinder::Grapheme, text);
    while (count - maxDescriptionLength() < maxLimitOverflow() && finder.toNextBoundary() != -1)
        count++;

    if (finder.position() > 0)
    {
        QSignalBlocker sb(d->description_);
        text.truncate(finder.position());
        d->description_->setPlainText(text);
        d->description_->moveCursor(QTextCursor::End);
    }

    d->errorVisible_ = count > maxDescriptionLength();
    d->updateDescriptionHeight();

    d->remainedCount_->setText(QString::number(maxDescriptionLength() - count), d->errorVisible_ ? errorColor() : remainedColor());
    d->remainedCount_->elide(d->spaceForRemained(width()));
    d->errorUnit_->elide(d->spaceForError(width()));

    if (!text.isEmpty() && d->emojiWidget_->current().isEmpty())
        d->emojiWidget_->setCurrent(Emoji::EmojiCode(0x1f4ac));

    d->canAcceptStatus_ = !QStringView(text).trimmed().isEmpty() && !d->errorVisible_;
    d->nextButton_->setEnabled(d->canAcceptStatus_);

    update();
}

void CustomStatusWidget::onNextClicked()
{
    confirmStatus();
}

void CustomStatusWidget::tryToConfirmStatus()
{
    if (d->canAcceptStatus_)
        confirmStatus();
}

void CustomStatusWidget::confirmStatus()
{
    auto seconds = std::chrono::seconds::zero();
    const auto endTime = d->status_.getEndTime();
    const auto statusEmpty = d->status_.isEmpty();

    if (!statusEmpty && endTime.isValid() && endTime > QDateTime::currentDateTime())
        seconds = std::chrono::seconds(QDateTime::currentDateTime().secsTo(endTime));
    else if (statusEmpty || endTime.isValid())
        seconds = defaultStatusDuration();

    GetDispatcher()->setStatus(d->emojiWidget_->current(), seconds.count(), d->description_->getPlainText().trimmed());
    Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
    Statuses::showToastWithDuration(seconds);
}

}
