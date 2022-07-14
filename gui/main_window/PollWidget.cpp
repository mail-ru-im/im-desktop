#include "stdafx.h"

#include "styles/ThemeParameters.h"
#include "controls/EmojiPickerDialog.h"
#include "controls/TransparentScrollBar.h"
#include "controls/TextWidget.h"
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
#include "omicron/omicron_helper.h"
#include "../common.shared/omicron_keys.h"
#include "main_window/MainWindow.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/history_control/ThreadPlate.h"
#include "../utils/features.h"

namespace
{
    int commonPollsMargin() noexcept { return Utils::scale_value(16); }

    int bottomPollsMargin() noexcept { return Utils::scale_value(20); }

    int emojiWidgetSpacing() noexcept { return Utils::scale_value(8); }

    int32_t remainedTopMargin() noexcept
    {
        return Utils::scale_value(8);
    }

    constexpr int32_t maxAnswerLength() noexcept
    {
        return 140;
    }

    int maxPollOptions()
    {
        return Omicron::_o(omicron::keys::max_poll_options, feature::default_max_poll_options());
    }

    constexpr int minPollOptions() noexcept
    {
        return 1;
    }

    QSize addEmojiIconSize() noexcept
    {
        return Utils::scale_value(QSize(32, 32));
    }

    bool isValidText(QStringView _text)  noexcept
    {
        return !_text.trimmed().isEmpty();
    }

    Styling::ThemeColorKey pollItemButtonColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    Styling::ThemeColorKey pollItemButtonHoveredColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_HOVER };
    }

    Styling::ThemeColorKey pollItemButtonPressedColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_ACTIVE };
    }
} // namespace

namespace Ui
{

class PollWidgetPrivate
{
public:
    struct InputData
    {
        Data::FString text_;
        Data::QuotesVec quotes_;
        Data::MentionMap mentions_;
    } inputData_;

    QString contact_;
    PollWidget* qptr_ = nullptr;
    PollItemsList* list_ = nullptr;
    TextEditEx* question_ = nullptr;
    CustomButton* emojiButton_ = nullptr;

    DialogButton* sendButton_ = nullptr;
    DialogButton* cancelButton_ = nullptr;

    QScrollArea* scrollArea_ = nullptr;
    QVariantAnimation* scrollAnimation_ = nullptr;
    QVBoxLayout* layout_ = nullptr;
    QVBoxLayout* contentLayout_ = nullptr;
    QPointer<EmojiPickerDialog> emojiPicker_;

    PollWidgetPrivate(PollWidget* q);

    void initUi(const QString& _contact);
    void createHeader();
    void createQuestionEdit();
    void createPollListItems();
    void createScrollArea();
    void createButtonsFrame();
    void createScrollAnimation();
    static Ui::DialogButton* createButton(QWidget* _parent, const QString& _text, const Ui::DialogButtonRole _role, bool _enabled = true);

    bool isSendEnabled() const;
};

PollWidgetPrivate::PollWidgetPrivate(PollWidget* _q) : qptr_(_q) {}

void PollWidgetPrivate::initUi(const QString& _contact)
{
    contact_ = _contact;
    layout_ = Utils::emptyVLayout(qptr_);
    layout_->setContentsMargins(commonPollsMargin(), commonPollsMargin(), 0, bottomPollsMargin());
    createHeader();
    createScrollArea();
    createButtonsFrame();
}

void PollWidgetPrivate::createHeader()
{
    auto header = new TextWidget(qptr_, QT_TRANSLATE_NOOP("poll", "Create poll"));
    header->init({ Fonts::appFontScaled(24, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });

    auto headerLayout = Utils::emptyHLayout();
    headerLayout->addWidget(header, 0, Qt::AlignLeft);
    if (Logic::getContactListModel()->isThread(contact_))
    {
        headerLayout->addSpacing(commonPollsMargin());
        headerLayout->addWidget(ThreadPlate::plateForPopup(qptr_), 0, Qt::AlignRight);
        headerLayout->addSpacing(commonPollsMargin());
    }
    layout_->addLayout(headerLayout);
    layout_->addSpacing(Utils::scale_value(16));
}

void PollWidgetPrivate::createQuestionEdit()
{
    TextEditBox* questionBox = new TextEditBox(qptr_, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID }, true, true, addEmojiIconSize());
    questionBox->initUi();
    questionBox->layout()->setSpacing(emojiWidgetSpacing());
    questionBox->layout()->setContentsMargins({ 0, 0, 0, Utils::scale_value(4) });
    Utils::ApplyStyle(questionBox, Styling::getParameters().getTextEditBoxCommonQss());

    question_ = questionBox->textEdit<TextEditEx>();
    question_->document()->documentLayout()->setPaintDevice(nullptr);//avoid unnecessary scaling performed by its own paintDevice
    question_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    question_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    question_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
    question_->setDocumentMargin(0);
    question_->setHeightSupplement(Utils::scale_value(4));
    question_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::FollowSettingsRules);
    question_->setTabChangesFocus(true);
    question_->setUndoRedoEnabled(true);
    question_->setMaxHeight(Utils::scale_value(100));
    question_->setPlaceholderText(QT_TRANSLATE_NOOP("poll", "Ask your question"));
    question_->setFormatEnabled(Features::isFormattingInInputEnabled());
    question_->viewport()->setCursor(Qt::IBeamCursor);
    Testing::setAccessibleName(emojiButton_, qsl("AS PollWidget questionWidget"));
    question_->setStyleSheet(qsl("background-color: transparent; border-style: none;"));
    QObject::connect(question_, &TextEditEx::enter, qptr_, &PollWidget::onEnterPressed);
    QObject::connect(question_, &TextEditEx::textChanged, qptr_, &PollWidget::onInputChanged);

    emojiButton_ = questionBox->emojiButton<CustomButton>();
    emojiButton_->setDefaultImage(qsl(":/smile"));
    emojiButton_->setFixedSize(addEmojiIconSize());
    emojiButton_->setCheckable(true);
    emojiButton_->setChecked(false);
    emojiButton_->setFocusPolicy(Qt::TabFocus);
    emojiButton_->setFocusColor(focusColorPrimaryKey());
    updateButtonColors(emojiButton_, InputStyleMode::Default);
    Testing::setAccessibleName(emojiButton_, qsl("AS PollWidget emojiWidget"));
    QObject::connect(emojiButton_, &CustomButton::clicked, qptr_, &PollWidget::popupEmojiPicker);

    contentLayout_->addWidget(questionBox);
    contentLayout_->addSpacing(Utils::scale_value(8));
}

void PollWidgetPrivate::createPollListItems()
{
    list_ = new PollItemsList(qptr_);
    QObject::connect(list_, &PollItemsList::enterPressed, qptr_, &PollWidget::onEnterPressed);
    QObject::connect(list_, &PollItemsList::itemAdded, qptr_, &PollWidget::onItemAdded);
    QObject::connect(list_, &PollItemsList::itemRemoved, qptr_, &PollWidget::onItemRemoved);
    QObject::connect(list_, &PollItemsList::textChanged, qptr_, &PollWidget::onInputChanged);

    contentLayout_->addWidget(list_);
}

void PollWidgetPrivate::createScrollArea()
{
    auto content = new QWidget(qptr_);
    contentLayout_ = Utils::emptyVLayout(content);
    contentLayout_->setContentsMargins(0, 0, commonPollsMargin(), 0);
    createQuestionEdit();
    createPollListItems();

    scrollArea_ = new QScrollArea(qptr_);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea_->setFocusPolicy(Qt::NoFocus);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet(qsl("background-color: transparent; border: none"));
    scrollArea_->setWidget(content);

    createScrollAnimation();

    layout_->addWidget(scrollArea_);
    layout_->addSpacing(Utils::scale_value(16));
}

void PollWidgetPrivate::createScrollAnimation()
{
    scrollAnimation_ = new QVariantAnimation(qptr_);
    scrollAnimation_->setDuration(150);
    QObject::connect(scrollAnimation_, &QVariantAnimation::valueChanged, qptr_, [this](const QVariant& value)
    {
       scrollArea_->verticalScrollBar()->setValue(value.toInt());
    });
}

void PollWidgetPrivate::createButtonsFrame()
{
    auto buttonsFrame = new QFrame(qptr_);
    buttonsFrame->setFixedWidth(Utils::scale_value(260));

    cancelButton_ = createButton(qptr_, QT_TRANSLATE_NOOP("poll", "Cancel"), DialogButtonRole::CANCEL);
    sendButton_ = createButton(qptr_, QT_TRANSLATE_NOOP("poll", "Send"), DialogButtonRole::CONFIRM, true);
    sendButton_->setEnabled(false);

    QObject::connect(cancelButton_, &DialogButton::clicked, qptr_, &PollWidget::onCancel);
    QObject::connect(sendButton_, &DialogButton::clicked, qptr_, &PollWidget::onSend);

    auto buttonsLayout = Utils::emptyHLayout(buttonsFrame);
    buttonsLayout->addWidget(cancelButton_);
    buttonsLayout->addSpacing(Utils::scale_value(16));
    buttonsLayout->addWidget(sendButton_);

    layout_->addWidget(buttonsFrame, 0, Qt::AlignHCenter);
}

Ui::DialogButton* PollWidgetPrivate::createButton(QWidget* _parent, const QString& _text, const Ui::DialogButtonRole _role, bool _enabled)
{
    auto button = new Ui::DialogButton(_parent, _text, _role);
    button->setFixedWidth(Utils::scale_value(116));
    button->setEnabled(_enabled);
    return button;
}

bool PollWidgetPrivate::isSendEnabled() const
{
    return list_->itemsWithTextCount() >= minPollOptions() && isValidText(question_->getPlainText());
}

PollWidget::PollWidget(const QString& _contact, QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<PollWidgetPrivate>(this))
{
    auto defaultMinHeight = Utils::scale_value(533);
    if (auto mainWindow = Utils::InterConnector::instance().getMainWindow())
    {
        const auto margin = Utils::scale_value(80);
        defaultMinHeight = std::max(mainWindow->minimumHeight() - margin, margin);
    }

    setMinimumHeight(defaultMinHeight);

    d->initUi(_contact);
}

PollWidget::~PollWidget()
{
}

void PollWidget::setFocusOnQuestion()
{
    d->question_->setFocus();
}

void PollWidget::setInputData(const Data::FString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions)
{
    d->inputData_.text_ = _text;
    d->inputData_.quotes_ = _quotes;
    d->inputData_.mentions_ = _mentions;

    d->question_->setMentions(_mentions);
    d->question_->setFormattedText(_text);
    d->question_->moveCursor(QTextCursor::End);
}

Data::FString PollWidget::getInputText() const
{
    return d->question_->getText();
}

int PollWidget::getCursorPos() const
{
    return d->question_->textCursor().position();
}

void PollWidget::onCancel()
{
    Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
}

void PollWidget::onSend()
{
    Data::PollData poll;

    poll.type_ = Data::PollData::AnonymousPoll;

    const auto answers = d->list_->answers();
    for (const auto& answerText : answers)
    {
        if (!isValidText(answerText))
            continue;

        auto answer = std::make_shared<Data::PollData::Answer>();
        answer->text_ = answerText;
        poll.answers_.push_back(std::move(answer));
    }

    core_dispatcher::MessageData messageData;
    messageData.mentions_ = std::move(d->inputData_.mentions_);
    messageData.quotes_ = std::move(d->inputData_.quotes_);
    messageData.text_ = d->question_->getText();
    Utils::replaceFilesPlaceholders(messageData.text_, {});
    messageData.poll_ = std::move(poll);

    GetDispatcher()->sendMessageToContact(d->contact_, std::move(messageData));

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::polls_create, { { "from", "plus" }, {"chat_type", Ui::getStatsChatType(d->contact_)} });

    Q_EMIT Utils::InterConnector::instance().acceptGeneralDialog();
    close();
}

void PollWidget::onEmojiSelected(const Emoji::EmojiCode& _code)
{
    d->question_->insertEmoji(_code);
    d->question_->setFocus();
}

void PollWidget::onEnterPressed()
{
    if (d->isSendEnabled())
        onSend();
}

void PollWidget::onItemAdded()
{
    d->sendButton_->setEnabled(d->isSendEnabled());

    d->scrollAnimation_->stop();
    d->scrollAnimation_->setStartValue(d->scrollArea_->verticalScrollBar()->value());
    d->scrollAnimation_->setEndValue(d->scrollArea_->verticalScrollBar()->maximum());
    d->scrollAnimation_->start();
}

void PollWidget::onItemRemoved()
{
    d->sendButton_->setEnabled(d->isSendEnabled());
}

void PollWidget::onInputChanged()
{
    d->sendButton_->setEnabled(d->isSendEnabled());
}

void Ui::PollWidget::popupEmojiPicker(bool _on)
{
    if (d->emojiPicker_ && !_on)
    {
        d->emojiPicker_->close();
    }
    else
    {
        if (!d->emojiPicker_)
            d->emojiPicker_ = new EmojiPickerDialog(d->question_, d->emojiButton_, this);
        connect(d->emojiPicker_, &EmojiPickerDialog::emojiSelected, this, &PollWidget::onEmojiSelected);
        connect(d->emojiPicker_, &EmojiPickerDialog::destroyed, this, [this]()
        {
            d->emojiButton_->setChecked(false);
        });
        d->emojiPicker_->popup();
        d->question_->setFocus();
    }
}

void Ui::PollWidget::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Escape && d->emojiPicker_)
        d->emojiPicker_->close();
    else
        QWidget::keyPressEvent(_event);
}

void Ui::PollWidget::closeEvent(QCloseEvent* _event)
{
    if (d->emojiPicker_)
        d->emojiPicker_->close();
    QWidget::closeEvent(_event);
}


class PollItemsListPrivate
{
public:
    QPixmap cachedPixmap_;
    PollItemsList* qptr_ = nullptr;
    DraggableList* list_ = nullptr;
    CustomButton* addWidget_ = nullptr;

    PollItemsListPrivate(PollItemsList* _q) : qptr_(_q) {}

    void createList()
    {
        list_ = new DraggableList(qptr_);
        list_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    }

    void createAddButton()
    {
        addWidget_ = new CustomButton(qptr_, qsl(":/controls/add_icon"), QSize(20, 20), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
        addWidget_->setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_HOVER });
        addWidget_->setPressedColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_ACTIVE });
        addWidget_->setText(QT_TRANSLATE_NOOP("poll", "Add another option"));
        addWidget_->setNormalTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
        addWidget_->setHoveredTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_HOVER });
        addWidget_->setPressedTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_ACTIVE });
        addWidget_->setFont(Fonts::appFontScaled(15, Fonts::FontWeight::Medium));
        addWidget_->setIconAlignment(Qt::AlignLeft);
        addWidget_->setTextAlignment(Qt::AlignLeft);
        addWidget_->setTextLeftOffset(Utils::scale_value(28));
        addWidget_->setVisible(false);
        QObject::connect(addWidget_, &CustomButton::clicked, qptr_, &PollItemsList::addItem);
    }

    void cachePixmap()
    {
        cachedPixmap_ = qptr_->grab();
        addWidget_->hide();
        list_->hide();
        qptr_->update();
    }

    void clearCache()
    {
        cachedPixmap_ = QPixmap();
        addWidget_->setVisible(list_->count() < maxPollOptions());
        list_->show();
        qptr_->update();
    }
};

PollItemsList::PollItemsList(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<PollItemsListPrivate>(this))
{
    auto layout = Utils::emptyVLayout(this);

    d->createList();
    layout->addWidget(d->list_);
    layout->addSpacing(Utils::scale_value(20));

    d->createAddButton();
    layout->addWidget(d->addWidget_);
    layout->addStretch(1);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    for (auto i = 0; i < 2; i++)
        addItem();

    d->addWidget_->setVisible(true);
}

PollItemsList::~PollItemsList() = default;

void PollItemsList::addItem()
{
    auto item = new PollItem(d->list_);

    item->hide();
    item->adjustSize();

    connect(item, &PollItem::remove, this, &PollItemsList::onRemoveItem);
    connect(item, &PollItem::enterPressed, this, &PollItemsList::enterPressed);
    connect(item, &PollItem::textChanged, this, &PollItemsList::textChanged);

    d->list_->addItem(item);
    item->show();
    item->setFocusOnInput();

    d->addWidget_->setVisible(d->list_->count() < maxPollOptions());

    Q_EMIT itemAdded();
}

PollItem* PollItemsList::itemAt(int _index)
{
    return static_cast<PollItem*>(d->list_->itemAt(_index));
}

int PollItemsList::count() const
{
    return d->list_->count();
}

void PollItemsList::paintEvent(QPaintEvent* _event)
{
    if (d->cachedPixmap_.isNull())
    {
        QWidget::paintEvent(_event);
    }
    else
    {
        QPainter painter(this);
        painter.drawPixmap(_event->rect(), d->cachedPixmap_, _event->rect());
    }
}

int PollItemsList::itemsWithTextCount() const
{
    auto count = 0;

    const auto orderedData = d->list_->orderedData();
    for (const auto& itemData : orderedData)
        count += (isValidText(itemData.toString()) ? 1 : 0);

    return count;
}

QStringList PollItemsList::answers() const
{
    QStringList result;

    const auto orderedData = d->list_->orderedData();
    result.reserve(orderedData.size());

    for (const auto& p : orderedData)
        result.push_back(p.toString());

    return result;
}

void PollItemsList::onRemoveItem()
{
    if (auto item = qobject_cast<PollItem*>(sender()))
    {
        auto index = d->list_->indexOf(item);

        d->cachePixmap();
        d->list_->removeItem(item);
        item->deleteLater();
        updateGeometry();
        d->clearCache();

        if (d->list_->count() != 0)
        {
            index = std::clamp(index, 0, d->list_->count() - 1);
            if (auto focusItem = qobject_cast<PollItem*>(d->list_->itemAt(index)))
                focusItem->setFocusOnInput();
        }
        else
        {
            d->addWidget_->setFocus();
        }

        QTimer::singleShot(0, this, [this]() { d->addWidget_->setVisible(d->list_->count() < maxPollOptions()); });
        Q_EMIT itemRemoved();
    }
}


class PollItemPrivate
{
public:
    PollItem* qptr_ = nullptr;
    TextEditEx* answer_ = nullptr;
    CustomButton* dragButton_ = nullptr;
    CustomButton* deleteButton_ = nullptr;

    TextRendering::TextUnitPtr remainedCount_;
    bool drawRemained_ = false;

    PollItemPrivate(PollItem* _q) : qptr_(_q) {}
    void createAnswer();
    void createDeleteButton();
    void createDragButton();
    void createRemainedCount();

    int spaceForRemained(int _itemWidth)
    {
        return _itemWidth - answer_->geometry().right();
    }
};

void PollItemPrivate::createAnswer()
{
    answer_ = new TextEditEx(qptr_, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID }, true, true);
    answer_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    answer_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    answer_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
    answer_->setMinimumWidth(Utils::scale_value(230));
    answer_->setPlaceholderText(QT_TRANSLATE_NOOP("poll", "Answer"));
    answer_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::FollowSettingsRules);
    answer_->setTabChangesFocus(true);
    answer_->setUndoRedoEnabled(true);
    answer_->document()->setDocumentMargin(0);
    answer_->document()->adjustSize();
    answer_->setHeightSupplement(Utils::scale_value(4));
    answer_->adjustHeight(answer_->minimumWidth());
    answer_->viewport()->setCursor(Qt::IBeamCursor);

    qptr_->setFocusProxy(answer_);

    QObject::connect(answer_, &TextEditEx::textChanged, qptr_, &PollItem::onTextChanged);
    QObject::connect(answer_, &TextEditEx::enter, qptr_, &PollItem::enterPressed);
    QObject::connect(answer_, &TextEditEx::textChanged, qptr_, &PollItem::textChanged);
    QObject::connect(answer_, &TextEditEx::focusOut, answer_, &TextEditEx::clearSelection);

    Utils::ApplyStyle(answer_, Styling::getParameters().getTextEditCommonQss(true));
}

void PollItemPrivate::createDeleteButton()
{
    deleteButton_ = new CustomButton(qptr_, qsl(":/delete_icon"), QSize(20, 20), pollItemButtonColorKey());
    deleteButton_->setHoverColor(pollItemButtonHoveredColorKey());
    deleteButton_->setPressedColor(pollItemButtonPressedColorKey());
    QObject::connect(deleteButton_, &CustomButton::clicked, qptr_, &PollItem::remove);
}

void PollItemPrivate::createDragButton()
{
    dragButton_ = new CustomButton(qptr_, qsl(":/smiles_menu/move_sticker_icon"), QSize(20, 20), pollItemButtonColorKey());
    dragButton_->setHoverColor(pollItemButtonHoveredColorKey());
    dragButton_->setPressedColor(pollItemButtonPressedColorKey());
    dragButton_->setCursor(Qt::SizeAllCursor);
    dragButton_->installEventFilter(qptr_);
}

void PollItemPrivate::createRemainedCount()
{
    remainedCount_ = TextRendering::MakeTextUnit(QString());
    remainedCount_->init({ Fonts::adjustedAppFont(15), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY } });
}


PollItem::PollItem(QWidget* _parent)
    : DraggableItem(_parent)
    , d(std::make_unique<PollItemPrivate>(this))
{
    setMinimumHeight(Utils::scale_value(45));
    auto layout = Utils::emptyHLayout(this);

    d->createAnswer();
    layout->addWidget(d->answer_);
    layout->addStretch();

    d->createDeleteButton();
    layout->addWidget(d->deleteButton_);
    layout->addSpacing(Utils::scale_value(20));

    d->createDragButton();
    layout->addWidget(d->dragButton_);

    d->createRemainedCount();
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

} // namespace Ui
