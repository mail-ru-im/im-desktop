#pragma once

namespace ptt
{
    class ConvertTask
        : public QObject
        , public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void ready(const QString& _aacFile, std::chrono::seconds _duration, QPrivateSignal) const;
        void error(QPrivateSignal) const;

    public:
        explicit ConvertTask(const QByteArray& _wavData, int _sampleRate, int _channelsCount, int _bitesPerSample);

        ~ConvertTask();

        void run() override;

    private:
        const QByteArray wavData_;
        const int sampleRate_;
        const int channelsCount_;
        const int bitesPerSample_;
    };
}