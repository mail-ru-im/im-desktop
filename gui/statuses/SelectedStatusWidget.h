#pragma once

#include "Status.h"
#include "controls/ClickWidget.h"

namespace Statuses
{
    class SelectedStatusWidget_p;

    //////////////////////////////////////////////////////////////////////////
    // SelectedStatusWidget
    //////////////////////////////////////////////////////////////////////////

    class SelectedStatusWidget : public QWidget
    {
        Q_OBJECT
    public:
        SelectedStatusWidget(QWidget* _parent);
        ~SelectedStatusWidget();
        void setStatus(const Status& _status);

    Q_SIGNALS:
        void avatarClicked(QPrivateSignal);
        void statusClicked(QPrivateSignal);
        void durationClicked(QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        std::unique_ptr<SelectedStatusWidget_p> d;
    };

    class AvatarWithStatus_p;

    //////////////////////////////////////////////////////////////////////////
    // AvatarWithStatus
    //////////////////////////////////////////////////////////////////////////

    class AvatarWithStatus : public QWidget
    {
        Q_OBJECT
    public:
        AvatarWithStatus(QWidget* _parent);
        ~AvatarWithStatus();

        void setStatus(const Status& _status);

    Q_SIGNALS:
        void avatarClicked(QPrivateSignal);
        void statusClicked(QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;

    private Q_SLOTS:
        void avatarChanged(const QString& _aimId);

    private:
        std::unique_ptr<AvatarWithStatus_p> d;
    };

    class StatusDurationButton_p;

    class StatusDurationButton : public Ui::ClickableWidget
    {
        Q_OBJECT
    public:
        StatusDurationButton(QWidget* _parent);
        ~StatusDurationButton();

        void setText(const QString& _text);
        void setMaxWidth(int _width);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        std::unique_ptr<StatusDurationButton_p> d;
    };
}
