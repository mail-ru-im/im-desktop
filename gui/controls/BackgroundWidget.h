#pragma once

Q_DECLARE_LOGGING_CATEGORY(bgWidget)

namespace Ui
{
    class BackgroundWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void wallpaperChanged(QPrivateSignal) const;

    public:
        BackgroundWidget(QWidget* _parent);

        void setImage(const QPixmap& _pixmap, const bool _tiling);
        void setColor(const QColor& _color);

        void setViewRect(const QRect& _rect);
        void updateWallpaper(const QString& _aimId);

        bool isFlatColor() const;

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        bool tiling_;
        QPixmap wallpaper_;
        QColor bgColor_;
        QRect viewRect_;
    };
}