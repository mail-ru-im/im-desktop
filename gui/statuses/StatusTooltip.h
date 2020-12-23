#pragma once

#include "Status.h"

namespace Ui
{
    class StatusTooltip_p;

    class StatusTooltip : public QObject
    {
        Q_OBJECT
    public:
        using RectCallback = std::function<QRect()>;

        ~StatusTooltip();

        void objectHovered(RectCallback _rectCallback,
                           const QString& _contactId,
                           QWidget* _object,
                           QWidget* _parent = nullptr, // parent is used to check if object rect is inside parent's rect, i.e. when object is inside of scroll area
                           Qt::CursorShape _objectCursorShape = Qt::PointingHandCursor);

        void updatePosition();
        void hide();

        static StatusTooltip* instance();

    private Q_SLOTS:
        void onTimer();

    private:
        StatusTooltip();

        std::unique_ptr<StatusTooltip_p> d;
    };

    class StatusTooltipWidget_p;
    class StatusTooltipWidget : public QWidget
    {
        Q_OBJECT
    public:
        StatusTooltipWidget(const Statuses::Status& _status, QWidget* _object, QWidget* _parent, Qt::CursorShape _objectCursorShape);
        ~StatusTooltipWidget();

        void showAt(const QRect& _objectRect);

        void setRectCallback(StatusTooltip::RectCallback _callback);
        void updatePosition();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void wheelEvent(QWheelEvent* _event) override;
        bool eventFilter(QObject* _object, QEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void keyReleaseEvent(QKeyEvent* _event) override;

    private Q_SLOTS:
        void onEmojiAnimationTimer();
        void onVisibilityTimer();

    private:
        std::unique_ptr<StatusTooltipWidget_p> d;
    };
}
