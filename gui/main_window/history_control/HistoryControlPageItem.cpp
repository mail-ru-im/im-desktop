#include "stdafx.h"

#include "HistoryControlPageItem.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/InterConnector.h"
#include "../../controls/TooltipWidget.h"
#include "../contact_list/ContactListModel.h"
#include "../friendly/FriendlyContainer.h"
#include "../MainPage.h"
#include "../ContactDialog.h"
#include "MessageStyle.h"
#include "LastStatusAnimation.h"

namespace
{
    constexpr int ANIM_MAX_VALUE = 100;
    constexpr int ADD_OFFSET = 10;
}

namespace Ui
{
    HistoryControlPageItem::HistoryControlPageItem(QWidget *parent)
        : QWidget(parent)
        , Selected_(false)
        , HasTopMargin_(false)
        , HasAvatar_(false)
        , hasSenderName_(false)
        , isChainedToPrev_(false)
        , isChainedToNext_(false)
        , HasAvatarSet_(false)
        , isDeleted_(false)
        , isEdited_(false)
        , isContextMenuEnabled_(true)
        , lastStatus_(LastStatus::None)
        , lastStatusAnimation_(nullptr)
        , QuoteAnimation_(parent)
        , isChat_(false)
    {
        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &HistoryControlPageItem::avatarChanged);

        setMouseTracking(true);
    }

    void HistoryControlPageItem::clearSelection()
    {
        if (Selected_)
            update();

        Selected_ = false;
        emit selectionChanged();
    }

    bool HistoryControlPageItem::hasAvatar() const
    {
        return HasAvatar_;
    }

    bool HistoryControlPageItem::hasSenderName() const
    {
        return hasSenderName_;
    }

    bool HistoryControlPageItem::hasTopMargin() const
    {
        return HasTopMargin_;
    }

    void HistoryControlPageItem::select()
    {
        if (!Selected_)
            update();

        Selected_ = true;
        emit selectionChanged();
    }

    void HistoryControlPageItem::setTopMargin(const bool value)
    {
        if (HasTopMargin_ == value)
            return;

        HasTopMargin_ = value;

        updateSize();
        updateGeometry();
        update();
    }

    bool HistoryControlPageItem::isSelected() const
    {
        return Selected_;
    }

    void HistoryControlPageItem::onActivityChanged(const bool /*isActive*/)
    {
    }

    void HistoryControlPageItem::onVisibilityChanged(const bool /*isVisible*/)
    {
    }

    void HistoryControlPageItem::onDistanceToViewportChanged(const QRect& /*_widgetAbsGeometry*/, const QRect& /*_viewportVisibilityAbsRect*/)
    {}

    void HistoryControlPageItem::setHasAvatar(const bool value)
    {
        HasAvatar_ = value;
        HasAvatarSet_ = true;

        updateGeometry();
    }

    void HistoryControlPageItem::setHasSenderName(const bool _hasSender)
    {
        hasSenderName_ = _hasSender;
        updateGeometry();
    }

    void HistoryControlPageItem::setChainedToPrev(const bool _isChained)
    {
        if (isChainedToPrev_ != _isChained)
        {
            isChainedToPrev_ = _isChained;

            onChainsChanged();
            updateGeometry();
        }
    }

    void HistoryControlPageItem::setChainedToNext(const bool _isChained)
    {
        if (isChainedToNext_ != _isChained)
        {
            isChainedToNext_ = _isChained;

            onChainsChanged();
            updateSize();
            updateGeometry();
            update();
        }
    }

    void HistoryControlPageItem::setContact(const QString& _aimId)
    {
        assert(!_aimId.isEmpty());

        if (_aimId == aimId_)
            return;

        assert(aimId_.isEmpty());

        aimId_ = _aimId;
        QuoteAnimation_.setAimId(_aimId);
    }

    void HistoryControlPageItem::setSender(const QString& /*_sender*/)
    {

    }

    void HistoryControlPageItem::updateFriendly(const QString&, const QString&)
    {
    }

    void HistoryControlPageItem::setDeliveredToServer(const bool _delivered)
    {

    }

    void HistoryControlPageItem::drawLastStatusIconImpl(QPainter& _p, int _rightPadding, int _bottomPadding)
    {
        if (lastStatusAnimation_)
        {
            const QRect rc = rect();

            const auto iconSize = MessageStyle::getLastStatusIconSize();

            lastStatusAnimation_->drawLastStatus(
                _p,
                rc.right() - MessageStyle::getRightBubbleMargin() - _rightPadding + MessageStyle::getLastStatusLeftMargin(),
                rc.bottom() - iconSize.height() - _bottomPadding - bottomOffset(),
                iconSize.width(),
                iconSize.height());
        }
    }

    int HistoryControlPageItem::maxHeadsCount() const
    {
        if (headsAtBottom())
            return 5;

        const auto w = width();
        if (w >= MessageStyle::fiveHeadsWidth())
            return 5;

        if (w >= MessageStyle::fourHeadsWidth())
            return 4;

        return 3;
    }

    void HistoryControlPageItem::avatarChanged(const QString& _aimid)
    {
        if (std::any_of(heads_.cbegin(), heads_.cend(), [&_aimid](const auto& h) { return h.aimid_ == _aimid; }))
            update();
    }

    void HistoryControlPageItem::drawLastStatusIcon(QPainter& _p,  LastStatus _lastStatus, const QString& _aimid, const QString& _friendly, int _rightPadding)
    {
        if (!heads_.isEmpty())
            return;

        switch (lastStatus_)
        {
        case LastStatus::None:
        case LastStatus::Read:
            break;
        case LastStatus::Pending:
        case LastStatus::DeliveredToServer:
        case LastStatus::DeliveredToPeer:
            drawLastStatusIconImpl(_p, _rightPadding, 0);
            break;
        default:
            break;
        }
    }

    void HistoryControlPageItem::mouseMoveEvent(QMouseEvent* e)
    {
        if (!heads_.isEmpty())
        {
            if (showHeadsTooltip(rect(), e->pos()))
                setCursor(Qt::PointingHandCursor);
        }

        return QWidget::mouseMoveEvent(e);
    }

    void HistoryControlPageItem::mousePressEvent(QMouseEvent* e)
    {
        pressPoint = e->pos();
        return QWidget::mousePressEvent(e);
    }

    void HistoryControlPageItem::mouseReleaseEvent(QMouseEvent* e)
    {
        if (!heads_.isEmpty() && Utils::clicked(pressPoint, e->pos()))
        {
            clickHeads(rect(), e->pos(), e->button() == Qt::LeftButton);
        }

        return QWidget::mouseReleaseEvent(e);
    }

    bool HistoryControlPageItem::showHeadsTooltip(const QRect& _rc, const QPoint& _pos)
    {
        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        auto xMargin = avatarSize + MessageStyle::getLastReadAvatarMargin();
        auto maxCount = maxHeadsCount();
        auto i = heads_.size() > maxCount ? maxCount : heads_.size() - 1;
        for (; i >= 0; --i)
        {
            const auto r = QRect(_rc.right() - xMargin, _rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin(), avatarSize, avatarSize);
            if (r.contains(_pos))
            {
                auto text = heads_[i].friendly_;
                if (i == maxCount)
                {
                    for (auto j = i + 1; j < heads_.size(); ++j)
                    {
                        text += QChar::LineFeed;
                        text += heads_[j].friendly_;
                    }
                }

                if (!text.isEmpty())
                    Tooltip::show(text, QRect(mapToGlobal(r.topLeft()), QSize(avatarSize, avatarSize)));

                return i != maxCount || (i == maxCount && heads_.size() == maxCount + 1);
            }

            xMargin += (avatarSize + MessageStyle::getLastReadAvatarOffset());
        }

        return false;
    }

    bool HistoryControlPageItem::clickHeads(const QRect& _rc, const QPoint& _pos, bool _left)
    {
        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        auto xMargin = avatarSize + MessageStyle::getLastReadAvatarMargin();
        auto maxCount = maxHeadsCount();
        auto i = heads_.size() > maxCount ? maxCount : heads_.size() - 1;
        for (; i >= 0; --i)
        {
            const auto r = QRect(_rc.right() - xMargin, _rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin(), avatarSize, avatarSize);
            if (r.contains(_pos) && (i != maxCount || (i == maxCount && heads_.size() == maxCount + 1)))
            {
                if (_left)
                {
                    Utils::openDialogOrProfile(heads_.at(i).aimid_, Utils::OpenDOPParam::aimid);
                    Tooltip::hide();
                }
                else
                {
                    emit mention(heads_.at(i).aimid_, heads_.at(i).friendly_);
                }
                return true;
            }

            xMargin += (avatarSize + MessageStyle::getLastReadAvatarOffset());
        }

        return false;
    }

    bool HistoryControlPageItem::hasHeads() const noexcept
    {
        return !heads_.isEmpty();
    }

    bool HistoryControlPageItem::headsAtBottom() const
    {
        if (isOutgoing())
            return true;

        auto dialog = Utils::InterConnector::instance().getContactDialog();
        if (!dialog)
            return false;

        return dialog->width() < Utils::scale_value(460);
    }

    QMap<QString, QVariant> HistoryControlPageItem::makeData(const QString& _command, const QString& _arg)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;

        if (!_arg.isEmpty())
            result[qsl("arg")] = _arg;

        return result;
    }

    bool HistoryControlPageItem::isChainedToPrevMessage() const
    {
        return isChainedToPrev_;
    }

    bool HistoryControlPageItem::isChainedToNextMessage() const
    {
        return isChainedToNext_;
    }

    bool HistoryControlPageItem::isContextMenuReplyOnly() const noexcept
    {
        if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
            return contactDialog->isRecordingPtt();
        return false;
    }

    void HistoryControlPageItem::drawHeads(QPainter& _p) const
    {
        if (heads_.isEmpty())
            return;

        const QRect rc = rect();

        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        const int size = Utils::scale_bitmap(avatarSize);

        auto xMargin = avatarSize + MessageStyle::getLastReadAvatarMargin() - Utils::scale_value(1);
        auto addMargin = 0, addingCount = 0, delCount = 0;
        const auto curAdd = addAnimation_.isRunning() ? addAnimation_.current() / ANIM_MAX_VALUE : 0;
        const auto curDel = removeAnimation_.isRunning() ? removeAnimation_.current() / ANIM_MAX_VALUE : ANIM_MAX_VALUE;
        for (const auto& h : heads_)
        {
            if (h.adding_)
            {
                addMargin += Utils::scale_value(ADD_OFFSET) - (Utils::scale_value(ADD_OFFSET) * curAdd);
                addingCount++;
            }
            if (h.removing_)
            {
                delCount++;
            }
        }

        auto maxCount = maxHeadsCount();
        const auto addPhAnimated = addMargin != 0 && heads_.size() - addingCount == maxCount;
        const auto delPhAnimated = curDel != ANIM_MAX_VALUE && heads_.size() - delCount <= maxCount;

        auto i = heads_.size() > maxCount ? maxCount : heads_.size() - 1;
        if (addPhAnimated && i < (heads_.size() - 1))
            i++;

        static const auto morePlaceholder = Utils::loadPixmap(qsl(":/history/i_more_seens_100"));

        for (; i >= 0; --i)
        {
            Utils::PainterSaver ps(_p);

            bool isDefault = false;
            QPixmap avatar;
            if (i == maxCount && !addPhAnimated && heads_.size() > maxCount + 1)
                avatar = morePlaceholder;
            else
                avatar = *Logic::GetAvatarStorage()->GetRounded(heads_[i].aimid_, heads_[i].friendly_, size, QString(), isDefault, false, false);

            auto margin = xMargin;
            if (heads_[i].adding_ && addAnimation_.isRunning() && i != maxCount)
            {
                _p.setOpacity(curAdd);
                margin += addMargin;
            }
            else if (heads_[i].removing_ || (i == maxCount && delPhAnimated))
            {
                _p.setOpacity(curDel);
            }

            _p.drawPixmap(
                rc.right() - margin,
                rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin(),
                avatarSize,
                avatarSize,
                avatar);

            if (i == (maxCount) && addPhAnimated)
            {
                avatar = morePlaceholder;
                _p.drawPixmap(
                    rc.right() - xMargin,
                    rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin() - (avatarSize - avatarSize * curAdd),
                    avatarSize,
                    avatarSize,
                    avatar);
            }

            xMargin += (avatarSize + MessageStyle::getLastReadAvatarOffset());
        }
    }

    qint64 HistoryControlPageItem::getId() const
    {
        return -1;
    }

    qint64 HistoryControlPageItem::getPrevId() const
    {
        return -1;
    }

    const QString& HistoryControlPageItem::getInternalId() const
    {
        static const QString _id;
        return _id;
    }

    QString HistoryControlPageItem::getSourceText() const
    {
        return QString();
    }

    void HistoryControlPageItem::setDeleted(const bool _isDeleted)
    {
        isDeleted_ = _isDeleted;
    }

    bool HistoryControlPageItem::isDeleted() const
    {
        return isDeleted_;
    }

    void HistoryControlPageItem::setEdited(const bool _isEdited)
    {
        isEdited_ = _isEdited;
    }

    bool HistoryControlPageItem::isEdited() const
    {
        return isEdited_;
    }

    void HistoryControlPageItem::setLastStatus(LastStatus _lastStatus)
    {
        if (lastStatus_ == _lastStatus)
            return;

        lastStatus_ = _lastStatus;
        if (lastStatus_ != LastStatus::None)
        {
            if (!lastStatusAnimation_)
                lastStatusAnimation_ = new LastStatusAnimation(this);
        }

        if (lastStatusAnimation_)
        {
            lastStatusAnimation_->setLastStatus(lastStatus_);
            showMessageStatus();
        }

        if (!isChat_)
        {
            if (lastStatus_ == LastStatus::Read)
            {
                Data::ChatHead h;
                h.aimid_ = aimId_;
                h.friendly_ = Logic::GetFriendlyContainer()->getFriendly(aimId_);
                setHeads({ std::move(h) });
            }
            else
            {
                setHeads({});
            }
        }
    }

    LastStatus HistoryControlPageItem::getLastStatus() const
    {
        return lastStatus_;
    }

    int HistoryControlPageItem::bottomOffset() const
    {
        auto margin =  Utils::getShadowMargin();

        if (isChat() && hasHeads() && headsAtBottom())
        {
            margin += MessageStyle::getLastReadAvatarSize() + MessageStyle::getLastReadAvatarOffset();
        }
        else if (isChat() && isOutgoing() && !nextIsOutgoing())
        {
            auto single = false;
            auto dialog = Utils::InterConnector::instance().getContactDialog();
            if (dialog)
                single = dialog->width() < Utils::scale_value(460);

            margin += single ? MessageStyle::getTopMargin(true) : (MessageStyle::getLastReadAvatarSize() + MessageStyle::getLastReadAvatarOffset());
        }

        return margin;
    }

    void HistoryControlPageItem::setHeads(const Data::HeadsVector& _heads)
    {
        if (!isChat_)
        {
            if (heads_.empty() && !_heads.empty())
                return addHeads(_heads);
            else if (!heads_.empty() && _heads.empty())
                return removeHeads(heads_);
        }

        if (areTheSameHeads(_heads))
            return;

        heads_.clear();
        for (const auto& h : _heads)
        {
            if (!heads_.contains(h))
                heads_.push_back(h);
        }

        updateSize();
        updateGeometry();
        update();
    }

    void HistoryControlPageItem::addHeads(const Data::HeadsVector& _heads)
    {
        if (areTheSameHeads(_heads))
            return;

        for (const auto& h : _heads)
            heads_.removeAll(h);

        const auto empty = heads_.isEmpty();

        for (const auto& h : _heads)
        {
            heads_.push_front(h);
            heads_.front().adding_ = !empty;
        }

        if (empty)
        {
            if (headsAtBottom() && !heightAnimation_.isRunning())
            {
                heightAnimation_.finish();
                heightAnimation_.start([this]() { updateGeometry(); update(); updateSize(); }, 0, ANIM_MAX_VALUE, 200, anim::easeOutExpo, 1);
            }
        }
        else
        {
            if (!addAnimation_.isRunning())
            {
                addAnimation_.finish();
                addAnimation_.start([this]()
                {
                    if (addAnimation_.current() == ANIM_MAX_VALUE)
                    {
                        for (auto& h : heads_)
                            h.adding_ = false;

                        updateGeometry();
                        updateSize();
                    }
                    update();
                }, 0, ANIM_MAX_VALUE, 200, anim::easeOutExpo, 1);
            }
        }

        updateGeometry();
        update();
    }

    void HistoryControlPageItem::removeHeads(const Data::HeadsVector& _heads)
    {
        auto delCount = 0;
        for (const auto& h : _heads)
        {
            const auto ind = heads_.indexOf(h);
            if (ind == -1)
                continue;

            heads_[ind].removing_ = true;
            ++delCount;
        }

        if (!removeAnimation_.isRunning())
        {
            removeAnimation_.finish();
            removeAnimation_.start([this]()
            {
                if (removeAnimation_.current() == 0)
                {
                    heads_.erase(std::remove_if(heads_.begin(), heads_.end(), [](const auto& x) { return x.removing_; }), heads_.end());

                    updateGeometry();
                    updateSize();
                }
                update();
            }, ANIM_MAX_VALUE, 0, isChat_ ? 200 : 0, anim::easeOutExpo, 1);
        }

        if (delCount && delCount == heads_.size())
        {
            if (!heightAnimation_.isRunning())
            {
                heightAnimation_.finish();
                heightAnimation_.start([this]() { updateGeometry(); update(); updateSize(); }, ANIM_MAX_VALUE, 0, 200, anim::easeOutExpo, 1);
            }
        }

        updateGeometry();
        update();
    }

    bool HistoryControlPageItem::areTheSameHeads(const QVector<Data::ChatHead>& _heads) const
    {
        return Data::isSameHeads(heads_, _heads);
    }

    void HistoryControlPageItem::updateSize()
    {
        resize(size());
    }

    void HistoryControlPageItem::setIsChat(bool _isChat)
    {
        isChat_ = _isChat;
    }

    void HistoryControlPageItem::showMessageStatus()
    {
        if (lastStatus_ == LastStatus::DeliveredToPeer || lastStatus_ == LastStatus::DeliveredToServer)
        {
            if (lastStatusAnimation_)
                lastStatusAnimation_->showStatus();
        }
    }


    void HistoryControlPageItem::hideMessageStatus()
    {
        if (lastStatus_ == LastStatus::DeliveredToPeer || lastStatus_ == LastStatus::DeliveredToServer)
        {
            if (lastStatusAnimation_)
                lastStatusAnimation_->hideStatus();
        }
    }
}
