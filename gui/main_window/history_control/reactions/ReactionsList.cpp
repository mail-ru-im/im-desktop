#include "stdafx.h"

#include "utils/utils.h"
#include "utils/DrawUtils.h"
#include "utils/InterConnector.h"
#include "utils/PersonTooltip.h"
#include "core_dispatcher.h"
#include "controls/TextUnit.h"
#include "main_window/containers/FriendlyContainer.h"
#include "controls/TransparentScrollBar.h"
#include "cache/avatars/AvatarStorage.h"
#include "controls/TextWidget.h"
#include "styles/ThemeParameters.h"
#include "previewer/Drawable.h"
#include "previewer/toast.h"
#include "main_window/ConnectionWidget.h"
#include "fonts.h"

#include "ReactionsList.h"
#include "ReactionsListItem.h"

namespace
{
    constexpr std::chrono::milliseconds leaveTimerInterval = std::chrono::milliseconds(50);
    constexpr std::chrono::milliseconds spinnerDelayInterval = std::chrono::milliseconds(300);

    int32_t listWidthUnscaled()
    {
        return 360;
    }

    QSize listSize()
    {
        return Utils::scale_value(QSize(listWidthUnscaled(), 386)); // 450 - button layout height (64)
    }

    QSize itemSize()
    {
        return Utils::scale_value(QSize(listWidthUnscaled(), 44));
    }

    QSize headerSize()
    {
        return Utils::scale_value(QSize(listWidthUnscaled(), 32));
    }

    QSize avatarRectSize()
    {
        return Utils::scale_value(QSize(32, 32));
    }

    int avatarSize()
    {
        return Utils::scale_bitmap(avatarRectSize().width());
    }

    int32_t avatarLeftMargin()
    {
        return Utils::scale_value(12);
    }

    int32_t avatarTopMargin()
    {
        return Utils::scale_value(6);
    }

    int32_t nameLeftMargin()
    {
        return Utils::scale_value(12);
    }

    int32_t nameTopMargin()
    {
        return Utils::scale_value(12);
    }

    int32_t emojiSize()
    {
        return Utils::scale_value(20);
    }

    int32_t headerItemBorderRadius()
    {
        return Utils::scale_value(16);
    }

    int32_t headerItemHeight()
    {
        return Utils::scale_value(32);
    }

    int32_t headerItemRectBorderWidth()
    {
        return Utils::scale_value(1);
    }

    enum class Hovered
    {
        No,
        Yes
    };

    enum class Pressed
    {
        No,
        Yes
    };

    auto headerItemTextColor(Hovered _hovered, Pressed _pressed)
    {
        if (_pressed == Pressed::Yes)
            return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_ACTIVE };
        else if (_hovered == Hovered::Yes)
            return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_HOVER };
        else
            return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    QColor headerItemBackgroundColor(Hovered _hovered, Pressed _pressed)
    {
        if (_pressed == Pressed::Yes)
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_ACTIVE);
        else if (_hovered == Hovered::Yes)
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_HOVER);
        else
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
    }

    const QImage& emojiImage(const QString& _emojiStr)
    {
        static std::unordered_map<QString, QImage> cache;

        auto it = cache.find(_emojiStr);
        if (it != cache.end())
            return it->second;

        auto image = Emoji::GetEmoji(Emoji::EmojiCode::fromQString(_emojiStr), Utils::scale_bitmap(emojiSize()));
        cache[_emojiStr] = std::move(image);

        return cache[_emojiStr];
    }

    QColor itemHoveredBackground()
    {
        static Styling::ColorContainer color{ Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT_INVERSE } };
        return color.actualColor();
    }

    QSize removeButtonSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    int32_t removeButtonRightMargin()
    {
        return Utils::scale_value(16);
    }

    int32_t nameRightMargin()
    {
        return Utils::scale_value(16);
    }

    int32_t removeButtonTopMargin()
    {
        return Utils::scale_value(12);
    }

    Styling::ThemeColorKey itemFontColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    QFont itemFont()
    {
        return Fonts::adjustedAppFont(15);
    }

    QPixmap removeButtonIcon(bool _hovered)
    {
        static auto hovered= Utils::StyledPixmap(qsl(":/controls/close_icon"), removeButtonSize(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY_HOVER });
        static auto normal = Utils::StyledPixmap(qsl(":/controls/close_icon"), removeButtonSize(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });
        return (_hovered ? hovered : normal).actualPixmap();
    }
}

namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// ReactionsList_p
//////////////////////////////////////////////////////////////////////////

class ReactionsList_p
{
public:

    ReactionsList_p(ReactionsList* _q) : q(_q) {}

    int createReactionTab(const QString& _reaction)
    {
        auto content = new ReactionsListTab(_reaction, msgId_, contact_, stackedWidget_);
        QObject::connect(content, &ReactionsListTab::removeMyReactionClicked, q, &ReactionsList::onRemoveMyReactionClicked);
        QObject::connect(content, &ReactionsListTab::itemClicked, q, &ReactionsList::onListItemClicked);

        if (!reactions_.myReaction_.isEmpty() && (_reaction == reactions_.myReaction_ || _reaction.isEmpty()))
            content->addItem(createMyReactionData());

        auto index = stackedWidget_->addWidget(content);
        widgetsIndex_[_reaction] = index;
        contentWidgets_.push_back(content);

        return index;
    }

    Data::ReactionsPage::Reaction createMyReactionData()
    {
        Data::ReactionsPage::Reaction reaction;
        reaction.contact_ = MyInfo()->aimId();
        reaction.reaction_ = reactions_.myReaction_;
        return reaction;
    }

    void removeMyReaction()
    {
        for (auto contentWidget : contentWidgets_)
            contentWidget->removeItem(MyInfo()->aimId());

        for (auto& reaction : reactions_.reactions_)
        {
            if (reaction.reaction_ == reactions_.myReaction_)
                reaction.count_--;
        }

        reactions_.myReaction_.clear();
    }

    ReactionsListHeader* header_;
    std::unordered_map<QString, int, Utils::QStringHasher> widgetsIndex_;
    std::vector<ReactionsListTab*> contentWidgets_;
    QStackedWidget* stackedWidget_;
    Data::Reactions reactions_;
    int64_t removeSeq_;
    QString contact_;
    int64_t msgId_;

    ReactionsList* q;
};

//////////////////////////////////////////////////////////////////////////
// ReactionsList
//////////////////////////////////////////////////////////////////////////

ReactionsList::ReactionsList(int64_t _msgId, const QString& _contact, const QString& _reaction, const Data::Reactions& _reactions, QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<ReactionsList_p>(this))
{
    d->msgId_ = _msgId;
    d->contact_ = _contact;
    d->reactions_ = _reactions;

    setFixedSize(listSize());

    QVBoxLayout* mainLayout = Utils::emptyVLayout(this);
    mainLayout->addSpacing(Utils::scale_value(20));

    QHBoxLayout* labelLayout = Utils::emptyHLayout();
    labelLayout->addSpacing(Utils::scale_value(12));
    auto label = new Ui::TextWidget(this, QT_TRANSLATE_NOOP("reactions", "Reactions"));
    TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(22), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
    params.maxLinesCount_ = 1;
    label->init(params);

    labelLayout->addWidget(label);
    labelLayout->addStretch();
    mainLayout->addLayout(labelLayout);

    d->header_ = new ReactionsListHeader(_parent);
    d->header_->setReactions(_reactions);
    QTimer::singleShot(0, this, [this, _reaction](){ d->header_->setChecked(_reaction); });
    connect(d->header_, &ReactionsListHeader::itemClicked, this, &ReactionsList::onHeaderItemClicked);
    mainLayout->addSpacing(Utils::scale_value(14));
    mainLayout->addWidget(d->header_);
    mainLayout->addSpacing(Utils::scale_value(16));

    d->stackedWidget_ = new QStackedWidget(this);
    mainLayout->addWidget(d->stackedWidget_);

    d->createReactionTab(_reaction);

    connect(GetDispatcher(), &core_dispatcher::reactionRemoveResult, this, &ReactionsList::onReactionRemoveResult);
}

ReactionsList::~ReactionsList()
{

}

void ReactionsList::onHeaderItemClicked(const QString& _reaction)
{
    int widgetIndex;

    auto widgetIndexIt = d->widgetsIndex_.find(_reaction);
    if (widgetIndexIt != d->widgetsIndex_.end())
        widgetIndex = widgetIndexIt->second;
    else
        widgetIndex = d->createReactionTab(_reaction);

    d->stackedWidget_->setCurrentIndex(widgetIndex);
}

void ReactionsList::onListItemClicked(const QString& _contact)
{
    Q_EMIT Utils::InterConnector::instance().profileSettingsShow(_contact);
    Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
}

void ReactionsList::onRemoveMyReactionClicked()
{
    d->removeSeq_ = GetDispatcher()->removeReaction(d->msgId_, d->contact_);
}

void ReactionsList::onReactionRemoveResult(int64_t _seq,bool _success)
{
    if (_seq != d->removeSeq_)
        return;

    if (_success)
    {
        d->removeMyReaction();
        d->header_->setReactions(d->reactions_);
    }
    else
    {
        Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("reactions", "Remove reaction: an error occurred"));
    }
}

//////////////////////////////////////////////////////////////////////////
// ReactionsListHeader_p
//////////////////////////////////////////////////////////////////////////

class ReactionsListHeader_p
{
public:
    ReactionsListHeader_p(ReactionsListHeader* _q) : q(_q) {}

    void createButtons(const Data::Reactions& _reactions)
    {
        for (const auto& reaction : _reactions.reactions_)
        {
            auto it = buttons_.find(reaction.reaction_);
            if (it != buttons_.end())
            {
                it->second->setText(QString::number(reaction.count_));
            }
            else if (reaction.count_ > 0)
            {
                auto item = new ReactionsListHeaderItem(scrollArea_);
                item->setTextAndReaction(QString::number(reaction.count_), reaction.reaction_);
                Testing::setAccessibleName(item, u"AS ReactionPopup category " % reaction.reaction_);

                buttons_[reaction.reaction_] = item;
                buttonsLayout_->addSpacing(Utils::scale_value(8));
                addItem(item);
            }
        }
    }

    void addItem(ReactionsListHeaderItem* _item)
    {
        buttonsLayout_->addWidget(_item);
        QObject::connect(_item, &ReactionsListHeaderItem::clicked, q, &ReactionsListHeader::onItemClicked);
    }

    void createAllButton()
    {
        auto item = new ReactionsListHeaderItem(q);
        item->setText(QT_TRANSLATE_NOOP("reactions", "All"));
        Testing::setAccessibleName(item, qsl("AS ReactionPopup category all"));

        buttons_[QString()] = item; // empty key for All button
        addItem(item);
    }

    std::unordered_map<QString, ReactionsListHeaderItem*, Utils::QStringHasher> buttons_;
    QScrollArea* scrollArea_;
    QHBoxLayout* buttonsLayout_;
    ReactionsListHeader* q;
};

//////////////////////////////////////////////////////////////////////////
// ReactionsListHeader
//////////////////////////////////////////////////////////////////////////

ReactionsListHeader::ReactionsListHeader(QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<ReactionsListHeader_p>(this))
{
    setFixedSize(headerSize());
    auto layout = Utils::emptyHLayout(this);

    d->scrollArea_ = new QScrollArea(this);
    d->scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea_->setStyleSheet(qsl("background-color: transparent; border: none"));

    auto contentWidget = new QWidget(d->scrollArea_);
    Testing::setAccessibleName(contentWidget, qsl("AS ReactionPopup categories"));
    d->scrollArea_->setWidget(contentWidget);
    d->scrollArea_->setWidgetResizable(true);
    d->scrollArea_->setFocusPolicy(Qt::NoFocus);
    auto contentLayout = Utils::emptyHLayout(contentWidget);
    d->buttonsLayout_ = Utils::emptyHLayout();
    contentLayout->addLayout(d->buttonsLayout_);
    contentLayout->addStretch();

    layout->addSpacing(Utils::scale_value(12));
    layout->addWidget(d->scrollArea_);

    d->createAllButton();
}

ReactionsListHeader::~ReactionsListHeader()
{

}

void ReactionsListHeader::setReactions(const Data::Reactions& _reactions)
{
    d->createButtons(_reactions);
    update();
}

void ReactionsListHeader::setChecked(const QString& _reaction)
{
    auto it = d->buttons_.find(_reaction);
    if (it != d->buttons_.end())
    {
        d->scrollArea_->ensureWidgetVisible(it->second);
        it->second->setChecked(true);
    }
}

void ReactionsListHeader::onItemClicked(const QString& _reaction)
{
    for (auto& [reaction, button] : d->buttons_)
        button->setChecked(reaction == _reaction);

    Q_EMIT itemClicked(_reaction);
}

//////////////////////////////////////////////////////////////////////////
// ReactionsListHeaderItem_p
//////////////////////////////////////////////////////////////////////////

class ReactionsListHeaderItem_p
{
public:
    QSize createContent()
    {
        auto currentX = 0;
        if (!reaction_.isEmpty())
        {
            emojiRect_ = QRect(Utils::scale_value(12), Utils::scale_value(6), emojiSize(), emojiSize());
            currentX = emojiRect_.right();
        }

        if (!text_.isEmpty())
        {
            if (currentX == 0)
                currentX += Utils::scale_value(12);
            else
                currentX += Utils::scale_value(4);

            label_ = std::make_unique<Label>();
            label_->setTextUnit(TextRendering::MakeTextUnit(text_));
            label_->initTextUnit({ Fonts::adjustedAppFont(14), headerItemTextColor(Hovered::No, Pressed::No) });

            label_->setDefaultColor(headerItemTextColor(Hovered::No, Pressed::No));
            label_->setHoveredColor(headerItemTextColor(Hovered::Yes, Pressed::No));
            label_->setPressedColor(headerItemTextColor(Hovered::No, Pressed::Yes));
            label_->setVerticalPosition(TextRendering::VerPosition::MIDDLE);
            label_->setYOffset(headerItemHeight() / 2);

            auto desiredWidth = label_->desiredWidth();
            label_->setRect({ currentX, 0, desiredWidth, headerItemHeight() });

            currentX += desiredWidth + Utils::scale_value(12);
        }

        return QSize(currentX, headerItemHeight());
    }

    void setHovered(bool _hovered)
    {
        label_->setHovered(_hovered);
        hovered_ = _hovered;
    }

    void setPressed(bool _pressed)
    {
        label_->setPressed(_pressed);
        pressed_ = _pressed;
    }

    QRect emojiRect_;
    std::unique_ptr<Label> label_;
    QString reaction_;
    QString text_;
    bool hovered_ = false;
    bool pressed_ = false;
    bool checked_ = false;
};

//////////////////////////////////////////////////////////////////////////
// ReactionsListHeaderItem
//////////////////////////////////////////////////////////////////////////

ReactionsListHeaderItem::ReactionsListHeaderItem(QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<ReactionsListHeaderItem_p>())
{
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
}

ReactionsListHeaderItem::~ReactionsListHeaderItem()
{

}

void ReactionsListHeaderItem::setText(const QString& _text)
{
    d->text_ = _text;
    setFixedSize(d->createContent());

    update();
}

void ReactionsListHeaderItem::setReaction(const QString& _reaction)
{
    d->reaction_ = _reaction;
    setFixedSize(d->createContent());

    update();
}

void ReactionsListHeaderItem::setTextAndReaction(const QString& _text, const QString& _reaction)
{
    d->text_ = _text;
    d->reaction_ = _reaction;
    setFixedSize(d->createContent());

    update();
}

void ReactionsListHeaderItem::setChecked(bool _checked)
{
    d->checked_ = _checked;
    update();
}

void ReactionsListHeaderItem::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    const auto border = headerItemRectBorderWidth();
    auto itemRect = rect().adjusted(border, border, -border, -border);

    path.addRoundedRect(itemRect, headerItemBorderRadius(), headerItemBorderRadius());

    if (d->checked_)
    {
        p.fillPath(path, headerItemBackgroundColor(Hovered::No, Pressed::No));
    }
    else
    {
        p.setPen(headerItemBackgroundColor(d->hovered_ ? Hovered::Yes : Hovered::No, d->pressed_ ? Pressed::Yes : Pressed::No));
        p.drawPath(path);
    }

    if (!d->emojiRect_.isEmpty())
        p.drawImage(d->emojiRect_, emojiImage(d->reaction_));

    d->label_->draw(p);

    QWidget::paintEvent(_event);
}

void ReactionsListHeaderItem::mouseMoveEvent(QMouseEvent* _event)
{
    if (!d->hovered_)
    {
        d->setHovered(true);
        update();
    }
    QWidget::mouseMoveEvent(_event);
}

void ReactionsListHeaderItem::mouseReleaseEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton && rect().contains(_event->pos()))
        Q_EMIT clicked(d->reaction_);

    d->setPressed(false);

    QWidget::mouseReleaseEvent(_event);
}

void ReactionsListHeaderItem::enterEvent(QEvent* _event)
{
    d->setHovered(true);
    update();
    QWidget::enterEvent(_event);
}

void ReactionsListHeaderItem::leaveEvent(QEvent* _event)
{
    d->setHovered(false);
    update();
    QWidget::leaveEvent(_event);
}

//////////////////////////////////////////////////////////////////////////
// ReactionsListTab_p
//////////////////////////////////////////////////////////////////////////

class ReactionsListTab_p
{
public:
    void requestNextPage()
    {
        seq_ = GetDispatcher()->getReactionsPage(msgId_, contact_, reaction_, QString(), olderCursor_, 20);
    }

    ProgressAnimation* progressAnimation_;
    ReactionsListContent* content_;
    QScrollArea* scrollArea_;
    QWidget* placeholder_;
    QTimer spinnerTimer_;
    bool exhausted_ = false;
    bool failed_ = false;
    QString olderCursor_;
    QString reaction_;
    QString contact_;
    int64_t msgId_;
    int64_t seq_ = 0;
};

//////////////////////////////////////////////////////////////////////////
// ReactionsListTab
//////////////////////////////////////////////////////////////////////////

ReactionsListTab::ReactionsListTab(const QString& _reaction, int64_t _msgId, const QString& _contact, QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<ReactionsListTab_p>())
{
    d->reaction_ = _reaction;
    d->contact_ = _contact;
    d->msgId_ = _msgId;

    d->olderCursor_ = QString::number(0);

    auto layout = Utils::emptyVLayout(this);

    d->placeholder_ = new QWidget(this);
    d->progressAnimation_ = new ProgressAnimation(this);
    d->progressAnimation_->setFixedSize(Utils::scale_value(QSize(32, 32)));
    d->progressAnimation_->setProgressWidth(Utils::scale_value(28));
    d->progressAnimation_->setVisible(false);
    auto placeholderLayout = Utils::emptyVLayout(d->placeholder_);
    placeholderLayout->setAlignment(Qt::AlignHCenter);
    placeholderLayout->addSpacing(Utils::scale_value(92));
    placeholderLayout->addWidget(d->progressAnimation_);
    placeholderLayout->addStretch();
    d->spinnerTimer_.setSingleShot(true);
    d->spinnerTimer_.setInterval(spinnerDelayInterval.count());
    d->spinnerTimer_.start();

    d->scrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
    d->scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea_->setFocusPolicy(Qt::NoFocus);
    d->scrollArea_->setWidgetResizable(true);
    d->scrollArea_->setStyleSheet(qsl("background-color: transparent; border: none"));

    d->content_ = new ReactionsListContent(d->scrollArea_);
    d->scrollArea_->setWidget(d->content_);
    d->scrollArea_->setVisible(false);

    layout->addWidget(d->placeholder_);
    layout->addWidget(d->scrollArea_);

    connect(GetDispatcher(), &core_dispatcher::reactionsListResult, this, &ReactionsListTab::onReactionsPage);
    connect(GetDispatcher(), &core_dispatcher::connectionStateChanged, this, &ReactionsListTab::onConnectionState);
    connect(d->scrollArea_->verticalScrollBar(), &QScrollBar::valueChanged, this, &ReactionsListTab::onScrolled, Qt::QueuedConnection);
    connect(d->content_, &ReactionsListContent::itemClicked, this, &ReactionsListTab::itemClicked);
    connect(d->content_, &ReactionsListContent::removeMyReactionClicked, this, &ReactionsListTab::removeMyReactionClicked);
    connect(&d->spinnerTimer_, &QTimer::timeout, this, &ReactionsListTab::onSpinnerTimer);

    d->requestNextPage();
}

ReactionsListTab::~ReactionsListTab()
{

}

void ReactionsListTab::removeItem(const QString& _contact)
{
    d->content_->removeItem(_contact);
}

void ReactionsListTab::addItem(const Data::ReactionsPage::Reaction& _reaction)
{
    Data::ReactionsPage page;
    page.reactions_.push_back(_reaction);
    d->content_->addReactions(page);
}

void ReactionsListTab::onReactionsPage(int64_t _seq, const Data::ReactionsPage& _page, bool _success)
{
    if (_seq == d->seq_)
    {
        if (_success)
        {
            if (d->placeholder_->isVisible())
            {
                d->spinnerTimer_.stop();
                d->progressAnimation_->stop();
                d->placeholder_->hide();
                d->scrollArea_->show();
            }

            d->olderCursor_ = _page.oldest_;
            d->exhausted_ = _page.exhausted_;
            d->content_->addReactions(_page);
            d->seq_ = 0;
        }
        else
        {
            d->failed_ = true;
        }
    }
}

void ReactionsListTab::onConnectionState(const ConnectionState& _state)
{
    if (_state == ConnectionState::stateOnline && std::exchange(d->failed_, false))
        d->requestNextPage();
}

void ReactionsListTab::onScrolled(int _value)
{
    if (d->seq_ != 0 || d->exhausted_)
        return;

    auto scroll = d->scrollArea_->verticalScrollBar();
    if (scroll->maximum() - _value < 10 * itemSize().height())
        d->requestNextPage();
}

void ReactionsListTab::onSpinnerTimer()
{
    d->progressAnimation_->show();
    d->progressAnimation_->start();
}

//////////////////////////////////////////////////////////////////////////
// ReactionsListContent_p
//////////////////////////////////////////////////////////////////////////

class ReactionsListContent_p
{
public:

    using Items = std::deque<ReactionsListItem*>;
    using ItemIterator = Items::iterator;

    ReactionsListContent_p(QWidget* _q) : q(_q) {}

    void drawItem(QPainter& _p, const ReactionsListItem* _item)
    {
        Utils::PainterSaver saver(_p);

        _p.setOpacity(_item->opacity_);

        if (_item->hovered_ && !_item->myReaction_)
            _p.fillRect(_item->rect_, itemHoveredBackground());

        if (_item->myReaction_)
            _p.drawPixmap(_item->buttonRect_, removeButtonIcon(_item->buttonHovered_));

        if (!_item->avatar_.isNull())
            _p.drawPixmap(_item->avatarRect_, _item->avatar_);

        _p.drawImage(_item->emojiRect_, emojiImage(_item->reaction_));

        _item->nameUnit_->draw(_p);
    }

    void drawInRect(QPainter& _p, const QRect& _rect)
    {
        auto it = itemIteratorAt(_rect.top());

        while (it != items_.end())
        {
            auto item = *it;
            if (!item)
                continue;

            if (item->rect_.top() > _rect.bottom())
                break;

            drawItem(_p, item);
            ++it;
        }
    }

    void updateItemsPositions(ItemIterator _itBegin, ItemIterator _itEnd, int32_t _yOffset)
    {
        auto it = _itBegin;
        while (it != _itEnd && it != items_.end())
        {
            auto& item = *it;
            item->rect_ = QRect({0, _yOffset}, itemSize());
            const auto topLeft = item->rect_.topLeft();
            item->avatarRect_ = QRect(topLeft + QPoint(avatarLeftMargin(), avatarTopMargin()), avatarRectSize());
            item->emojiRect_ = QRect(item->avatarRect_.topLeft() + QPoint(Utils::scale_value(16), Utils::scale_value(16)), QSize(emojiSize(), emojiSize()));

            if (item->contact_ == MyInfo()->aimId())
            {
                item->buttonRect_ = QRect({ item->rect_.right() - removeButtonSize().width() - removeButtonRightMargin(), item->rect_.top() + removeButtonTopMargin() }, removeButtonSize());
                item->myReaction_ = true;
            }

            const auto nameLeft = item->avatarRect_.right() + nameLeftMargin();
            auto availableWidth = itemSize().width() - nameLeft - nameRightMargin();
            if (!item->buttonRect_.isEmpty())
                availableWidth -= removeButtonSize().width() + removeButtonRightMargin();

            item->nameUnit_->getHeight(availableWidth);
            item->nameUnit_->setOffsets(nameLeft, _yOffset + nameTopMargin());

            _yOffset += itemSize().height();
            ++it;
        }

        q->setFixedSize(itemSize().width(), calcHeight());
    }

    bool isValidIndex(int64_t _index)
    {
        return _index >= 0 && _index < items_.size();
    }

    int32_t calcHeight()
    {
        return items_.size() * itemSize().height();
    }

    ReactionsListItem* itemAt(int _yPos)
    {
        auto it = itemIteratorAt(_yPos);
        if (it != items_.end())
            return *it;

        return nullptr;
    }

    ItemIterator itemIteratorAt(int _yPos)
    {
        if (_yPos < 0 || _yPos > calcHeight())
            return items_.end();

        auto index = _yPos / itemSize().height();
        return items_.begin() + index;
    }

    Items items_;
    std::unordered_map<QString, QPointer<ReactionsListItem>, Utils::QStringHasher> itemsIndexByContact_;
    QPointer<ReactionsListItem> hoveredItem_;
    QPointer<ReactionsListItem> pressedItem_;
    bool firstLoad_ = true;
    QTimer leaveTimer_;
    QPoint pressPos_;
    QWidget* q;
};

//////////////////////////////////////////////////////////////////////////
// ReactionsListContent
//////////////////////////////////////////////////////////////////////////

ReactionsListContent::ReactionsListContent(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<ReactionsListContent_p>(this))
    , personTooltip_(new Utils::PersonTooltip(_parent))
{
    setMouseTracking(true);

    d->leaveTimer_.setInterval(leaveTimerInterval.count());
    connect(&d->leaveTimer_, &QTimer::timeout, this, &ReactionsListContent::onLeaveTimer);
}

ReactionsListContent::~ReactionsListContent()
{

}

void ReactionsListContent::addReactions(const Data::ReactionsPage& _page)
{
    auto currentCount = d->items_.size();
    auto currentHeight = d->calcHeight();
    for (auto& reaction : _page.reactions_)
    {
        if (d->itemsIndexByContact_.find(reaction.contact_) != d->itemsIndexByContact_.end())
            continue;

        auto item = new ReactionsListItem(this);
        item->contact_ = reaction.contact_;
        item->reaction_ = reaction.reaction_;

        const auto friendly = Logic::GetFriendlyContainer()->getFriendly(reaction.contact_);

        bool isDefault;
        item->avatar_ = Logic::GetAvatarStorage()->GetRounded(reaction.contact_, friendly, avatarSize(), isDefault, false, false);
        item->nameUnit_ = TextRendering::MakeTextUnit(friendly, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        TextRendering::TextUnit::InitializeParameters params{ itemFont(), itemFontColor() };
        params.maxLinesCount_ = 1;
        item->nameUnit_->init(params);

        d->items_.push_back(item);
        d->itemsIndexByContact_[item->contact_] = item;
        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &ReactionsListContent::onAvatarChanged);
    }

    d->updateItemsPositions(d->items_.begin() + currentCount, d->items_.end(), currentHeight);
}

void ReactionsListContent::removeItem(const QString& _contact)
{
    auto it = d->itemsIndexByContact_.find(_contact);
    if (it != d->itemsIndexByContact_.end())
    {
        if (auto item = d->itemsIndexByContact_[_contact])
            d->items_.erase(d->itemIteratorAt(item->rect_.center().y()));

        d->itemsIndexByContact_.erase(_contact);
        d->updateItemsPositions(d->items_.begin(), d->items_.end(), 0);
        update();
    }
}

void ReactionsListContent::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    d->drawInRect(p, _event->rect());
}

void ReactionsListContent::mouseMoveEvent(QMouseEvent* _event)
{
    auto lastHoveredItem = d->hoveredItem_;
    auto handCursor = false;
    const auto pos = _event->pos();

    if (!lastHoveredItem || !lastHoveredItem->rect_.contains(pos))
    {
        if (auto hoveredItem = d->itemAt(pos.y()))
        {
            hoveredItem->hovered_ = true;
            hoveredItem->buttonHovered_ = hoveredItem->buttonRect_.contains(pos);
            QRect updateRect;
            if (lastHoveredItem)
            {
                lastHoveredItem->hovered_ = false;
                lastHoveredItem->buttonHovered_ = false;
                updateRect |= lastHoveredItem->rect_;
            }

            updateRect |= hoveredItem->rect_;
            d->hoveredItem_ = hoveredItem;
            update(updateRect);

            handCursor = !hoveredItem->myReaction_ || hoveredItem->buttonHovered_;
        }
    }
    else if (lastHoveredItem)
    {
        lastHoveredItem->buttonHovered_ = lastHoveredItem->buttonRect_.contains(pos);
        update(lastHoveredItem->rect_);

        handCursor = !lastHoveredItem->myReaction_ || lastHoveredItem->buttonHovered_;
        if (!lastHoveredItem->avatarRect_.contains(pos))
            personTooltip_->hide();
    }

    setCursor(handCursor ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (const auto avatarRect = d->hoveredItem_->avatarRect_; avatarRect.isValid() && avatarRect.contains(pos))
    {
        const auto globalAvatarRect = QRect(mapToGlobal(avatarRect.topLeft()), avatarRect.size());
        personTooltip_->show(Utils::PersonTooltipType::Person, d->hoveredItem_->contact_, globalAvatarRect, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Top);
    }

    QWidget::mouseMoveEvent(_event);
}

void ReactionsListContent::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
    {
        if (auto pressedItem = d->itemAt(_event->pos().y()))
        {
            pressedItem->buttonPressed_ = pressedItem->buttonRect_.contains(_event->pos());
            d->pressedItem_ = pressedItem;
            update(pressedItem->rect_);
        }
        d->pressPos_ = _event->pos();
    }

    QWidget::mousePressEvent(_event);
}

void ReactionsListContent::mouseReleaseEvent(QMouseEvent* _event)
{
    if (d->pressedItem_)
    {
        if (d->pressedItem_->buttonRect_.contains(_event->pos()) && d->pressedItem_->buttonRect_.contains(d->pressPos_))
            Q_EMIT removeMyReactionClicked();
        else if (d->pressedItem_->rect_.contains(_event->pos()) && d->pressedItem_->rect_.contains(d->pressPos_) && !d->pressedItem_->myReaction_)
            Q_EMIT itemClicked(d->pressedItem_->contact_);
    }

    d->pressPos_ = QPoint();
    d->pressedItem_.clear();

    QWidget::mouseReleaseEvent(_event);
}

void ReactionsListContent::leaveEvent(QEvent* _event)
{
    d->leaveTimer_.start();
    QWidget::leaveEvent(_event);
}

void ReactionsListContent::enterEvent(QEvent* _event)
{
    d->leaveTimer_.stop();
    QWidget::enterEvent(_event);
}

void ReactionsListContent::onAvatarChanged(const QString& _contact)
{
    auto it = d->itemsIndexByContact_.find(_contact);
    if (it != d->itemsIndexByContact_.end())
    {
        if (auto item = it->second)
        {
            bool isDefault;
            const auto friendly = Logic::GetFriendlyContainer()->getFriendly(item->contact_);
            item->avatar_ = Logic::GetAvatarStorage()->GetRounded(item->contact_, friendly, avatarSize(), isDefault, false, false);
            update(item->rect_);
        }
    }
}

void ReactionsListContent::onLeaveTimer()
{
    const auto pos = QCursor::pos();
    auto parent = parentWidget();
    const auto insideParent = parent && parent->rect().contains(parent->mapFromGlobal(pos));
    const auto insideRect = rect().contains(mapFromGlobal(pos));
    if (!insideParent || !insideRect)
    {
        if (d->hoveredItem_)
        {
            d->hoveredItem_->hovered_ = false;
            d->hoveredItem_->buttonHovered_ = false;
            update(d->hoveredItem_->rect_);
        }

        d->hoveredItem_.clear();
        d->leaveTimer_.stop();
        personTooltip_->hide();
    }
}

int AccessibleReactionsListContent::childCount() const
{
    return list_ ? list_->d->items_.size() : 0;
}

QAccessibleInterface* AccessibleReactionsListContent::child(int _index) const
{
    if (!list_ || _index < 0 || _index >= childCount())
        return nullptr;

    return QAccessible::queryAccessibleInterface(list_->d->items_[_index]);
}

int AccessibleReactionsListContent::indexOfChild(const QAccessibleInterface* _child) const
{
    if (!list_ || !_child)
        return -1;

    const auto item = qobject_cast<ReactionsListItem*>(_child->object());
    return Utils::indexOf(list_->d->items_.cbegin(), list_->d->items_.cend(), item);
}

QString AccessibleReactionsListContent::text(QAccessible::Text _type) const
{
    return list_ && _type == QAccessible::Text::Name ? qsl("AS ReactionPopup members") : QString();
}

}
