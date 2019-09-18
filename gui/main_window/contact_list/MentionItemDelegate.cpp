#include "stdafx.h"

#include "MentionItemDelegate.h"
#include "SearchHighlight.h"

#include "../../cache/avatars/AvatarStorage.h"
#include "../../styles/ThemeParameters.h"
#include "../../app_config.h"
#include "../friendly/FriendlyContainer.h"

#include "ContactListModel.h"

namespace
{
    int verticalEdgeMargin()
    {
        return Utils::scale_value(4);
    }

    int itemHorPadding()
    {
        return Utils::scale_value(12);
    }

    int nickPadding()
    {
        return Utils::scale_value(4);
    }

    int avatarSize()
    {
        return Utils::scale_value(20);
    }

    int textLeftPadding()
    {
        return (itemHorPadding() * 2) + avatarSize();
    }

    constexpr std::chrono::milliseconds dropCacheTimeout = std::chrono::minutes(3);

    QFont getFont()
    {
        return Fonts::appFontScaled(14, Fonts::FontWeight::Normal);
    }
}

namespace Logic
{
    MentionItemDelegate::MentionItemDelegate(QObject* parent)
        : QItemDelegate(parent)
        , lastDrawTime_(QTime::currentTime())
        , cacheTimer_(new QTimer(this))
    {
        cacheTimer_->setInterval(dropCacheTimeout.count());
        cacheTimer_->setTimerType(Qt::VeryCoarseTimer);
        connect(cacheTimer_, &QTimer::timeout, this, [this]()
        {
            if (lastDrawTime_.msecsTo(QTime::currentTime()) >= cacheTimer_->interval())
                dropCache();
        });
        cacheTimer_->start();

        connect(GetFriendlyContainer(), &FriendlyContainer::friendlyChanged, this, [this](const QString& _aimid, const QString&)
        {
            contactCache_[_aimid].reset();
        });
    }

    void MentionItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        Utils::PainterSaver ps(*_painter);

        const auto item = _index.data(Qt::DisplayRole).value<MentionSuggest>();
        const auto isSelected = (_option.state & QStyle::State_Selected);
        const auto inHoverColor = !item.isServiceItem() && isSelected;

        const auto topAdjust = isFirstItem(_index) ? verticalEdgeMargin() : 0;
        const auto botAdjust = isLastItem(_index) ? -verticalEdgeMargin() : 0;
        const auto itemRect = _option.rect.adjusted(0, topAdjust, 0, botAdjust);

        if (inHoverColor)
            Ui::RenderMouseState(*_painter, inHoverColor, false, itemRect);

        _painter->translate(itemRect.topLeft());

        if (item.isServiceItem())
            renderServiceItem(*_painter, itemRect, item);
        else
            renderContactItem(*_painter, itemRect, item);

        lastDrawTime_ = QTime::currentTime();

        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            if (!item.isServiceItem())
            {
                // FONT
                _painter->setPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
                _painter->setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));

                const auto x = itemRect.width();
                const auto y = itemRect.height();
                Utils::drawText(*_painter, QPointF(x, y), Qt::AlignRight | Qt::AlignBottom, item.aimId_);
            }
        }
    }

    void MentionItemDelegate::renderServiceItem(QPainter& _painter, const QRect& _itemRect, const MentionSuggest & _item) const
    {
        Utils::PainterSaver ps(_painter);

        _painter.setPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));

        _painter.setFont(Fonts::appFontScaled(12, Fonts::FontWeight::SemiBold));

        const auto x = itemHorPadding();
        const auto y = _itemRect.height() / 2;
        Utils::drawText(_painter, QPointF(x, y), Qt::AlignVCenter, _item.friendlyName_);
    }

    void MentionItemDelegate::renderContactItem(QPainter& _painter, const QRect& _itemRect, const MentionSuggest& _item) const
    {
        bool isDef = false;
        auto avatar = Logic::GetAvatarStorage()->GetRounded(
            _item.aimId_,
            _item.friendlyName_,
            Utils::scale_bitmap(avatarSize()),
            QString(),
            isDef,
            false,
            false
        );

        if (!avatar->isNull())
        {
            const QPoint pos(itemHorPadding(), (itemHeight() - avatarSize()) / 2);
            _painter.drawPixmap(pos, *avatar);
        }

        if (_item.friendlyName_.isEmpty())
            return;

        auto& ci = contactCache_[_item.aimId_];
        bool needUpdate = false;

        const auto maxTextWidth = _itemRect.width() - textLeftPadding() - itemHorPadding();
        if (ci)
            needUpdate = ci->cachedWidth_ != maxTextWidth || ci->highlight_ != _item.highlight_;

        if (!ci || needUpdate)
        {
            ci = std::make_unique<CachedItem>();
            ci->cachedWidth_ = maxTextWidth;
            ci->highlight_ = _item.highlight_;

            const Ui::highlightsV hlVec = { _item.highlight_ };

            int textWidth = 0;
            int nickWidth = 0;
            if (!_item.highlight_.isEmpty() && !_item.nick_.isEmpty() && Ui::findNextHighlight(_item.nick_, hlVec).indexStart_ != -1)
            {
                ci->itemNick_ = Ui::createHightlightedText(
                    Utils::makeNick(_item.nick_),
                    getFont(),
                    Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                    Ui::getTextHighlightedColor(),
                    1,
                    hlVec);

                ci->itemNick_->evaluateDesiredSize();

                nickWidth = ci->itemNick_->getLastLineWidth() + nickPadding();
            }

            ci->itemText_ = Ui::createHightlightedText(
                _item.friendlyName_,
                getFont(),
                Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
                Ui::getTextHighlightedColor(),
                1,
                hlVec);

            ci->itemText_->evaluateDesiredSize();
            ci->itemText_->setOffsets(textLeftPadding(), itemHeight() / 2);

            textWidth = ci->itemText_->getLastLineWidth();
            if (textWidth + nickWidth > maxTextWidth)
            {
                if (ci->itemNick_)
                {
                    if (nickWidth <= maxTextWidth / 2)
                    {
                        ci->itemText_->getHeight(maxTextWidth - nickWidth);
                        textWidth = ci->itemText_->getLastLineWidth();
                    }
                    else
                    {
                        ci->itemText_->getHeight(maxTextWidth / 2);
                        textWidth = ci->itemText_->getLastLineWidth();

                        ci->itemNick_->getHeight(maxTextWidth - textWidth - nickPadding());
                    }

                    ci->itemNick_->setOffsets(textLeftPadding() + textWidth + nickPadding(), itemHeight() / 2);
                }
                else
                {
                    ci->itemText_->getHeight(maxTextWidth);
                }
            }
            else if (ci->itemNick_)
            {
                ci->itemNick_->setOffsets(textLeftPadding() + textWidth + nickPadding(), itemHeight() / 2);
            }
        }

        ci->itemText_->draw(_painter, Ui::TextRendering::VerPosition::MIDDLE);

        if (ci->itemNick_)
            ci->itemNick_->draw(_painter, Ui::TextRendering::VerPosition::MIDDLE);
    }

    bool MentionItemDelegate::isFirstItem(const QModelIndex& _index) const
    {
        return _index.isValid() && _index.row() == 0;
    }

    bool MentionItemDelegate::isLastItem(const QModelIndex& _index) const
    {
        return _index.isValid() && _index.row() == _index.model()->rowCount() - 1;
    }

    QSize MentionItemDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        auto height = itemHeight();

        const auto model = qobject_cast<const MentionModel*>(_index.model());
        if (model && model->isServiceItem(_index))
            height = serviceItemHeight();

        if (isFirstItem(_index))
            height += verticalEdgeMargin();

        if (isLastItem(_index))
            height += verticalEdgeMargin();

        return QSize(_option.rect.width(), height);
    }

    int MentionItemDelegate::itemHeight() const
    {
        return Utils::scale_value(32);
    }

    int MentionItemDelegate::serviceItemHeight() const
    {
        return Utils::scale_value(20);
    }

    void MentionItemDelegate::dropCache()
    {
        contactCache_.clear();
    }
}
