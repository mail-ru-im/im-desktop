#pragma once

namespace Utils
{
    class LoadPixmapFromFileTask
        : public QObject
        , public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void loadedSignal(QPixmap pixmap, const QSize _originalSize);

    public:
        explicit LoadPixmapFromFileTask(const QString& path, const QSize& _maxSize = QSize());

        virtual ~LoadPixmapFromFileTask();

        void run() override;

    private:
        const QString Path_;
        const QSize maxSize_;
    };
}