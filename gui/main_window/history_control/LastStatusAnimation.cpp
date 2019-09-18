#include "stdafx.h"

#include "LastStatusAnimation.h"

#include "../../utils/utils.h"
#include "../../cache/ColoredCache.h"
#include "MessageStyle.h"
#include "HistoryControlPageItem.h"

namespace
{
    constexpr std::chrono::milliseconds pendingTimeout = std::chrono::milliseconds(500);

    struct IconStatusSet
    {
        QPixmap pending_;
        QPixmap deliveredToServer_;
        QPixmap deliveredToClient_;
        QPixmap pendingRecents_;

        void fill(const QColor& _color)
        {
            const auto makeIcon = [&_color](const auto& _path)
            {
                return Utils::renderSvg(_path, Ui::MessageStyle::getLastStatusIconSize(), _color);
            };

            pending_ = makeIcon(qsl(":/delivery/sending"));
            deliveredToServer_ = makeIcon(qsl(":/delivery/sent"));
            deliveredToClient_ = makeIcon(qsl(":/delivery/delivered"));
            pendingRecents_ = pending_;
        }
    };

    const IconStatusSet& getStatusPixmaps(const QColor& _color)
    {
        static Utils::ColoredCache<IconStatusSet> sets;
        return sets[_color];
    }
}

namespace Ui
{
    LastStatusAnimation::LastStatusAnimation(HistoryControlPageItem* _parent)
        : QObject(_parent)
        , widget_(_parent)
        , isActive_(true)
        , isPlay_(false)
        , show_(false)
        , pendingTimer_(new QTimer(this))
        , lastStatus_(LastStatus::None)
    {
        assert(_parent);

        pendingTimer_->setSingleShot(true);
        pendingTimer_->setInterval(pendingTimeout.count());
        connect(pendingTimer_, &QTimer::timeout, this, &LastStatusAnimation::onPendingTimeout);
    }

    LastStatusAnimation::~LastStatusAnimation()
    {
    }

    void LastStatusAnimation::onPendingTimeout()
    {
        widget_->update();
    }

    QPixmap LastStatusAnimation::getPendingPixmap(const QColor& _color)
    {
        return getStatusPixmaps(_color).pendingRecents_;
    }

    QPixmap LastStatusAnimation::getDeliveredToClientPixmap(const QColor& _color)
    {
        return getStatusPixmaps(_color).deliveredToClient_;
    }

    QPixmap LastStatusAnimation::getDeliveredToServerPixmap(const QColor& _color)
    {
        return getStatusPixmaps(_color).deliveredToServer_;
    }

    void LastStatusAnimation::startPendingTimer()
    {
        if (isPendingTimerActive())
            return;

        pendingTimer_->start();
    }

    void LastStatusAnimation::stopPendingTimer()
    {
        pendingTimer_->stop();
    }

    bool LastStatusAnimation::isPendingTimerActive() const
    {
        return pendingTimer_->isActive();
    }

    void LastStatusAnimation::setLastStatus(const LastStatus _lastStatus)
    {
        if (lastStatus_ == _lastStatus)
            return;

        lastStatus_ = _lastStatus;

        if (lastStatus_ != LastStatus::Pending)
            stopPendingTimer();

        switch (lastStatus_)
        {
            case LastStatus::None:
                show_ = false;
                break;
            case LastStatus::Read:
                break;
            case LastStatus::Pending:
            {
                startPendingTimer();
                show_ = true;
                break;
            }
            case LastStatus::DeliveredToServer:
            case LastStatus::DeliveredToPeer:
            {
                show_ = false;
                break;
            }
            default:
                break;
        }

        widget_->update();
    }

    void LastStatusAnimation::hideStatus()
    {
        if (show_)
        {
            show_ = false;
            widget_->update();
        }
    }

    void LastStatusAnimation::showStatus()
    {
        if (!show_)
        {
            show_ = true;
            widget_->update();
        }
    }

    void LastStatusAnimation::drawLastStatus(
        QPainter& _p,
        const int _x,
        const int _y,
        const int _dx,
        const int _dy) const
    {
        if (!show_)
            return;

        if (lastStatus_ == LastStatus::Pending && isPendingTimerActive())
            return;

        const auto& icons = getStatusPixmaps(Styling::getParameters(widget_->getContact()).getColor(Styling::StyleVariable::BASE_PRIMARY));

        QPixmap statusIcon;
        switch (lastStatus_)
        {
            case LastStatus::Pending:
                statusIcon = icons.pending_;
                break;
            case LastStatus::DeliveredToServer:
                statusIcon = icons.deliveredToServer_;
                break;
            case LastStatus::DeliveredToPeer:
                statusIcon = icons.deliveredToClient_;
                break;

            default:
                break;
        }

        if (!statusIcon.isNull())
            _p.drawPixmap(_x, _y, _dx, _dy, statusIcon);
    }

}
