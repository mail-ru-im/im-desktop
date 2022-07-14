#pragma once

namespace Ui
{
    class VideoWindowHeader : public QWidget
    {
        Q_OBJECT

    public:
        VideoWindowHeader(QWidget* _parent);
        ~VideoWindowHeader();

        void setTitle(const QString& _text);

    Q_SIGNALS:
        void onMinimized();
        void onFullscreen();
        void requestHangup();

    private Q_SLOTS:
        void updateTheme();

    protected:
        void mouseDoubleClickEvent(QMouseEvent* _event) override;

    private:
        std::unique_ptr<class VideoWindowHeaderPrivate> d;
    };
}
