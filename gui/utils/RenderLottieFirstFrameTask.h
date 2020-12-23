#pragma once

namespace Utils
{
    class RenderLottieFirstFrameTask : public QObject, public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void result(const QImage& _firstFrame, QPrivateSignal) const;

    public:
        RenderLottieFirstFrameTask(const QString& _fileName, QSize _size = {});

        void run() override;

    private:
        QSize size_;
        QString path_;
    };
}