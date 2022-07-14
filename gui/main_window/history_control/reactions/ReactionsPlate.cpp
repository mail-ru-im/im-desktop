#include "stdafx.h"

#include "core_dispatcher.h"
#include "controls/TextUnit.h"
#include "controls/GeneralDialog.h"
#include "controls/TooltipWidget.h"
#include "styles/ThemeParameters.h"
#include "main_window/MainWindow.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/containers/FriendlyContainer.h"
#include "../HistoryControlPageItem.h"
#include "utils/InterConnector.h"
#include "../MessageStyle.h"
#include "utils/DrawUtils.h"
#include "utils/utils.h"
#include "fonts.h"

#include "ReactionsPlate.h"
#include "ReactionsList.h"
#include "ReactionItem.h"

namespace
{
    constexpr std::chrono::milliseconds animationDuration = std::chrono::milliseconds(150);
    constexpr std::chrono::milliseconds updateAnimationDuration = std::chrono::milliseconds(150);
    constexpr auto maxTooltipReactionsCount = 13u;
    constexpr size_t maxReactionsCount = 3;

    int32_t plateHeight()
    {
        return Ui::MessageStyle::Plates::plateHeight();
    }

    QColor plateColor(Ui::ReactionsPlateType _type, bool _outgoing)
    {
        if (_type == Ui::ReactionsPlateType::Regular)
        {
            if (_outgoing)
            {
                static Styling::ColorContainer color = Styling::ThemeColorKey { Styling::StyleVariable::CHAT_TERTIARY };
                return color.actualColor();
            }

            static Styling::ColorContainer color = Styling::ThemeColorKey { Styling::StyleVariable::BASE_LIGHT };
            return color.actualColor();
        }

        static Styling::ColorContainer color = Styling::ThemeColorKey { Styling::StyleVariable::GHOST_SECONDARY };
        return color.actualColor();
    }

    int32_t plateEmojiSize()
    {
        return Utils::scale_value(19);
    }

    int32_t plateSideMargin()
    {
        return Utils::scale_value(8);
    }

    int32_t plateTopMargin()
    {
        return Utils::scale_value(3);
    }

    int32_t textMargin()
    {
        return Utils::scale_value(8);
    }

    QFont countFont()
    {
        return Fonts::adjustedAppFont(12, Fonts::FontWeight::Normal);
    }

    Styling::ThemeColorKey countTextColor(Ui::ReactionsPlateType _type, bool _outgoing)
    {
        if (_type == Ui::ReactionsPlateType::Regular)
        {
            if (_outgoing)
                return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_PASTEL };
            else
                return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
        }
        else
        {
            return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT };
        }
    }

    QString countText(int _count)
    {
        const auto threshold = 1000;
        if (_count < threshold)
            return QString::number(_count);
        else
            return qsl("%1k").arg(static_cast<double>(_count) / threshold, 0, 'g', 2);
    }

    bool allowedToViewReactionsList(const QString& _aimId)
    {
        if (Logic::getContactListModel()->isThread(_aimId))
            return true;

        const auto role = Logic::getContactListModel()->getYourRole(_aimId);
        const auto notAllowed = (role.isEmpty() && Utils::isChat(_aimId)) || role == u"notamember" || Logic::getContactListModel()->isChannel(_aimId) && !Logic::getContactListModel()->areYouAdmin(_aimId);

        return !notAllowed;
    }
}

namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// ReactionsPlate_p
//////////////////////////////////////////////////////////////////////////

class ReactionsPlate_p
{
public:
    using ReactionItems = std::vector<ReactionItem*>;

    ReactionsPlate_p(ReactionsPlate* _q)
        : opacityAnimation_(new QVariantAnimation(_q))
        , geometryAnimation_(new QVariantAnimation(_q))
        , q(_q)
    {
        geometryAnimation_->setStartValue(0.0);
        geometryAnimation_->setEndValue(1.0);
        geometryAnimation_->setEasingCurve(QEasingCurve::InOutSine);
        geometryAnimation_->setDuration(updateAnimationDuration.count());

        countUnit_ = TextRendering::MakeTextUnit(QString());
        countUnit_->init({ countFont(), countTextColor(ReactionsPlateType::Regular, outgoingPosition_) });
    }

    struct PageRequest
    {
        QString reaction_;
        int64_t seq_ = 0;
    };

    enum class UpdateMode
    {
        Update,
        Add,
    };

    ReactionItem* createReactionItem(const Data::Reactions::Reaction& _reaction, const QString& _myReaction, int64_t _msgId)
    {
        auto item = new ReactionItem(q);
        item->reaction_ = _reaction.reaction_;
        item->emoji_ = Emoji::GetEmoji(Emoji::EmojiCode::fromQString(_reaction.reaction_), Utils::scale_bitmap(plateEmojiSize()));
        item->count_ = _reaction.count_;
        item->msgId_ = _msgId;

        return item;
    }

    size_t getTopReactionsSize()
    {
        return std::min(items_.size(), maxReactionsCount);
    }

    int updateCounter(int _precounted = 0)
    {
        auto totalCounter = _precounted;
        for (size_t i = 0; i < items_.size(); ++i)
            totalCounter += items_[i]->count_;

        if (totalCounter > 0)
        {
            countUnit_->setText(countText(totalCounter));
            countUnit_->init({ countFont(), countTextColor(item_->reactionsPlateType(), outgoingPosition_) });
        }
        q->update();
        return totalCounter;
    }

    void clearCounters()
    {
        counterAboutToAddExisting_ = 0;
        counterAboutToDeleteExisting_ = 0;
    }

    void updateReactionItem(ReactionItem* _item, const Data::Reactions::Reaction& _reaction, const QString& _myReaction)
    {
        _item->count_ = _reaction.count_;
    }

    void updateItems(const Data::Reactions& _reactions)
    {
        clearCounters();
        for (auto& reaction : _reactions.reactions_)
        {
            if (reaction.count_ == 0)
                continue;

            auto it = std::find_if(items_.begin(), items_.end(), [reaction](const auto& _item) { return _item->reaction_ == reaction.reaction_; });
            if (it != items_.end())
                updateReactionItem(*it, reaction, _reactions.myReaction_);
            else
                items_.push_back(createReactionItem(reaction, _reactions.myReaction_, _reactions.msgId_));
        }

        std::sort(items_.begin(), items_.end(), [](const auto& _i1, const auto& _i2){ return _i1->count_ > _i2->count_; });
    }

    int updateItemGeometry(ReactionItem* _item, int _xOffset)
    {
        auto startXOffset = _xOffset;
        _item->emojiRect_ = QRect(_xOffset, plateTopMargin(), plateEmojiSize(), plateEmojiSize());
        _xOffset += _item->emojiRect_.width();
        _item->rect_ = QRect(_item->emojiRect_.topLeft(), QSize(plateEmojiSize(), plateHeight()));

        return _xOffset - startXOffset;
    }

    std::vector<int> calcItemsOffsets(ReactionItems& _items, int& _xOffset)
    {
        std::vector<int> offsets_;
        for (size_t i = 0; i < items_.size(); ++i)
        {
            offsets_.push_back(_xOffset);
            _xOffset += items_[i]->emojiRect_.width();
        }

        return offsets_;
    }

    int updateItemsGeometry(ReactionItems& _items, int _xOffset = 0, UpdateMode _mode = UpdateMode::Update)
    {
        if (_mode == UpdateMode::Add && (_items.empty() || items_.size() >= maxReactionsCount))
            return countUnit_->desiredWidth() + 2 * textMargin();

        size_t itemsSize = _mode == UpdateMode::Add
                           ? std::min(_items.size(), (maxReactionsCount - items_.size()))
                           : std::min(_items.size(), maxReactionsCount);

        auto startXOffset = _xOffset;
        for (size_t i = 0; i < itemsSize; ++i)
            _xOffset += updateItemGeometry(_items[i], _xOffset);

        auto updatedCounter = _mode == UpdateMode::Update && addedReactions_.empty() && deletedReactions_.empty()
                              ? updateCounter()
                              : countUnit_->getText().toInt();

        if (updatedCounter != 0)
        {
            const auto countWidth = countUnit_->desiredWidth();
            countUnit_->getHeight(countWidth);
            _xOffset += countWidth + 2 * textMargin();
        }

        return _xOffset - startXOffset;
    }

    QPoint calcPlatePosition(int _plateWidth)
    {
        const auto messageRect = item_->messageRect();

        const auto top = messageRect.bottom() + MessageStyle::Plates::plateOffsetY(item_->reactionsPlateType());
        auto left = 0;

        if (outgoingPosition_)
        {
            left = messageRect.right() - MessageStyle::Plates::plateOffsetX() - _plateWidth;
        }
        else
        {
            left = messageRect.left() + MessageStyle::Plates::plateOffsetX();
            if (auto threadRect = item_->threadPlateRect(); threadRect.width() > 0)
                left += threadRect.width() + MessageStyle::Plates::plateOffsetX();
        }

        return QPoint(left, top);
    }

    QRect updateGeometryHelper()
    {
        const auto plateType = item_->reactionsPlateType();
        shadow_->setEnabled(plateType == ReactionsPlateType::Regular);

        const auto itemsWidth = updateItemsGeometry(items_, plateSideMargin());
        const auto plateWidth = itemsWidth > 0 ? itemsWidth + plateSideMargin() : 0;
        const auto plateSize = QSize(plateWidth, MessageStyle::Plates::plateHeightWithShadow(plateType));

        const QRect newRect(calcPlatePosition(plateSize.width()), plateSize);
        return newRect;
    }

    void setPlateGeometry(const QRect& _geometry)
    {
        q->setGeometry(_geometry);
        Q_EMIT q->platePositionChanged();
    }

    void draw(QPainter& _p)
    {
        QPainterPath platePath;
        const auto plateRect = QRect(0, 0, q->width(), plateHeight());
        platePath.addRoundedRect(plateRect, MessageStyle::Plates::borderRadius(), MessageStyle::Plates::borderRadius());

        Utils::drawPlateSolidShadow(_p, platePath);

        _p.setOpacity(opacity_);
        _p.fillPath(platePath, plateColor(item_->reactionsPlateType(), outgoingPosition_));

        for (size_t i = 0; i < getTopReactionsSize(); ++i)
        {
            _p.setOpacity(items_[i]->opacity_ * opacity_);
            _p.drawImage(items_[i]->emojiRect_, items_[i]->emoji_);
        }

        _p.setOpacity(opacity_);
        countUnit_->setOffsets(q->width() - countUnit_->cachedSize().width() - textMargin(), plateHeight() / 2);
        countUnit_->draw(_p, TextRendering::VerPosition::MIDDLE);
    }

    void startShowAnimation()
    {
        updateInProgress_ = true;
        opacity_ = 0;

        opacityAnimation_->stop();
        opacityAnimation_->disconnect(q);
        opacityAnimation_->setStartValue(0.0);
        opacityAnimation_->setEndValue(1.0);
        opacityAnimation_->setEasingCurve(QEasingCurve::InOutSine);
        opacityAnimation_->setDuration(animationDuration.count());

        QObject::connect(opacityAnimation_, &QVariantAnimation::valueChanged, q, [this](const QVariant& value)
        {
            opacity_ = value.toDouble();
            shadow_->setColor(Utils::plateShadowColorWithAlpha(opacity_));
            q->update();
        });
        QObject::connect(opacityAnimation_, &QVariantAnimation::finished, q, [this]()
        {
            updateInProgress_ = false;
            startQueuedAnimation();
        });

        opacityAnimation_->start();

        q->show();
    }

    void startUpdateAnimation()
    {
        updateInProgress_ = true;

        opacityAnimation_->stop();
        opacityAnimation_->disconnect(q);
        opacityAnimation_->setStartValue(1.0);
        opacityAnimation_->setEndValue(0.0);
        opacityAnimation_->setEasingCurve(QEasingCurve::OutInSine);
        opacityAnimation_->setDuration(updateAnimationDuration.count());

        QObject::connect(opacityAnimation_, &QVariantAnimation::valueChanged, q, [this](const QVariant& value)
        {
            if (deletedReactions_.empty())
                return;

            const auto opacity = value.toDouble();
            for (size_t i = 0; i < getTopReactionsSize(); ++i)
            {
                if (deletedReactions_.count(items_[i]->reaction_))
                    items_[i]->opacity_ = opacity;
            }
            q->update();
        });
        QObject::connect(opacityAnimation_, &QVariantAnimation::finished, q, [this]()
        {
            startGeometryAnimation();
        });

        opacityAnimation_->start();
    }

    void startGeometryAnimation()
    {
        auto currentWidth = q->width();

        std::vector<int> currentOffsets;

        for (size_t i = 0; i < items_.size(); ++i)
            currentOffsets.push_back(items_[i]->emojiRect_.left());

        bool isGeometryUpdated = true;
        if (counterAboutToAddExisting_ == 0)
        {
            updateCounter(counterAboutToDeleteExisting_);
            counterAboutToDeleteExisting_ = 0;
        }
        else if (counterAboutToDeleteExisting_ < 0 && (counterAboutToAddExisting_ + counterAboutToDeleteExisting_ != 0))
        {
            updateCounter(counterAboutToDeleteExisting_ + counterAboutToAddExisting_);
            clearCounters();
        }

        if (counterAboutToAddExisting_ + counterAboutToDeleteExisting_ == 0)
            clearCounters();

        for (auto& reaction : deletedReactions_)
        {
            auto it = std::find_if(items_.begin(), items_.end(), [reaction](const auto& _item) { return _item->reaction_ == reaction; });
            if (it != items_.end())
                items_.erase(it);
        }

        auto xOffset = plateSideMargin();
        const auto newOffsets = calcItemsOffsets(items_, xOffset);
        const auto sizeAfterDeletion = xOffset;

        addedItems_.clear();
        for (auto& reaction : addedReactions_)
            addedItems_.emplace_back(createReactionItem(reaction, reactions_.myReaction_, reactions_.msgId_));

        const auto addedSize = updateItemsGeometry(addedItems_, sizeAfterDeletion, UpdateMode::Add);

        isGeometryUpdated &= items_.size() <= maxReactionsCount;
        if (isGeometryUpdated)
        {
            isGeometryUpdated &= std::none_of(currentOffsets.cbegin(), currentOffsets.cend(), [](const auto& off) { return off == 0; });
            isGeometryUpdated &= std::equal(currentOffsets.cbegin(), currentOffsets.cbegin() + getTopReactionsSize(), newOffsets.cbegin());
        }

        const auto newWidth = isGeometryUpdated ? sizeAfterDeletion + addedSize : currentWidth;

        if (items_.size() == 1 && items_[0]->rect_.left() > newOffsets[0])
            currentOffsets[0] = items_[0]->rect_.left();

        geometryAnimation_->stop();
        geometryAnimation_->disconnect(q);
        QObject::connect(geometryAnimation_, &QVariantAnimation::valueChanged, q, [this, currentOffsets, newOffsets, currentWidth, newWidth](const QVariant& value)
        {
            const auto val = value.toDouble();
            for (auto i = 0u; i < items_.size(); i++)
            {
                const auto itemNewX = currentOffsets[i] + (newOffsets[i] - currentOffsets[i]) * val;
                updateItemGeometry(items_[i], itemNewX);
            }

            const auto width = currentWidth + (newWidth - currentWidth) * val;
            setPlateGeometry(QRect(calcPlatePosition(width), QSize(width, MessageStyle::Plates::plateHeightWithShadow(item_->reactionsPlateType()))));

            q->update();
        });
        QObject::connect(geometryAnimation_, &QVariantAnimation::finished, q, [this]()
        {
            startAddedItemsAnimation();
        });
        geometryAnimation_->start();
    }

    void startAddedItemsAnimation()
    {
        for (auto& item : addedItems_)
            item->opacity_ = 0;

        updateCounter(counterAboutToAddExisting_);
        clearCounters();

        auto addedIndex = items_.size();
        std::move(addedItems_.begin(), addedItems_.end(), std::back_inserter(items_));
        addedItems_.resize(0);

        opacityAnimation_->stop();
        opacityAnimation_->disconnect(q);
        opacityAnimation_->setStartValue(0.0);
        opacityAnimation_->setEndValue(1.0);
        opacityAnimation_->setEasingCurve(QEasingCurve::InOutSine);
        opacityAnimation_->setDuration(updateAnimationDuration.count());

        QObject::connect(opacityAnimation_, &QVariantAnimation::valueChanged, q, [this, addedIndex](const QVariant& value)
        {
            const auto opacity = value.toDouble();
            for (auto i = addedIndex; i < items_.size(); ++i)
                items_[i]->opacity_ = opacity;
            q->update();
        });
        QObject::connect(opacityAnimation_, &QVariantAnimation::finished, q, [this]()
        {
            updateItems(reactions_);
            setPlateGeometry(updateGeometryHelper());

            updateInProgress_ = false;
            startQueuedAnimation();
        });

        opacityAnimation_->start();
    }

    void startQueuedAnimation()
    {
        if (queuedData_)
        {
            reactions_ = *queuedData_;
            processUpdate(reactions_);

            if (addedReactions_.empty() && deletedReactions_.empty())
            {
                updateItems(reactions_);
                setPlateGeometry(updateGeometryHelper());
            }
            else
            {
                startUpdateAnimation();
            }

            queuedData_.reset();
        }
    }

    void processUpdate(const Data::Reactions& _reactions)
    {
        deletedReactions_.clear();
        addedReactions_.clear();
        clearCounters();

        reactions_ = _reactions;

        if (items_.empty())
            return;

        std::unordered_set<QString, Utils::QStringHasher> reactionsSet;

        for (auto& reaction : _reactions.reactions_)
        {
            if (reaction.count_ == 0)
                continue;

            auto it = std::find_if(items_.begin(), items_.end(), [&reaction](const auto& _item) { return _item->reaction_ == reaction.reaction_; });
            if (it == items_.end())
            {
                addedReactions_.push_back(reaction);
                counterAboutToAddExisting_ += reaction.count_;
            }
            else if (reaction.count_ > (*it)->count_)
            {
                counterAboutToAddExisting_ += reaction.count_ - (*it)->count_;
            }
            else
            {
                counterAboutToDeleteExisting_ += reaction.count_ - (*it)->count_;
            }

            reactionsSet.insert(reaction.reaction_);
        }

        for (const auto& item : items_)
        {
            if (reactionsSet.count(item->reaction_) == 0)
            {
                deletedReactions_.insert(item->reaction_);
                counterAboutToDeleteExisting_ -= item->count_;
            }
        }

        if (counterAboutToAddExisting_ != 0 && counterAboutToDeleteExisting_ != 0)
        {
            if (counterAboutToAddExisting_ + counterAboutToDeleteExisting_ == 0)
            {
                counterAboutToDeleteExisting_ = 0;
                if (deletedReactions_.empty())
                    counterAboutToAddExisting_ = 0;
            }
        }
    }

    void showTooltip(const ReactionItem& _item)
    {
        QString tooltipText;
        if (allowedToViewReactionsList(item_->getContact()))
        {
            tooltipText += _item.reaction_ % QChar::LineFeed;

            auto pageIt = pageCache_.find(_item.reaction_);
            if (pageIt != pageCache_.end())
            {
                const auto& page = pageIt->second;
                const auto itemsToShowCount = _item.count_ > maxTooltipReactionsCount ? maxTooltipReactionsCount - 1 : maxTooltipReactionsCount; // -1 for "And %1 more" line

                for (auto i = 0u; i < page.reactions_.size() && i < itemsToShowCount; i++)
                    tooltipText += Logic::GetFriendlyContainer()->getFriendly(page.reactions_[i].contact_) % QChar::LineFeed;

                if (_item.count_ > itemsToShowCount)
                    tooltipText += QT_TRANSLATE_NOOP("reactions", "And %1 more...").arg(_item.count_ - itemsToShowCount);
            }
            else
            {
                tooltipText += QT_TRANSLATE_NOOP("reactions", "Loading...");
            }

            if (pageIt == pageCache_.end() || outdatedPages_.count(_item.reaction_) != 0)
            {
                auto it = std::find_if(pageSeqs_.begin(), pageSeqs_.end(), [&_item](const PageRequest& _pageRequest) { return _pageRequest.reaction_ == _item.reaction_; });
                if (it == pageSeqs_.end())
                {
                    PageRequest request;
                    request.reaction_ = _item.reaction_;
                    request.seq_ = GetDispatcher()->getReactionsPage(item_->getId(), item_->getContact(), _item.reaction_, {}, u"0", maxTooltipReactionsCount);
                    pageSeqs_.push_back(std::move(request));
                }
            }
        }
        else
        {
            tooltipText += _item.reaction_ % QChar::Space % QString::number(_item.count_);
        }

        Tooltip::show(tooltipText, QRect(q->mapToGlobal(_item.rect_.topLeft()), _item.rect_.size()), {-1, -1});
    }

    void initShadow()
    {
        shadow_ = Utils::initPlateShadowEffect(q, opacity_);
        q->setGraphicsEffect(shadow_);
    }

    QRect getPlateRect() const
    {
        return q->rect();
    }

    QPoint pressPos_;
    Data::Reactions reactions_;
    HistoryControlPageItem* item_;
    ReactionItems items_;
    ReactionItems addedItems_;
    QGraphicsDropShadowEffect* shadow_;
    std::unordered_map<QString, Data::ReactionsPage, Utils::QStringHasher> pageCache_;
    std::unordered_set<QString, Utils::QStringHasher> outdatedPages_;
    std::unordered_set<QString, Utils::QStringHasher> deletedReactions_;
    std::vector<Data::Reactions::Reaction> addedReactions_;
    std::optional<Data::Reactions> queuedData_;
    QVariantAnimation* opacityAnimation_;
    QVariantAnimation* geometryAnimation_;
    std::vector<PageRequest> pageSeqs_;
    bool updateInProgress_ = false;
    double opacity_ = 0;
    bool outgoingPosition_;
    ReactionsPlate* q;
    TextRendering::TextUnitPtr countUnit_;
    int counterAboutToAddExisting_ = 0;
    int counterAboutToDeleteExisting_ = 0;
};

//////////////////////////////////////////////////////////////////////////
// ReactionsPlate
//////////////////////////////////////////////////////////////////////////

ReactionsPlate::ReactionsPlate(HistoryControlPageItem* _item)
    : QWidget(_item),
      d(std::make_unique<ReactionsPlate_p>(this))
{
    Testing::setAccessibleName(this, u"AS HistoryPage messageReactions " % QString::number(_item->getId()));

    d->item_ = _item;
    setVisible(false);
    setMouseTracking(true);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &ReactionsPlate::onMultiselectChanged);
    connect(GetDispatcher(), &core_dispatcher::reactionsListResult, this, &ReactionsPlate::onReactionsPage);

    d->initShadow();
}

ReactionsPlate::~ReactionsPlate() = default;

void ReactionsPlate::setReactions(const Data::Reactions& _reactions)
{
    if (!_reactions.isEmpty())
    {
        d->processUpdate(_reactions);

        if (d->addedReactions_.empty() && d->deletedReactions_.empty() && !d->updateInProgress_)
        {
            d->updateItems(_reactions);

            d->setPlateGeometry(d->updateGeometryHelper());

            if (!isVisible() && !Utils::InterConnector::instance().isMultiselect(d->item_->getContact()))
                d->startShowAnimation();
        }
        else
        {
            if (d->updateInProgress_)
                d->queuedData_ = _reactions;
            else
                d->startUpdateAnimation();
        }
    }
    else
    {
        d->items_.clear();
        hide();
    }

    for (const auto& [reaction, page] : d->pageCache_)
        d->outdatedPages_.insert(reaction);

    update();
}

void ReactionsPlate::setOutgoingPosition(bool _outgoingPosition)
{
    d->outgoingPosition_ = _outgoingPosition;
}

void ReactionsPlate::onMessageSizeChanged()
{
    d->setPlateGeometry(d->updateGeometryHelper());
}

void ReactionsPlate::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    d->draw(p);

    QWidget::paintEvent(_event);
}

void ReactionsPlate::showEvent(QShowEvent* _event)
{
    raise();
    QWidget::showEvent(_event);
}

void ReactionsPlate::mouseMoveEvent(QMouseEvent* _event)
{
    setCursor(allowedToViewReactionsList(d->item_->getContact()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    for (size_t i = 0; i < std::min(d->items_.size(), maxReactionsCount); ++i)
    {
        const auto item = d->items_.at(i);
        if (item->rect_.contains(_event->pos()))
        {
            d->showTooltip(*item);
            break;
        }
    }

    QWidget::mouseMoveEvent(_event);
}

void ReactionsPlate::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
        d->pressPos_ = _event->pos();

    _event->accept();
}

void ReactionsPlate::mouseReleaseEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton && allowedToViewReactionsList(d->item_->getContact()))
    {
        Tooltip::hide();
        if (const auto plateRect = d->getPlateRect();
            plateRect.contains(_event->pos()) && plateRect.contains(d->pressPos_))
        {
            auto item = std::find_if(d->items_.cbegin(), d->items_.cend(),
                                     [pos = _event->pos(), pressPos = d->pressPos_](auto it)
                                     {
                                        return it->rect_.contains(pos) && it->rect_.contains(pressPos);
                                     });
            if (item == d->items_.cend())
                Q_EMIT reactionClicked(QString());
            else
                Q_EMIT reactionClicked((*item)->reaction_);

        }
    }

    d->pressPos_ = QPoint();
    _event->accept();
}

void ReactionsPlate::leaveEvent(QEvent* _event)
{

}

void ReactionsPlate::wheelEvent(QWheelEvent* _event)
{
    if constexpr (platform::is_apple())
        Tooltip::hide();

    QWidget::wheelEvent(_event);
}

void ReactionsPlate::onMultiselectChanged()
{
    if (!d->item_->isMultiselectEnabled())
        return;

    if (Utils::InterConnector::instance().isMultiselect(d->item_->getContact()))
        hide();
    else if (d->getTopReactionsSize() > 0)
        show();
}

void ReactionsPlate::onReactionsPage(int64_t _seq, const Data::ReactionsPage& _page, bool _success)
{
    auto it = std::find_if(d->pageSeqs_.begin(), d->pageSeqs_.end(), [_seq](const ReactionsPlate_p::PageRequest& _pageRequest) { return _pageRequest.seq_ == _seq; });

    if (it == d->pageSeqs_.end())
        return;

    const auto& reaction = it->reaction_;

    if (_success)
    {
        d->pageCache_[reaction] = _page;
        d->outdatedPages_.erase(reaction);

        if (isVisible())
        {
            const auto mousePos = mapFromGlobal(QCursor::pos());
            for (auto& item : d->items_)
            {
                if (item->reaction_ == reaction && item->rect_.contains(mousePos))
                {
                    d->showTooltip(*item);
                    break;
                }
            }
        }
    }

    d->pageSeqs_.erase(it);
}

int AccessibleReactionsPlate::childCount() const
{
    return plate_ ? plate_->d->getTopReactionsSize() : 0;
}

QAccessibleInterface* AccessibleReactionsPlate::child(int _index) const
{
    if (!plate_ || _index < 0 || _index >= childCount())
        return nullptr;

    return QAccessible::queryAccessibleInterface(plate_->d->items_[_index]);
}

int AccessibleReactionsPlate::indexOfChild(const QAccessibleInterface* _child) const
{
    if (!plate_ || !_child)
        return -1;

    const auto reaction = qobject_cast<ReactionItem*>(_child->object());
    return Utils::indexOf(plate_->d->items_.cbegin(), plate_->d->items_.cend(), reaction);
}

QString AccessibleReactionsPlate::text(QAccessible::Text _type) const
{
    return plate_ && _type == QAccessible::Text::Name ? plate_->accessibleName() : QString();
}

}
