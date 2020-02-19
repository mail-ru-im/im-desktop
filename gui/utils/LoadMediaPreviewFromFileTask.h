#pragma once

namespace Utils
{
    class LoadMediaPreviewFromFileTask : public QObject, public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void loaded(QPixmap pixmap, const QSize _originalSize);

    public:
        explicit LoadMediaPreviewFromFileTask(const QString& _path);
        ~LoadMediaPreviewFromFileTask();

        void run() override;

    private:
        QString path_;
    };
}
