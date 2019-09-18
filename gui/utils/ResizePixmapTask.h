#pragma once

namespace Utils
{
    class ResizePixmapTask
        : public QObject
        , public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void resizedSignal(QPixmap);

    public:
        ResizePixmapTask(const QPixmap& _preview, const QSize& _size, Qt::AspectRatioMode _aspectMode = Qt::KeepAspectRatio);

        void run() override;

    private:
        QPixmap preview_;
        QSize size_;
        Qt::AspectRatioMode aspectMode_;
    };
}