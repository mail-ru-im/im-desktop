#pragma once

#include "ClickWidget.h"

namespace Ui
{
    class FileSharingIcon : public ClickableWidget
    {
        Q_OBJECT

    public:
        FileSharingIcon(bool _isOutgoing, QWidget* _parent = nullptr);
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
        static QSize getIconSize();

        void startAnimation();
        void stopAnimation();
        bool isAnimating() const;

        void onVisibilityChanged(const bool _isVisible);

        enum class BlockReason
        {
            NoBlock,
            Antivirus,
            TrustCheck,
        };
        void setBlocked(BlockReason _reason);

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        bool isFileDownloading() const;
        bool isFileDownloaded() const;
        void initAnimation();

    private:
        QPixmap overrideIcon_;
        QPixmap fileType_;
        QColor iconColor_;
        QString fileName_;
        BlockReason blockReason_;
        Styling::ThemeChecker themeChecker_;

        int64_t bytesTotal_ = 0;
        int64_t bytesCurrent_ = 0;

        QVariantAnimation* anim_ = nullptr;
        bool isOutgoing_ = false;
    };
}