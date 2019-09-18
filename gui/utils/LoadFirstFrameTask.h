#pragma once

namespace Utils
{
    class LoadFirstFrameTask
        : public QObject
        , public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void loadedSignal(const QPixmap& _pixmap, const QSize _originalSize);
        void duration(qint64 _duration);
        void gotAudio(bool _gotAudio);

    public:
        explicit LoadFirstFrameTask(QString path);

        virtual ~LoadFirstFrameTask();

        virtual void run() override;

    private:
        const QString Path_;
    };
}
