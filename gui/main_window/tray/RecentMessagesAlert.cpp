#include "stdafx.h"
#include "MessageAlertWidget.h"
#include "RecentMessagesAlert.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../contact_list/RecentsTab.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

#include "../../controls/TextEmojiWidget.h"
#include "../../controls/CustomButton.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../main_window/MainWindow.h"
#include "../../styles/ThemeParameters.h"

namespace
{
    const int max_alerts_count = 3;
    const int max_alerts_count_mail = 1;
    const int alert_visible_time = 5000; //5 sec
    const int alert_hide_animation_time = 2000; //2 sec
    const int view_all_widget_height = 40;

    QSize getCloseButtonSize()
    {
        return QSize(14, 14);
    }

    int getCloseButtonMargin()
    {
        return Utils::scale_value(16);
    }

    QColor getViewAllWidgetColor(const bool _selected)
    {
        return Styling::getParameters().getColor(_selected ? Styling::StyleVariable::BASE_BRIGHT_INVERSE : Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    QColor getViewAllTextColor(const bool _selected)
    {
        return Styling::getParameters().getColor(_selected ? Styling::StyleVariable::TEXT_PRIMARY_HOVER : Styling::StyleVariable::TEXT_PRIMARY);
    }
}

namespace Ui
{
    ViewAllWidget::ViewAllWidget(QWidget* _parent)
        : QWidget(_parent)
        , selected_(false)
    {
    }

    ViewAllWidget::~ViewAllWidget()
    {
    }

    void ViewAllWidget::enterEvent(QEvent* _e)
    {
        selected_ = true;

        update();

        QWidget::enterEvent(_e);
    }

    void ViewAllWidget::leaveEvent(QEvent* _e)
    {
        selected_ = false;

        update();

        QWidget::leaveEvent(_e);
    }

    void ViewAllWidget::mouseReleaseEvent(QMouseEvent* _e)
    {
        Q_EMIT clicked();

        QWidget::mouseReleaseEvent(_e);
    }

    void ViewAllWidget::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        p.fillRect(rect(), getViewAllWidgetColor(selected_));

        p.setPen(getViewAllTextColor(selected_));

        static QString text = QT_TRANSLATE_NOOP("notifications", "View all");

        static QFont font = Fonts::appFontScaled(14, Fonts::FontWeight::Normal);

        p.setFont(font);

        p.drawText(rect(), text, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
    }


    RecentMessagesAlert::RecentMessagesAlert(const AlertType _alertType)
        : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
        , layout_(new QVBoxLayout())
        , alertsCount_(0)
        , closeButton_(new CustomButton(this, qsl(":/controls/close_icon"), getCloseButtonSize()))
        , viewAllWidget_(new ViewAllWidget(this))
        , timer_(new QTimer(this))
        , animation_(new QPropertyAnimation(this, QByteArrayLiteral("windowOpacity"), this))
        , height_(0)
        , cursorIn_(false)
        , viewAllWidgetVisible_(false)
        , alertType_(_alertType)
    {
        maxAlertCount_ = (_alertType == AlertType::alertTypeMessage) ? max_alerts_count : max_alerts_count_mail;

        Styling::Buttons::setButtonDefaultColors(closeButton_);
        closeButton_->setFixedSize(Utils::scale_value(getCloseButtonSize()));

        init();
    }

    RecentMessagesAlert::~RecentMessagesAlert()
    {

    }

    void RecentMessagesAlert::resizeEvent(QResizeEvent* _event)
    {
        const int x = _event->size().width() - getCloseButtonMargin() - Utils::scale_value(getCloseButtonSize().width()) - Ui::get_gui_settings()->get_shadow_width();
        const int y = getCloseButtonMargin() + Ui::get_gui_settings()->get_shadow_width();

        closeButton_->move(QPoint(x, y));

        QWidget::resizeEvent(_event);
    }

    void RecentMessagesAlert::enterEvent(QEvent* e)
    {
        cursorIn_ = true;
        setWindowOpacity(1);
        animation_->stop();
        timer_->start();
        return QWidget::enterEvent(e);
    }

    void RecentMessagesAlert::leaveEvent(QEvent* e)
    {
        cursorIn_ = false;
        return QWidget::leaveEvent(e);
    }

    void RecentMessagesAlert::mouseReleaseEvent(QMouseEvent* e)
    {
        QPoint p = e->pos();
        if (p.y() >= closeButton_->y() && p.y() <= closeButton_->y() + closeButton_->height())
        {
            if (layout_->count() > 1)
            {
                QLayoutItem* item = layout_->itemAt(1);

                if (item && item->widget())
                {
                    if (Ui::MessageAlertWidget* alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget()))
                    {
                        messageAlertClicked(alert->id(), alert->mailId(), alert->mentionId());
                    }
                }
            }
        }
        else if (e->button() == Qt::RightButton)
        {
            closeAlert();
        }
        return QWidget::mouseReleaseEvent(e);
    }

    void RecentMessagesAlert::showEvent(QShowEvent *e)
    {
        Q_EMIT changed();
        return QWidget::showEvent(e);
    }

    void RecentMessagesAlert::hideEvent(QHideEvent *e)
    {
        Q_EMIT changed();
        return QWidget::hideEvent(e);
    }

    void RecentMessagesAlert::init()
    {
        setAttribute(Qt::WA_ShowWithoutActivating);

        QVBoxLayout* topLayout = Utils::emptyVLayout();

        QWidget* topWidget = new QWidget(this);
        topWidget->setCursor(Qt::PointingHandCursor);
        setLayout(topLayout);
        topLayout->addWidget(topWidget);

        topWidget->setStyleSheet(ql1s("QWidget{background: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
        topWidget->setObjectName(qsl("topWidget"));
        topWidget->setStyle(QApplication::style());
        topWidget->setLayout(layout_);
        Testing::setAccessibleName(topWidget, qsl("AS Notification"));

        layout_->setSpacing(0);
        layout_->setMargin(0);

        height_ += Ui::get_gui_settings()->get_shadow_width() * 2;

        viewAllWidget_->setFixedSize(getAlertItemWidth(), Utils::scale_value(view_all_widget_height));
        viewAllWidget_->setContentsMargins(QMargins(0, 0, 0, 0));

        layout_->addWidget(viewAllWidget_);

        viewAllWidget_->hide();

        setFixedSize(getAlertItemWidth() + Ui::get_gui_settings()->get_shadow_width() * 2, height_);

        timer_->setInterval(alert_visible_time);
        timer_->setSingleShot(true);
        connect(timer_, &QTimer::timeout, this, &RecentMessagesAlert::startAnimation);

        connect(closeButton_, &QPushButton::clicked, this, &RecentMessagesAlert::closeAlert);
        connect(closeButton_, &QPushButton::clicked, this, &RecentMessagesAlert::statsCloseAlert);
        Testing::setAccessibleName(closeButton_, qsl("AS Notification closeButton"));

        connect(viewAllWidget_, &ViewAllWidget::clicked, this, &RecentMessagesAlert::viewAll);
        Testing::setAccessibleName(viewAllWidget_, qsl("AS Notification viewAll"));

        Utils::addShadowToWindow(this);

        animation_->setDuration(alert_hide_animation_time);
        animation_->setStartValue(1);
        animation_->setEndValue(0);
        connect(animation_, &QPropertyAnimation::finished, this, &RecentMessagesAlert::closeAlert);
    }

    bool RecentMessagesAlert::isMailAlert() const
    {
        return (alertType_ == AlertType::alertTypeEmail);
    }

    bool RecentMessagesAlert::isMessageAlert() const
    {
        return (alertType_ == AlertType::alertTypeMessage);
    }

    bool RecentMessagesAlert::isMentionAlert() const
    {
        return (alertType_ == AlertType::alertTypeMentionMe);
    }

    void RecentMessagesAlert::startAnimation()
    {
#ifdef __linux__
        closeAlert();
        return;
#endif //__linux__
        if (cursorIn_)
            timer_->start();
        else
            animation_->start();
    }

    void RecentMessagesAlert::messageAlertClicked(const QString& aimId, const QString& mailId, qint64 mentionId)
    {
        closeAlert();
        Utils::InterConnector::instance().getMainWindow()->closePopups({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });

        Q_EMIT messageClicked(aimId, mailId, mentionId, alertType_);
    }

    void RecentMessagesAlert::messageAlertClosed(const QString& /*aimId*/, const QString& /*mailId*/, qint64 /*mentionId*/)
    {
        closeAlert();
        Utils::InterConnector::instance().getMainWindow()->closePopups({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
    }

    void RecentMessagesAlert::closeAlert()
    {
        hide();
        setWindowOpacity(1);
        animation_->stop();
        markShowed();
    }

    void RecentMessagesAlert::statsCloseAlert()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::alert_close);
    }

    void RecentMessagesAlert::viewAll()
    {
        messageAlertClicked(QString(), QString(), -1);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::alert_viewall);
    }

    void RecentMessagesAlert::addAlert(const Data::DlgState& state)
    {
        bool showViewAll = (alertsCount_ == maxAlertCount_ && isMessageAlert());

        int i = 0;

        while (QLayoutItem* item = layout_->itemAt(i))
        {
            auto alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget());
            if (alert && alert->id() == state.AimId_)
            {
                height_ -= getAlertItemHeight();
                alert->setWindowOpacity(0.0);
                layout_->removeWidget(alert);
                alert->deleteLater();
                showViewAll = viewAllWidget_->isVisible() && alertsCount_ == maxAlertCount_ && isMessageAlert();
                --alertsCount_;
            }

            ++i;
        }

        if (alertsCount_ == maxAlertCount_)
        {
            QLayoutItem* item = layout_->itemAt(alertsCount_ - 1);
            QWidget* w = item ? item->widget() : nullptr;
            if (w)
            {
                height_ -= getAlertItemHeight();
                w->setWindowOpacity(0.0);
                layout_->removeWidget(w);
                w->deleteLater();
                --alertsCount_;
            }
        }

        MessageAlertWidget* widget = new MessageAlertWidget(state, this);
        Testing::setAccessibleName(widget, u"AS Notification alert " % state.AimId_ % u' ' % QString::number(state.LastMsgId_));
        connect(widget, &MessageAlertWidget::clicked, this, &RecentMessagesAlert::messageAlertClicked, Qt::DirectConnection);
        connect(widget, &MessageAlertWidget::closed, this, &RecentMessagesAlert::messageAlertClosed, Qt::DirectConnection);
        height_ += getAlertItemHeight();
        layout_->insertWidget(0, widget);
        ++alertsCount_;

        if (showViewAll)
        {
            if (!viewAllWidgetVisible_)
            {
                viewAllWidgetVisible_ = true;
                viewAllWidget_->show();
                height_ += Utils::scale_value(view_all_widget_height);
            }
        }
        else
        {
            if (viewAllWidgetVisible_)
            {
                viewAllWidgetVisible_ = false;
                viewAllWidget_->hide();
                height_ -= Utils::scale_value(view_all_widget_height);
            }
        }

        setFixedHeight(height_);
        setWindowOpacity(1);
        animation_->stop();
        timer_->start();

        closeButton_->raise();

        Q_EMIT changed();
    }

    void RecentMessagesAlert::removeAlert(const QString& _aimId, qint64 _messageId)
    {
        int i = 0;

        while (QLayoutItem* item = layout_->itemAt(i))
        {
            auto alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget());
            if (alert && alert->id() == _aimId && (alert->messageId() == _messageId || alert->mentionId() == _messageId))
            {
                height_ -= getAlertItemHeight();
                alert->setWindowOpacity(0.0);
                layout_->removeWidget(alert);
                alert->deleteLater();
                --alertsCount_;
            }

            ++i;
        }

        if (i == 1)
            closeAlert();
        else
            setFixedHeight(height_);
    }

    void RecentMessagesAlert::markShowed()
    {
        if (!isMessageAlert())
            return;

        int i = 0;
        while (QLayoutItem* item = layout_->itemAt(i))
        {
            auto alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget());
            if (alert)
            {
                height_ -= getAlertItemHeight();
                alert->setWindowOpacity(0.0);
                layout_->removeWidget(alert);
                alert->deleteLater();
                --alertsCount_;
            }
            else // there is no need to increment if item was deleted
            {
                ++i;
            }
        }

        if (viewAllWidgetVisible_)
        {
            viewAllWidgetVisible_ = false;
            viewAllWidget_->hide();
            height_ -= Utils::scale_value(view_all_widget_height);
        }

        setFixedHeight(height_);
    }

    bool RecentMessagesAlert::updateMailStatusAlert(const Data::DlgState& state)
    {
        bool showViewAll = (alertsCount_ == maxAlertCount_ && isMessageAlert());
        int i = 0;

        bool mailStatusFound = false;
        while (QLayoutItem* item = layout_->itemAt(i))
        {
            auto alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget());
            if (alert && alert->id() == state.AimId_ && alert->mailId().isEmpty())
            {
                height_ -= getAlertItemHeight();
                alert->setWindowOpacity(0.0);
                layout_->removeWidget(alert);
                alert->deleteLater();
                showViewAll = viewAllWidget_->isVisible() && alertsCount_ == maxAlertCount_ && isMessageAlert();
                --alertsCount_;
                mailStatusFound = true;
            }
            ++i;
        }

        if (!mailStatusFound)
            return false;

        if (alertsCount_ == maxAlertCount_)
        {
            QLayoutItem* item = layout_->itemAt(alertsCount_);
            QWidget* w = item ? item->widget() : nullptr;
            if (w)
            {
                height_ -= getAlertItemHeight();
                w->setWindowOpacity(0.0);
                layout_->removeWidget(w);
                w->deleteLater();
                --alertsCount_;
            }
        }

        MessageAlertWidget* widget = new MessageAlertWidget(state, this);
        connect(widget, &MessageAlertWidget::clicked, this, &RecentMessagesAlert::messageAlertClicked, Qt::DirectConnection);
        connect(widget, &MessageAlertWidget::closed, this, &RecentMessagesAlert::messageAlertClosed, Qt::DirectConnection);
        height_ += getAlertItemHeight();
        layout_->insertWidget(1, widget);
        ++alertsCount_;

        setFixedHeight(height_);

        Q_EMIT changed();

        return true;
    }
}
