#include "stdafx.h"
#include "MessageAlertWidget.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace Ui
{
    int getAlertItemHeight()
    {
        return Utils::scale_value(100);
    }

    int getAlertItemWidth()
    {
        return Utils::scale_value(376);
    }

    static bool isMail(const Data::DlgState& _state)
    {
        return _state.AimId_ == u"mail";
    }

    std::unique_ptr<RecentItemBase> createItem(const Data::DlgState& _state)
    {
        if (isMail(_state))
        {
            if (_state.MailId_.isEmpty())
            {
                return std::make_unique<ComplexMailAlertItem>(_state);
            }
            else
            {
                return std::make_unique<MailAlertItem>(_state);
            }
        }
        else
        {
            return std::make_unique<MessageAlertItem>(_state, false);
        }
    }

    MessageAlertWidget::MessageAlertWidget(const Data::DlgState& _state, QWidget* _parent)
        : QWidget(_parent)
        , State_(_state)
        , item_(createItem(_state))
        , hovered_(false)
    {
        Options_.initFrom(this);
        setFixedSize(getAlertItemWidth(), getAlertItemHeight());
        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &MessageAlertWidget::avatarChanged);
    }

    MessageAlertWidget::~MessageAlertWidget()
    {
    }

    QString MessageAlertWidget::id() const
    {
        return State_.AimId_;
    }

    QString MessageAlertWidget::mailId() const
    {
        return State_.MailId_;
    }

    qint64 MessageAlertWidget::mentionId() const
    {
        return State_.mentionMsgId_;
    }

    void MessageAlertWidget::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);

        item_->draw(painter, Options_.rect, Ui::ViewParams(), false, hovered_, false, false);

        //Delegate_->paint(&painter, Options_, State_, false);
    }

    void MessageAlertWidget::resizeEvent(QResizeEvent* e)
    {
        Options_.initFrom(this);
        return QWidget::resizeEvent(e);
    }

    void MessageAlertWidget::enterEvent(QEvent* e)
    {
        hovered_ = true;
        update();
        return QWidget::enterEvent(e);
    }

    void MessageAlertWidget::leaveEvent(QEvent* e)
    {
        hovered_ = false;
        update();
        return QWidget::leaveEvent(e);
    }

    void MessageAlertWidget::mousePressEvent(QMouseEvent* e)
    {
        e->accept();
    }

    void MessageAlertWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        e->accept();
        if (e->button() == Qt::RightButton)
            Q_EMIT closed(id(), mailId(), mentionId());
        else
            Q_EMIT clicked(id(), mailId(), mentionId());
    }

    void MessageAlertWidget::avatarChanged(const QString& aimId)
    {
        if (aimId == State_.AimId_ || State_.Friendly_.contains(aimId))
            update();
    }
}
