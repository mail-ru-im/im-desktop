#pragma once

#include "ClickWidget.h"
#include "../animation/animation.h"

namespace Ui
{
    class FileSharingIcon : public ClickableWidget
    {
        Q_OBJECT

    public:
        FileSharingIcon(QWidget* _parent = nullptr);
        ~FileSharingIcon();

        void setBytes(const int64_t _bytesTotal, const int64_t _bytesCurrent);
        void setTotalBytes(const int64_t _bytesTotal);
        void setCurrentBytes(const int64_t _bytesCurrent);
        void setDownloaded();
        void reset();

        void setFilename(const QString& _fileName);
        QPixmap getIcon() const;

        static QPixmap getIcon(const QString& _fileName);
        static QColor getFillColor(const QString& _fileName);

        void startAnimation();
        void stopAnimation();
        bool isAnimating() const;

        void onVisibilityChanged(const bool _isVisible);

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        bool isFileDownloading() const;
        bool isFileDownloaded() const;

        QPixmap fileType_;
        QColor iconColor_;

        int64_t bytesTotal_;
        int64_t bytesCurrent_;

        anim::Animation anim_;
    };
}