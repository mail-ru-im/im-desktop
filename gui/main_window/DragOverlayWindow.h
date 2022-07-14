#pragma once

namespace Ui
{
    class DragOverlayWindow : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void sentFilesOpened();
        void onQuickSent();
        void onHideDragOverlay();
    public:

        enum SendMode
        {
            QuickSend = 1,
            SendWithCaption = 1 << 1
        };

        explicit DragOverlayWindow(const QString& _aimId, QWidget* _parent);

        void setClipPath(const QPainterPath& clip);
        QPainterPath clipPath() const;

        void setMode(const int _mode);
        void setModeByMimeData(const QMimeData* _mimeData);

        void processMimeData(const QMimeData* _mimeData);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void dragEnterEvent(QDragEnterEvent* _e) override;
        void dragLeaveEvent(QDragLeaveEvent* _e) override;
        void dragMoveEvent(QDragMoveEvent* _e) override;
        void dropEvent(QDropEvent* _e) override;

    private:
        void drawQuickSend(QPainter& _p);
        void drawSendWithCaption(QPainter& _p);
        void onTimer();

    private:
        QPainterPath clipPath_;
        QTimer dragMouseoverTimer_;
        bool top_;
        int mode_;
        QString aimId_;
        double radius_;
    };
}