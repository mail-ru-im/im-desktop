#include "stdafx.h"

#include "controls/TextUnit.h"
#include "controls/GeneralDialog.h"
#include "styles/ThemeParameters.h"
#include "main_window/MainWindow.h"
#include "main_window/contact_list/ContactListModel.h"
#include "../HistoryControlPageItem.h"
#include "utils/InterConnector.h"
#include "animation/animation.h"
#include "../MessageStyle.h"
#include "utils/DrawUtils.h"
#include "utils/utils.h"
#include "fonts.h"

#include "ReactionsPlate.h"
#include "ReactionsList.h"

namespace
{
    constexpr std::chrono::milliseconds animationDuration = std::chrono::milliseconds(150);
    constexpr std::chrono::milliseconds updateAnimationDuration = std::chrono::milliseconds(100);

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
        const auto notAllowed = role == ql1s("notamember") || Logic::getContactListModel()->isChannel(_chatId);

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
    ReactionsPlate_p(ReactionsPlate* _q) : q(_q) {}

    struct ReactionItem
    {
        QRect rect_;
        QRect emojiRect_;
        QImage emoji_;
        bool mouseOver_ = false;
        QString reaction_;
        double opacity_ = 1;
        TextRendering::TextUnitPtr countUnit_;
        anim::Animation opacityAnimation_;
        anim::Animation geometryAnimation_;
    };

    ReactionItem createReactionItem(const Data::Reactions::Reaction& _reaction, const QString& _myReaction)
    {
        ReactionItem item;
        item.reaction_ = _reaction.reaction_;
        item.countUnit_ = TextRendering::MakeTextUnit(countText(_reaction.count_));
        item.countUnit_->init(countFont(_reaction.reaction_ == _myReaction), countTextColor(item_->reactionsPlateType(), outgoingPosition_));
        item.emoji_ = Emoji::GetEmoji(Emoji::EmojiCode::fromQString(_reaction.reaction_), Utils::scale_bitmap(plateEmojiSize()));

        return item;
    }

    void updateReactionItem(ReactionItem& _item, const Data::Reactions::Reaction& _reaction, const QString& _myReaction)
    {
        _item.countUnit_->setText(countText(_reaction.count_));
        _item.countUnit_->init(countFont(_reaction.reaction_ == _myReaction), countTextColor(item_->reactionsPlateType(), outgoingPosition_));
    }

    void updateItems(const Data::Reactions& _reactions)
    {
        for (auto& reaction : _reactions.reactions_)
        {
            if (reaction.count_ == 0)
                continue;

            auto it = std::find_if(items_.begin(), items_.end(), [reaction](const ReactionItem& _item) { return _item.reaction_ == reaction.reaction_; });
            if (it != items_.end())
                updateReactionItem(*it, reaction, _reactions.myReaction_);
            else
                items_.push_back(createReactionItem(reaction, _reactions.myReaction_));
        }
    }

    int updateItemGeometry(ReactionItem& _item, int _xOffset)
    {
        auto startXOffset = _xOffset;
        _item.emojiRect_ = QRect(_xOffset, plateTopMargin(), plateEmojiSize(), plateEmojiSize());
        const auto countWidth = _item.countUnit_->desiredWidth();
        _xOffset += _item.emojiRect_.width() + emojiRightMargin();
        _item.countUnit_->getHeight(countWidth);
        _item.countUnit_->setOffsets(_xOffset, plateHeight() / 2);
        _item.rect_ = QRect(_item.emojiRect_.topLeft(), QSize(plateEmojiSize() + emojiRightMargin() + countWidth, plateHeight()));
        _xOffset += textRightMargin() + countWidth;

        return _xOffset - startXOffset;
    }    

    std::vector<int> calcItemsOffsets(std::vector<ReactionItem>& _items, int& _xOffset)
    {
        std::vector<int> offsets_;
        for (const auto& item : _items)
        {
            offsets_.push_back(_xOffset);
            _xOffset += item.emojiRect_.width() + emojiRightMargin();
            const auto countWidth = item.countUnit_->desiredWidth();
            _xOffset += textRightMargin() + countWidth;
        }

        return offsets_;
    }

    int updateItemsGeometry(std::vector<ReactionItem>& _items, int _xOffset = 0)
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
            _p.setOpacity(item.opacity_ * opacity_);
            _p.drawImage(item.emojiRect_, item.emoji_);
            item.countUnit_->draw(_p, TextRendering::VerPosition::MIDDLE);
        }
    }

    void startShowAnimation()
    {
        updateInProgress_ = true;
        opacity_ = 0;

        opacityAnimation_.finish();
        opacityAnimation_.start([this]()
        {
            opacity_ = opacityAnimation_.current();
            shadow_->setColor(shadowColorWithAlpha());
            q->update();
        },[this]()
        {
            updateInProgress_ = false;
            startQueuedAnimation();
        },0, 1, animationDuration.count(), anim::sineInOut);

        q->show();
    }

    void startUpdateAnimation()
    {
        updateInProgress_ = true;

        opacityAnimation_.finish();
        opacityAnimation_.start([this]()
        {
            if (deletedReactions_.empty())
                return;

            for (auto& item : items_)
            {
                if (deletedReactions_.count(item.reaction_))
                    item.opacity_ = opacityAnimation_.current();
            }
            q->update();
        },[this]()
        {
            startGeometryAnimation();
        }, 1, 0, updateAnimationDuration.count(), anim::sineInOut);
    }

    void startGeometryAnimation()
    {
        auto currentWidth = q->size().width();

        std::vector<int> currentOffsets;

        for (auto& item : items_)
        {
            if (deletedReactions_.count(item.reaction_) == 0)
                currentOffsets.push_back(item.emojiRect_.left());
        }

        for (auto& reaction : deletedReactions_)
        {
            auto it = std::find_if(items_.begin(), items_.end(), [reaction](const ReactionItem& _item) { return _item.reaction_ == reaction; });
            if (it != items_.end())
                items_.erase(it);
        }

        auto xOffset = plateSideMargin();
        auto newOffsets = calcItemsOffsets(items_, xOffset);

        auto sizeAfterDeletion = xOffset;

        addedItems_.clear();

        for (auto& reaction : addedReactions_)
            addedItems_.push_back(createReactionItem(reaction, reactions_.myReaction_));

        auto addedSize = updateItemsGeometry(addedItems_, sizeAfterDeletion);

        auto newWidth = sizeAfterDeletion + addedSize;

        geometryAnimation_.finish();
        geometryAnimation_.start([this, currentOffsets, newOffsets, currentWidth, newWidth]()
        {
            for (auto i = 0u; i < items_.size(); i++)
            {
                auto itemNewX = currentOffsets[i] + (newOffsets[i] - currentOffsets[i]) * geometryAnimation_.current();
                updateItemGeometry(items_[i], itemNewX);
            }

            auto width = currentWidth + (newWidth - currentWidth) * geometryAnimation_.current();
            setPlateGeometry(QRect(calcPlatePosition(width), QSize(width, plateHeightWithShadow(item_->reactionsPlateType()))));

            q->update();
        }, [this]()
        {
            startAddedItemsAnimation();
        }, 0, 1, updateAnimationDuration.count(), anim::sineInOut);
    }

    void startAddedItemsAnimation()
    {
        for (auto& item : addedItems_)
            item.opacity_ = 0;

        auto addedIndex = items_.size();
        std::move(addedItems_.begin(), addedItems_.end(), std::back_inserter(items_));
        addedItems_.resize(0);

        opacityAnimation_.finish();
        opacityAnimation_.start([this, addedIndex]()
        {
            for (auto i = addedIndex; i < items_.size(); i++)
                items_[i].opacity_ = opacityAnimation_.current();
            q->update();
        }, [this]()
        {
            updateItems(reactions_);
            setPlateGeometry(updateGeometryHelper());

            updateInProgress_ = false;
            startQueuedAnimation();
        }, 0, 1, updateAnimationDuration.count(), anim::sineInOut);
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

            auto it = std::find_if(items_.begin(), items_.end(), [reaction](const ReactionItem& _item) { return _item.reaction_ == reaction.reaction_; });
            if (it == items_.end())
                addedReactions_.push_back(reaction);

            reactionsSet.insert(reaction.reaction_);
        }

        for (const auto& item : items_)
        {
            if (reactionsSet.count(item.reaction_) == 0)
                deletedReactions_.insert(item.reaction_);
        }
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
    std::vector<ReactionItem> items_;
    std::vector<ReactionItem> addedItems_;
    QGraphicsDropShadowEffect* shadow_;
    std::unordered_set<QString, Utils::QStringHasher> deletedReactions_;
    std::vector<Data::Reactions::Reaction> addedReactions_;
    std::optional<Data::Reactions> queuedData_;
    anim::Animation opacityAnimation_;
    anim::Animation geometryAnimation_;
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
    d->item_ = _item;
    setVisible(false);
    setMouseTracking(true);
    setCursor(allowedToViewReactionsList(_item->getContact()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &ReactionsPlate::onMultiselectChanged);

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
        for (auto& item : d->items_)
        {
            if (item.rect_.contains(_event->pos()) && item.rect_.contains(d->pressPos_))
            {
                Q_EMIT reactionClicked(item.reaction_);
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

}
