#pragma once

namespace Utils
{
    class BlurPixmapTask : public QObject, public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void blurred(const QPixmap& _result, qint64 _srcCacheKey, QPrivateSignal) const;

    public:
        BlurPixmapTask(const QPixmap& _source, const unsigned int _radius, const int _coreCount = -1);

        void run();

    private:
        QPixmap blurPixmap() const;

    private:
        QPixmap source_;
        unsigned int radius_;
        int coreCount_;
    };
}