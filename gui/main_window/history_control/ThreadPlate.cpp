#include "stdafx.h"

#include "ThreadPlate.h"
#include "types/thread.h"
#include "HistoryControlPageItem.h"
#include "MessageStyle.h"
#include "utils/DrawUtils.h"
#include "styles/ThemeParameters.h"
#include "controls/TextUnit.h"
#include "cache/ColoredCache.h"
#include "fonts.h"

namespace
{
    int plateLeftPadding() noexcept
    {
        return Utils::scale_value(8);
    }

    int plateRightPadding() noexcept
    {
        return Utils::scale_value(6);
    }

    int iconSize() noexcept
    {
        return Utils::scale_value(16);
    }

    QFont getCounterFont()
    {
        return Fonts::adjustedAppFont(12, Fonts::FontWeight::Medium);
    }

    int threadIconToMentionIconMargin() noexcept
    {
        return Utils::scale_value(2);
    }

    struct MentionIcon
    {
        QPixmap icon_;

        void fill(const QColor& _color)
        {
            icon_ = Utils::renderSvg(qsl(":/history/mention"), { iconSize(), iconSize() }, _color);
        }
    };

    struct ThreadIcon
    {
        QPixmap icon_;

        void fill(const QColor& _color)
        {
            icon_ = Utils::renderSvg(qsl(":/thread_icon_filled"), { iconSize(), iconSize() }, _color);
        }
    };

    const QPixmap& getMentionIcon(QColor _color)
    {
        static Utils::ColoredCache<MentionIcon> cache;
        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            cache.clear();
        return cache[_color].icon_;
    }

    const QPixmap& getThreadIcon(QColor _color)
    {
        static Utils::ColoredCache<ThreadIcon> cache;
        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            cache.clear();
        return cache[_color].icon_;
    }
}

namespace Ui
{
    ThreadPlate::ThreadPlate(HistoryControlPageItem* _item)
        : ClickableWidget(_item)
        , item_(_item)
        , isOutgoing_(_item->isOutgoingPosition())
    {
        Testing::setAccessibleName(this, u"AS ThreadPlate " % QString::number(_item->getId()));

        setFixedHeight(MessageStyle::Plates::plateHeightWithShadow(_item->reactionsPlateType()));

        setGraphicsEffect(Utils::initPlateShadowEffect(this));
    }

    ThreadPlate::ThreadPlate(const QString& _caption, QWidget* _parent)
        : ClickableWidget(_parent)
    {
        setFixedHeight(MessageStyle::Plates::plateHeightWithShadow(ReactionsPlateType::Regular));
    }

    void ThreadPlate::updateWith(const std::shared_ptr<Data::ThreadUpdate>& _update)
    {
        isSubscriber_ = _update->isSubscriber_;
        repliesCount_ = std::max(0, _update->repliesCount_);
        hasMention_ = _update->unreadMentionsMe_ > 0;
        hasUnreads_ = _update->unreadMessages_ > 0;

        resize(sizeHint());
        updatePosition();
        update();
    }

    void ThreadPlate::updatePosition()
    {
        if (!item_)
            return;

        const auto messageRect = item_->messageRect();

        const auto top = messageRect.bottom() + MessageStyle::Plates::plateOffsetY(item_->reactionsPlateType());
        auto left = 0;

        if (isOutgoing_)
        {
            left = messageRect.right() - MessageStyle::Plates::plateOffsetX() - sizeHint().width();

            if (const auto reactRect = item_->reactionsPlateRect(); reactRect.width() > 0)
                left -= reactRect.width() + MessageStyle::Plates::plateOffsetX();
        }
        else
        {
            left = messageRect.left() + MessageStyle::Plates::plateOffsetX();
        }

        move(left, top);
    }

    QSize ThreadPlate::sizeHint() const
    {
        const auto mentionWidth = hasMention_ ? iconSize() : 0;
        const auto counterWidth = repliesCount_ > 0 ? repliesCounterWidth() : 0;

        return { plateLeftPadding() + iconSize() + mentionWidth + counterWidth + plateRightPadding(), height() };
    }

    ThreadPlate* ThreadPlate::plateForPopup(QWidget* _parent)
    {
        auto plate = new ThreadPlate(QT_TRANSLATE_NOOP("groupchat", "To replies"), _parent);
        plate->setEnabled(false);
        return plate;
    }

    void ThreadPlate::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        p.setPen(Qt::NoPen);

        QPainterPath platePath;
        const auto plateRect = QRect(0, 0, width(), MessageStyle::Plates::plateHeight());
        platePath.addRoundedRect(plateRect, MessageStyle::Plates::borderRadius(), MessageStyle::Plates::borderRadius());

        Utils::drawPlateSolidShadow(p, platePath);
        p.fillPath(platePath, getBackgroundColor());

        const auto iconColor = Styling::getParameters().getColor(getIconColorStyle());
        const auto y = (MessageStyle::Plates::plateHeight() - iconSize()) / 2;
        auto x = plateLeftPadding();

        p.drawPixmap(x, y, getThreadIcon(iconColor));
        x += iconSize();

        if (hasMention_)
        {
            x += threadIconToMentionIconMargin();
            p.drawPixmap(x, y, getMentionIcon(iconColor));
            x += iconSize();
            x -= threadIconToMentionIconMargin();
        }

        if (repliesCount_ > 0)
        {
            const QRectF textRect(x, y - Utils::fscale_value(1), repliesCounterWidth(), iconSize());
            p.setPen(Styling::getParameters().getColor(getTextColorStyle()));
            p.setFont(getCounterFont());
            Utils::drawText(p, textRect.center(), Qt::AlignCenter, repliesCounterString());
        }
    }

    QColor ThreadPlate::getBackgroundColor() const
    {
        if (hasUnreads_ && isSubscriber_)
            return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);

        if (isOutgoing_)
            return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_TERTIARY);

        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_LIGHT);
    }

    QColor ThreadPlate::getIconBgColor() const
    {
        if (hasUnreads_ && isSubscriber_)
            return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);

        if (isOutgoing_)
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE, 0.55);

        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    Styling::StyleVariable ThreadPlate::getIconColorStyle() const
    {
        if (hasUnreads_ && isSubscriber_)
            return Styling::StyleVariable::BASE_GLOBALWHITE;

        return Styling::StyleVariable::PRIMARY;
    }

    Styling::StyleVariable ThreadPlate::getTextColorStyle() const
    {
        if (hasUnreads_ && isSubscriber_)
            return Styling::StyleVariable::BASE_GLOBALWHITE;

        return isOutgoing_ ? Styling::StyleVariable::PRIMARY_PASTEL : Styling::StyleVariable::BASE_PRIMARY;
    }

    bool ThreadPlate::isIconShadowNeeded() const
    {
        return !isOutgoing_ && !hasUnreads_;
    }

    QString ThreadPlate::repliesCounterString() const
    {
        return Utils::getUnreadsBadgeStr(repliesCount_);
    }

    int ThreadPlate::repliesCounterWidth() const
    {
        return (repliesCounterString().size() + 1) * (iconSize() / 2);
    }
}
