#pragma once

namespace Ui
{
    class ContactDialog;

    class DragOverlayWindow : public QWidget
    {
        Q_OBJECT

    public:

        enum SendMode
        {
            QuickSend = 1,
            SendWithCaption = 1 << 1
        };

        explicit DragOverlayWindow(ContactDialog* _parent);

        void setMode(const int _mode);
        void setModeByMimeData(const QMimeData* _mimeData);

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
        ContactDialog* Parent_;
        QTimer dragMouseoverTimer_;
        bool top_;
        int mode_;
    };
}