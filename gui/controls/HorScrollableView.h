#pragma once

namespace Ui
{
    class HorScrollableView : public QTableView
    {
        Q_OBJECT

    Q_SIGNALS:
        void enter();
        void leave();

    public:
        explicit HorScrollableView(QWidget* _parent);

    public Q_SLOTS:
        void showScroll();
        void hideScroll();

    protected:
        void wheelEvent(QWheelEvent*) override;
        void enterEvent(QEvent *) override;
        void leaveEvent(QEvent *) override;
        void mouseMoveEvent(QMouseEvent *) override;

    private:
        QGraphicsOpacityEffect* opacityEffect_;
        QPropertyAnimation* fadeAnimation_;
        QTimer* timer_;
    };
}