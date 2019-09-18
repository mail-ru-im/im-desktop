#pragma once


namespace Ui
{
    class HistoryControlPageItem;
    enum class LastStatus;

    class LastStatusAnimation : public QObject
    {
        Q_OBJECT

    private Q_SLOTS:
        void onPendingTimeout();

    public:
        explicit LastStatusAnimation(HistoryControlPageItem* _parent);
        ~LastStatusAnimation();

        void setLastStatus(const LastStatus _lastStatus);

        void hideStatus();
        void showStatus();

        void drawLastStatus(
            QPainter& _p,
            const int _x,
            const int _y,
            const int _dx,
            const int _dy) const;

        static QPixmap getPendingPixmap(const QColor& _color);
        static QPixmap getDeliveredToClientPixmap(const QColor& _color);
        static QPixmap getDeliveredToServerPixmap(const QColor& _color);

    private:
        void startPendingTimer();
        void stopPendingTimer();
        bool isPendingTimerActive() const;

    private:
        HistoryControlPageItem* widget_;

        bool isActive_;
        bool isPlay_;
        bool show_;

        QTimer* pendingTimer_;

        LastStatus  lastStatus_;
    };
}