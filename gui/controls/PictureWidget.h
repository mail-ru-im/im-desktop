#pragma once

namespace Ui {

    class PictureWidget : public QWidget
    {
        QPixmap pixmapToDraw_;

        int x_;
        int y_;
        int align_;

    protected:
        virtual void paintEvent(QPaintEvent*) override;

    public:
        explicit PictureWidget(QWidget* _parent, const QString& _imageName = QString());

        void setImage(const QString& _imageName);
        void setImage(const QPixmap& _pixmap)
        {
            setImage(_pixmap, -1);
        }
        void setImage(const QPixmap& _pixmap, int radius);

        void setOffsets(const int _horOffset, const int _verOffset);
        void setIconAlignment(const Qt::Alignment _flags);
    };
}