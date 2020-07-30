#include "stdafx.h"

#include "styles/ThemeParameters.h"
#include "controls/TransparentScrollBar.h"
#include "controls/TooltipWidget.h"
#include "controls/LineEditEx.h"
#include "controls/TextEditEx.h"
#include "controls/DialogButton.h"
#include "controls/CustomButton.h"
#include "controls/ClickWidget.h"
#include "utils/InterConnector.h"
#include "utils/gui_coll_helper.h"
#include "core_dispatcher.h"
#include "utils/utils.h"
#include "types/poll.h"
#include "fonts.h"
#include "input_widget/InputWidgetUtils.h"
#include "PollWidget.h"
#include "animation/animation.h"
#include "omicron/omicron_helper.h"
#include "../common.shared/omicron_keys.h"

namespace
{
    int commonPollsMargin()
    {
        return Utils::scale_value(16);
    }

    int bottomPollsMargin()
    {
        return Utils::scale_value(20);
    }

    Ui::DialogButton* createButton(QWidget* _parent, const QString& _text, const Ui::DialogButtonRole _role, bool _enabled = true)
    {
        auto button = new Ui::DialogButton(_parent, _text, _role);
        button->setFixedWidth(Utils::scale_value(116));
        button->setEnabled(_enabled);
        return button;
    }

    int32_t remainedTopMargin()
    {
        return Utils::scale_value(8);
    }

    int32_t maxAnswerLength()
    {
        return 140;
    }

    int maxPollOptions()
    {
        return Omicron::_o(omicron::keys::max_poll_options, feature::default_max_poll_options());
    }

    int minPollOptions()
    {
        return 1;
    }

    bool isValidText(QStringView _text)
    {
        return !_text.trimmed().isEmpty();
    }

    const QColor& pollItemButtonColor()
    {
        static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
        return color;
    }

    const QColor& pollItemButtonHoveredColor()
    {
        static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY_HOVER);
        return color;
    }

    const QColor& pollItemButtonPressedColor()
    {
        static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY_ACTIVE);
        return color;
    }
}


namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// PollWidget_p
//////////////////////////////////////////////////////////////////////////

class PollWidget_p
{
public:
    bool isSendEnabled() const { return list_->itemsWithTextCount() >= minPollOptions() && isValidText(question_->getPlainText()); }

    QString contact_;
    PollItemsList* list_ = nullptr;
    TextEditEx* question_ = nullptr;
    DialogButton* sendButton_ = nullptr;
    DialogButton* cancelButton_ = nullptr;
    CustomButton* addWidget_ = nullptr;
    ScrollAreaWithTrScrollBar* scrollArea_ = nullptr;
    anim::Animation scrollAnimation_;

    struct InputData
    {
        QString text_;
        Data::QuotesVec quotes_;
        Data::MentionMap mentions_;
    } inputData_;
};

//////////////////////////////////////////////////////////////////////////
// PollWidget
//////////////////////////////////////////////////////////////////////////

PollWidget::PollWidget(const QString& _contact, QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<PollWidget_p>())
{
    d->contact_ = _contact;
    setMinimumHeight(Utils::scale_value(533));

    auto layout = Utils::emptyVLayout(this);
    layout->setContentsMargins(commonPollsMargin(), commonPollsMargin(), 0, bottomPollsMargin());

    auto header = new TextWidget(this, QT_TRANSLATE_NOOP("poll", "Create poll"));
    header->init(Fonts::appFontScaled(24, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));

    layout->addWidget(header);
    layout->addSpacing(Utils::scale_value(16));

    auto content = new QWidget(this);
    auto contentLayout = Utils::emptyVLayout(content);
    contentLayout->setContentsMargins(0, 0, commonPollsMargin(), 0);

    d->question_ = new TextEditEx(this, Fonts::appFontScaled(17), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, true);
    d->question_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->question_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->question_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
    d->question_->document()->setDocumentMargin(0);
    d->question_->document()->adjustSize();
    d->question_->addSpace(Utils::scale_value(4));
    d->question_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::FollowSettingsRules);
    d->question_->setTabChangesFocus(true);
    d->question_->setMaxHeight(Utils::scale_value(100));
    connect(d->question_, &TextEditEx::enter, this, &PollWidget::onEnterPressed);
    connect(d->question_, &TextEditEx::textChanged, this, &PollWidget::onInputChanged);
    connect(d->question_, &TextEditEx::focusOut, d->question_, &TextEditEx::clearSelection);
    Utils::ApplyStyle(d->question_, Styling::getParameters().getTextEditCommonQss(true));

    d->question_->setPlaceholderText(QT_TRANSLATE_NOOP("poll", "Ask your question"));

    contentLayout->addWidget(d->question_);
    contentLayout->addSpacing(Utils::scale_value(8));

    d->list_ = new PollItemsList(this);
    connect(d->list_, &PollItemsList::enterPressed, this, &PollWidget::onEnterPressed);
    connect(d->list_, &PollItemsList::itemRemoved, this, &PollWidget::onItemRemoved);
    connect(d->list_, &PollItemsList::textChanged, this, &PollWidget::onInputChanged);

    contentLayout->addWidget(d->list_);
    contentLayout->addSpacing(Utils::scale_value(20));

    d->addWidget_ = new CustomButton(this, qsl(":/controls/add_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
    d->addWidget_->setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER));
    d->addWidget_->setPressedColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE));
    d->addWidget_->setText(QT_TRANSLATE_NOOP("poll", "Add another option"));
    d->addWidget_->setNormalTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
    d->addWidget_->setHoveredTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER));
    d->addWidget_->setPressedTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE));
    d->addWidget_->setFont(Fonts::appFontScaled(17, Fonts::FontWeight::Medium));
    d->addWidget_->setIconAlignment(Qt::AlignLeft);
    d->addWidget_->setTextAlignment(Qt::AlignLeft);
    d->addWidget_->setTextLeftOffset(Utils::scale_value(28));
    connect(d->addWidget_, &CustomButton::clicked, this, &PollWidget::onAdd);

    contentLayout->addWidget(d->addWidget_);
    contentLayout->addStretch(1);

    d->scrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
    d->scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea_->setFocusPolicy(Qt::NoFocus);
    d->scrollArea_->setWidgetResizable(true);
    d->scrollArea_->setStyleSheet(qsl("background-color: transparent; border: none"));
    d->scrollArea_->setWidget(content);

    layout->addWidget(d->scrollArea_);

    auto buttonsFrame = new QFrame(this);
    buttonsFrame->setFixedWidth(Utils::scale_value(260));
    auto buttonsLayout = Utils::emptyHLayout(buttonsFrame);
    d->cancelButton_ = createButton(this, QT_TRANSLATE_NOOP("poll", "Cancel"), DialogButtonRole::CANCEL);
    buttonsLayout->addWidget(d->cancelButton_);
    buttonsLayout->addSpacing(Utils::scale_value(16));
    d->sendButton_ = createButton(this, QT_TRANSLATE_NOOP("poll", "Send"), DialogButtonRole::CONFIRM, true);
    d->sendButton_->setEnabled(false);
    buttonsLayout->addWidget(d->sendButton_);

    connect(d->cancelButton_, &DialogButton::clicked, this, &PollWidget::onCancel);
    connect(d->sendButton_, &DialogButton::clicked, this, &PollWidget::onSend);

    layout->addSpacing(Utils::scale_value(16));
    layout->addWidget(buttonsFrame, 0, Qt::AlignHCenter);

    setFocusPolicy(Qt::ClickFocus);
}

PollWidget::~PollWidget()
{

}

void PollWidget::setFocusOnQuestion()
{
    d->question_->setFocus();
}

void PollWidget::setInputData(const QString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions)
{
    d->inputData_.text_ = _text;
    d->inputData_.quotes_ = _quotes;
    d->inputData_.mentions_ = _mentions;

    d->question_->setMentions(_mentions);
    d->question_->setPlainText(_text, false);
    d->question_->moveCursor(QTextCursor::End);
}

void PollWidget::onAdd()
{
    auto startValue = d->scrollArea_->verticalScrollBar()->value();

    d->list_->addItem();

    d->addWidget_->setVisible(d->list_->count() < maxPollOptions());
    d->sendButton_->setEnabled(d->isSendEnabled());

    d->scrollAnimation_.finish();
    d->scrollAnimation_.start([this, startValue]() {
        auto* scrollBar = d->scrollArea_->verticalScrollBar();
        scrollBar->setValue(startValue + (scrollBar->maximum() - startValue) * d->scrollAnimation_.current());
    }, 0, 1, 150);
}

void PollWidget::onCancel()
{
    Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
}

void PollWidget::onSend()
{
    Data::PollData poll;

    poll.type_ = Data::PollData::AnonymousPoll;

    for (const auto& answerText : d->list_->answers())
    {
        if (!isValidText(answerText))
            continue;

        auto answer = std::make_shared<Data::PollData::Answer>();
        answer->text_ = answerText;
        poll.answers_.push_back(std::move(answer));
    }

    core_dispatcher::MessageData data;
    data.mentions_ = std::move(d->inputData_.mentions_);
    data.quotes_ = std::move(d->inputData_.quotes_);
    data.text_ = Utils::replaceFilesPlaceholders(d->question_->getPlainText(), Data::FilesPlaceholderMap());
    data.poll_ = poll;

    GetDispatcher()->sendMessageToContact(d->contact_, std::move(data));

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::polls_create, { { "from", "plus" }, {"chat_type", Ui::getStatsChatType()} });

    Q_EMIT Utils::InterConnector::instance().acceptGeneralDialog();
    close();
}

void PollWidget::onEnterPressed()
{
    if (d->isSendEnabled())
        onSend();
}

void PollWidget::onItemRemoved()
{
    // show addwidget through a singleshot to avoid layout jumping on macos
    QTimer::singleShot(0, this, [this]() { d->addWidget_->setVisible(d->list_->count() < maxPollOptions()); });
    d->sendButton_->setEnabled(d->isSendEnabled());
}

void PollWidget::onInputChanged()
{
    d->sendButton_->setEnabled(d->isSendEnabled());
}

//////////////////////////////////////////////////////////////////////////
// PollItemsList_p
//////////////////////////////////////////////////////////////////////////

class PollItemsList_p
{
public:
    DraggableList* list_ = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// PollItemsList
//////////////////////////////////////////////////////////////////////////

PollItemsList::PollItemsList(QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<PollItemsList_p>())
{
    auto layout = Utils::emptyVLayout(this);
    d->list_ = new DraggableList(this);
    d->list_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    for (auto i = 0; i < 2; i++)
        addItem();

    layout->addWidget(d->list_);
}

PollItemsList::~PollItemsList()
{

}

void PollItemsList::addItem()
{
    auto item = new PollItem(d->list_);
    item->setFocusOnInput();

    connect(item, &PollItem::remove, this, &PollItemsList::onRemoveItem);
    connect(item, &PollItem::enterPressed, this, &PollItemsList::enterPressed);
    connect(item, &PollItem::textChanged, this, &PollItemsList::textChanged);

    d->list_->addItem(item);
}

PollItem* PollItemsList::itemAt(int _index)
{
    return static_cast<PollItem*>(d->list_->itemAt(_index));
}

int PollItemsList::count() const
{
    return d->list_->count();
}

int PollItemsList::itemsWithTextCount() const
{
    auto count = 0;

    for (const auto& itemData : d->list_->orderedData())
        count += (isValidText(itemData.toString()) ? 1 : 0);

    return count;
}

QStringList PollItemsList::answers() const
{
    QStringList result;

    const auto data = d->list_->orderedData();

    for (const auto& d : data)
        result.push_back(d.toString());

    return result;
}

void PollItemsList::onRemoveItem()
{
    if (auto item = qobject_cast<PollItem*>(sender()))
    {
        const auto removedItemHasFocus = item->hasFocusOnInput();
        auto index = d->list_->indexOf(item);

        d->list_->removeItem(item);
        item->deleteLater();
        updateGeometry();

        if (removedItemHasFocus)
        {
            index = std::min(std::max(0, index), d->list_->count() - 1);
            if (auto focusItem = qobject_cast<PollItem*>(d->list_->itemAt(index)))
                focusItem->setFocusOnInput();
        }

        Q_EMIT itemRemoved();
    }
}

//////////////////////////////////////////////////////////////////////////
// PollItem_p
//////////////////////////////////////////////////////////////////////////

class PollItem_p
{
public:
    int32_t spaceForRemained(int _itemWidth)
    {
        return _itemWidth - answer_->geometry().right();
    }

    TextEditEx* answer_ = nullptr;
    CustomButton* dragButton_ = nullptr;
    CustomButton* deleteButton_ = nullptr;

    TextRendering::TextUnitPtr remainedCount_;
    bool drawRemained_ = false;
};

//////////////////////////////////////////////////////////////////////////
// PollItem
//////////////////////////////////////////////////////////////////////////

PollItem::PollItem(QWidget* _parent)
    : DraggableItem(_parent),
      d(std::make_unique<PollItem_p>())
{
    setMinimumHeight(Utils::scale_value(45));
    auto layout = Utils::emptyHLayout(this);

    d->answer_ = new TextEditEx(this, Fonts::appFontScaled(17), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, true);
    d->answer_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->answer_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->answer_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
    d->answer_->setMinimumWidth(Utils::scale_value(230));
    d->answer_->setPlaceholderText(QT_TRANSLATE_NOOP("poll", "Answer"));
    d->answer_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::FollowSettingsRules);
    d->answer_->setTabChangesFocus(true);
    d->answer_->document()->setDocumentMargin(0);
    d->answer_->document()->adjustSize();
    d->answer_->addSpace(Utils::scale_value(4));
    d->answer_->adjustHeight(d->answer_->minimumWidth());

    setFocusProxy(d->answer_);

    connect(d->answer_, &TextEditEx::textChanged, this, &PollItem::onTextChanged);
    connect(d->answer_, &TextEditEx::enter, this, &PollItem::enterPressed);
    connect(d->answer_, &TextEditEx::textChanged, this, &PollItem::textChanged);
    connect(d->answer_, &TextEditEx::focusOut, d->answer_, &TextEditEx::clearSelection);

    Utils::ApplyStyle(d->answer_, Styling::getParameters().getTextEditCommonQss(true));

    layout->addWidget(d->answer_);
    layout->addStretch();

    d->deleteButton_ = new CustomButton(this, qsl(":/delete_icon"), QSize(20, 20), pollItemButtonColor());
    d->deleteButton_->setHoverColor(pollItemButtonHoveredColor());
    d->deleteButton_->setPressedColor(pollItemButtonPressedColor());
    connect(d->deleteButton_, &CustomButton::clicked, this, &PollItem::remove);
    layout->addWidget(d->deleteButton_);
    layout->addSpacing(Utils::scale_value(20));

    d->dragButton_ = new CustomButton(this, qsl(":/smiles_menu/move_sticker_icon"), QSize(20, 20), pollItemButtonColor());
    d->dragButton_->setHoverColor(pollItemButtonHoveredColor());
    d->dragButton_->setPressedColor(pollItemButtonPressedColor());
    d->dragButton_->setCursor(Qt::SizeAllCursor);
    d->dragButton_->installEventFilter(this);

    layout->addWidget(d->dragButton_);

    d->remainedCount_ = TextRendering::MakeTextUnit(QString());
    d->remainedCount_->init(Fonts::adjustedAppFont(15), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
}

PollItem::~PollItem() = default;

QVariant PollItem::data() const
{
    return d->answer_->getPlainText();
}

void PollItem::setFocusOnInput()
{
    d->answer_->setFocus();
}

bool PollItem::hasFocusOnInput() const
{
    return d->answer_->hasFocus();
}

void PollItem::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);

    p.fillRect(_event->rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

    if (d->drawRemained_)
        d->remainedCount_->draw(p);

    DraggableItem::paintEvent(_event);
}

void PollItem::resizeEvent(QResizeEvent* _event)
{
    const auto buttonGeometry = d->dragButton_->geometry();
    const auto remainedHeight = d->remainedCount_->cachedSize().height();

    d->drawRemained_ = _event->size().height() - buttonGeometry.bottom() - remainedTopMargin() > remainedHeight;
    d->remainedCount_->setOffsets(_event->size().width() - d->remainedCount_->desiredWidth(), buttonGeometry.bottom() + remainedTopMargin());
}

bool PollItem::eventFilter(QObject* _object, QEvent* _event)
{
    if (_object == d->dragButton_)
    {
        if (_event->type() == QEvent::MouseMove && d->dragButton_->isPressed())
        {
            auto mouseMove = static_cast<QMouseEvent*>(_event);
            const auto pos = d->dragButton_->mapToParent(mouseMove->pos());
            onDragMove(pos);
        }
        else if (_event->type() == QEvent::MouseButtonPress)
        {
            auto mousePress = static_cast<QMouseEvent*>(_event);
            const auto pos = d->dragButton_->mapToParent(mousePress->pos());
            onDragStart(pos);
        }
        else if (_event->type() == QEvent::MouseButtonRelease)
        {
            onDragStop();
        }
    }
    return false;
}

void PollItem::onTextChanged()
{
    const auto text = d->answer_->getPlainText();
    if (text.size() > maxAnswerLength())
    {
        d->answer_->setPlainText(text.left(maxAnswerLength()));
        d->answer_->moveCursor(QTextCursor::End);
    }

    d->remainedCount_->setText(QString::number(maxAnswerLength() - d->answer_->getPlainText().size()));
    d->remainedCount_->getHeight(d->spaceForRemained(width()));
    update();
}

}
