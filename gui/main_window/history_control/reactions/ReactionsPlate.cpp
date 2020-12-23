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
    constexpr std::chrono::milliseconds updateAnimationDuration = std::chrono::milliseconds(100);
    constexpr auto maxTooltipReactionsCount = 13u;

    int32_t plateOffsetY(Ui::ReactionsPlateType _type)
    {
        if (_type == Ui::ReactionsPlateType::Regular)
            return Ui::MessageStyle::Reactions::plateYOffset();
        else
            return Ui::MessageStyle::Reactions::mediaPlateYOffset();
    }

    int32_t plateOffsetX()
    {
        return Utils::scale_value(4);
    }

    int32_t plateHeight()
    {
        return Ui::MessageStyle::Reactions::plateHeight();
    }

    int32_t plateHeightWithShadow(Ui::ReactionsPlateType _type)
    {
        auto height = plateHeight();
        if (_type == Ui::ReactionsPlateType::Regular)
            height += Ui::MessageStyle::Reactions::shadowHeight();

        return height;
    }

    int32_t plateBorderRadius()
    {
        return Utils::scale_value(12);
    }

    const QColor& plateColor(Ui::ReactionsPlateType _type, bool _outgoing)
    {
        if (_type == Ui::ReactionsPlateType::Regular)
        {
            if (_outgoing)
            {
                static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT);
                return color;
            }
            else
            {
                static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_LIGHT);
                return color;
            }
        }
        else
        {
            static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::GHOST_SECONDARY);
            return color;
        }
    }

    int32_t shadowOffset()
    {
        return Utils::scale_value(2);
    }

    int32_t plateEmojiSize()
    {
        return Utils::scale_value(19);
    }

    int32_t plateSideMargin()
    {
        return Utils::scale_value(4);
    }

    int32_t plateTopMargin()
    {
        return Utils::scale_value(3);
    }

    int32_t emojiRightMargin()
    {
        return Utils::scale_value(4);
    }

    int32_t textRightMargin()
    {
        return Utils::scale_value(6);
    }

    QFont countFont(bool _myReaction)
    {
        if (_myReaction)
            return Fonts::adjustedAppFont(12, Fonts::FontWeight::Bold);
        else
            return Fonts::adjustedAppFont(12, Fonts::FontWeight::Normal);
    }

    const QColor& countTextColor(Ui::ReactionsPlateType _type, bool _outgoing)
    {
        if (_type == Ui::ReactionsPlateType::Regular)
        {
            if (_outgoing)
            {
                static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL);
                return color;
            }
            else
            {
                static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
                return color;
            }
        }
        else
        {
            static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            return color;
        }
    }

    QColor shadowColor()
    {
        return QColor(0, 0, 0, 255);
    }

    double shadowColorAlpha()
    {
        return 0.12;
    }

    QString countText(int _count)
    {
        if (_count < 100)
            return QString::number(_count);
        else
            return qsl("99+");
    }

    bool allowedToViewReactionsList(const QString& _chatId)
    {
        const auto role = Logic::getContactListModel()->getYourRole(_chatId);
        const auto notAllowed = role == u"notamember" || Logic::getContactListModel()->isChannel(_chatId) && !Logic::getContactListModel()->isYouAdmin(_chatId);

        return !notAllowed;
    }

    int shadowHeight()
    {
        return Utils::scale_value(1);
    }

    void drawSolidShadow(QPainter& _p, const QPainterPath& _path)
    {
        Utils::ShadowLayer layer;
        layer.color_ = QColor(17, 32, 55, 255 * 0.05);
        layer.yOffset_ = shadowHeight();

        Utils::drawLayeredShadow(_p, _path, { layer });
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
    }

    struct PageRequest
    {
        QString reaction_;
        int64_t seq_ = 0;
    };

    ReactionItem* createReactionItem(const Data::Reactions::Reaction& _reaction, const QString& _myReaction, int64_t _msgId)
    {
        auto item = new ReactionItem(q);
        item->reaction_ = _reaction.reaction_;
        item->countUnit_ = TextRendering::MakeTextUnit(countText(_reaction.count_));
        item->countUnit_->init(countFont(_reaction.reaction_ == _myReaction), countTextColor(item_->reactionsPlateType(), outgoingPosition_));
        item->emoji_ = Emoji::GetEmoji(Emoji::EmojiCode::fromQString(_reaction.reaction_), Utils::scale_bitmap(plateEmojiSize()));
        item->count_ = _reaction.count_;
        item->msgId_ = _msgId;

        return item;
    }

    void updateReactionItem(ReactionItem* _item, const Data::Reactions::Reaction& _reaction, const QString& _myReaction)
    {
        _item->count_ = _reaction.count_;
        _item->countUnit_->setText(countText(_reaction.count_));
        _item->countUnit_->init(countFont(_reaction.reaction_ == _myReaction), countTextColor(item_->reactionsPlateType(), outgoingPosition_));
    }

    void updateItems(const Data::Reactions& _reactions)
    {
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
    }

    int updateItemGeometry(ReactionItem* _item, int _xOffset)
    {
        auto startXOffset = _xOffset;
        _item->emojiRect_ = QRect(_xOffset, plateTopMargin(), plateEmojiSize(), plateEmojiSize());
        const auto countWidth = _item->countUnit_->desiredWidth();
        _xOffset += _item->emojiRect_.width() + emojiRightMargin();
        _item->countUnit_->getHeight(countWidth);
        _item->countUnit_->setOffsets(_xOffset, plateHeight() / 2);
        _item->rect_ = QRect(_item->emojiRect_.topLeft(), QSize(plateEmojiSize() + emojiRightMargin() + countWidth, plateHeight()));
        _xOffset += textRightMargin() + countWidth;

        return _xOffset - startXOffset;
    }

    std::vector<int> calcItemsOffsets(ReactionItems& _items, int& _xOffset)
    {
        std::vector<int> offsets_;
        for (const auto& item : _items)
        {
            offsets_.push_back(_xOffset);
            _xOffset += item->emojiRect_.width() + emojiRightMargin();
            const auto countWidth = item->countUnit_->desiredWidth();
            _xOffset += textRightMargin() + countWidth;
        }

        return offsets_;
    }

    int updateItemsGeometry(ReactionItems& _items, int _xOffset = 0)
    {
        auto startXOffset = _xOffset;
        for (auto& item: _items)
            _xOffset += updateItemGeometry(item, _xOffset);

        return _xOffset - startXOffset;
    }

    QPoint calcPlatePosition(int _plateWidth)
    {
        const auto messageRect = item_->messageRect();

        const auto top = messageRect.bottom() + plateOffsetY(item_->reactionsPlateType());
        auto left = 0;

        if (outgoingPosition_)
            left = messageRect.right() - plateOffsetX() - _plateWidth;
        else
            left = messageRect.left() + plateOffsetX();

        return QPoint(left, top);
    }

    QRect updateGeometryHelper()
    {
        const auto plateType = item_->reactionsPlateType();
        shadow_->setEnabled(plateType == ReactionsPlateType::Regular);

        auto xOffset = plateSideMargin();
        xOffset += updateItemsGeometry(items_, xOffset);

        const auto plateSize = QSize(xOffset, plateHeightWithShadow(plateType));

        return QRect(calcPlatePosition(plateSize.width()), plateSize);
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
        platePath.addRoundedRect(plateRect, plateBorderRadius(), plateBorderRadius());

        drawSolidShadow(_p, platePath);

        _p.setOpacity(opacity_);
        _p.fillPath(platePath, plateColor(item_->reactionsPlateType(), outgoingPosition_));

        for (auto& item : items_)
        {
            _p.setOpacity(item->opacity_ * opacity_);
            _p.drawImage(item->emojiRect_, item->emoji_);
            item->countUnit_->draw(_p, TextRendering::VerPosition::MIDDLE);
        }
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
            shadow_->setColor(shadowColorWithAlpha());
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
        opacityAnimation_->setEasingCurve(QEasingCurve::InOutSine);
        opacityAnimation_->setDuration(updateAnimationDuration.count());

        QObject::connect(opacityAnimation_, &QVariantAnimation::valueChanged, q, [this](const QVariant& value) {
            if (deletedReactions_.empty())
                return;

            const auto opacity = value.toDouble();
            for (auto& item : items_)
            {
                if (deletedReactions_.count(item->reaction_))
                    item->opacity_ = opacity;
            }
            q->update();
        });
        QObject::connect(opacityAnimation_, &QVariantAnimation::finished, q, [this]() {
            startGeometryAnimation();
        });

        opacityAnimation_->start();
    }

    void startGeometryAnimation()
    {
        auto currentWidth = q->size().width();

        std::vector<int> currentOffsets;

        for (auto& item : items_)
        {
            if (deletedReactions_.count(item->reaction_) == 0)
                currentOffsets.push_back(item->emojiRect_.left());
        }

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

        const auto addedSize = updateItemsGeometry(addedItems_, sizeAfterDeletion);
        const auto newWidth = sizeAfterDeletion + addedSize;

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
            setPlateGeometry(QRect(calcPlatePosition(width), QSize(width, plateHeightWithShadow(item_->reactionsPlateType()))));

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
                addedReactions_.push_back(reaction);

            reactionsSet.insert(reaction.reaction_);
        }

        for (const auto& item : items_)
        {
            if (reactionsSet.count(item->reaction_) == 0)
                deletedReactions_.insert(item->reaction_);
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

    QColor shadowColorWithAlpha()
    {
        auto color = shadowColor();
        color.setAlphaF(shadowColorAlpha() * opacity_);
        return color;
    }

    void initShadow()
    {
        shadow_ = new QGraphicsDropShadowEffect(q);
        shadow_->setBlurRadius(8);
        shadow_->setOffset(0, Utils::scale_value(1));
        shadow_->setColor(shadowColorWithAlpha());

        q->setGraphicsEffect(shadow_);
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
};

//////////////////////////////////////////////////////////////////////////
// ReactionsPlate
//////////////////////////////////////////////////////////////////////////

ReactionsPlate:: ReactionsPlate(HistoryControlPageItem* _item)
    : QWidget(_item),
      d(std::make_unique<ReactionsPlate_p>(this))
{
    Testing::setAccessibleName(this, u"AS HistoryPage messageReactions " % QString::number(_item->getId()));

    d->item_ = _item;
    setVisible(false);
    setMouseTracking(true);
    setCursor(allowedToViewReactionsList(_item->getContact()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &ReactionsPlate::onMultiselectChanged);
    connect(GetDispatcher(), &core_dispatcher::reactionsListResult, this, &ReactionsPlate::onReactionsPage);

    d->initShadow();
}

ReactionsPlate::~ReactionsPlate()
{

}

void ReactionsPlate::setReactions(const Data::Reactions& _reactions)
{
    if (!_reactions.isEmpty())
    {
        d->processUpdate(_reactions);

        if (d->addedReactions_.empty() && d->deletedReactions_.empty() && !d->updateInProgress_)
        {
            d->updateItems(_reactions);

            d->setPlateGeometry(d->updateGeometryHelper());

            if (!isVisible() && !Utils::InterConnector::instance().isMultiselect())
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
    for (auto& item : d->items_)
    {
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

        for (auto& item : d->items_)
        {
            if (item->rect_.contains(_event->pos()) && item->rect_.contains(d->pressPos_))
            {
                Q_EMIT reactionClicked(item->reaction_);
                break;
            }
        }
    }

    d->pressPos_ = QPoint();
    _event->accept();
}

void ReactionsPlate::leaveEvent(QEvent* _event)
{

}

void ReactionsPlate::onMultiselectChanged()
{
    if (Utils::InterConnector::instance().isMultiselect())
        hide();
    else if (d->items_.size() > 0)
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
    return plate_ ? plate_->d->items_.size() : 0;
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
