#pragma once

#include "../../../styles/WallpaperId.h"
#include "controls/ClickWidget.h"

namespace Styling
{
    using WallpaperPtr = std::shared_ptr<class ThemeWallpaper>;
}

namespace Ui
{
    class WallpaperWidget : public ClickableWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void wallpaperClicked(const Styling::WallpaperId& _id, QPrivateSignal);

    private Q_SLOTS:
        void onPreviewAwailable(const Styling::WallpaperId& _id);

    public:
        WallpaperWidget(QWidget* _parent, const Styling::WallpaperPtr _wallpaper);
        const Styling::WallpaperId& getId() const noexcept;
        void setSelected(const bool _isSelected);
        void setCaption(const QString& _capt, const QColor& _captColor);
        void dropCaption();

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void showEvent(QShowEvent* _e) override;
        void resizeEvent(QResizeEvent*) override;

        virtual QColor bgColor() const;
        virtual QColor tint() const;
        virtual bool isPreviewNeeded() const;

        QPainterPath clipPath_;

    private:
        void setPreview(const QPixmap& _preview);

        bool isSelected_;
        Styling::WallpaperPtr wallpaper_;
        QPixmap pixmap_;

        QString caption_;
        QColor captionColor_;
    };

    class UserWallpaperWidget : public WallpaperWidget
    {
    public:
        UserWallpaperWidget(QWidget* _parent, const Styling::WallpaperPtr _wallpaper);

    protected:
        void paintEvent(QPaintEvent* _e) override;

        QColor bgColor() const override;
        QColor tint() const override;
        bool isPreviewNeeded() const override;
    };
}
