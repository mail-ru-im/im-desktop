#pragma once

namespace Utils
{
    // stub for https://bugreports.qt.io/browse/QTBUG-60231
    class OpacityEffect : public QGraphicsEffect
    {
        Q_OBJECT
        Q_PROPERTY(double opacity READ opacity WRITE setOpacity)

    public:
        explicit OpacityEffect(QWidget* _parent);

        void setOpacity(const double _opacity);
        double opacity() const noexcept { return opacity_; }

    protected:
        void draw(QPainter* _painter) override;

    private:
        double opacity_;
    };

    class GradientEdgesEffect : public QGraphicsEffect
    {
        Q_OBJECT

    public:
        explicit GradientEdgesEffect(QWidget* _parent, const int _width, const Qt::Alignment _sides, const int _offset = 0);

        void setSides(const Qt::Alignment _sides);
        Qt::Alignment getSides() const noexcept { return sides_; }

        void setWidth(const int _width);
        int width() const noexcept { return width_; }

        void setOffset(const int _offset);
        int offset() const noexcept { return offset_; }

    protected:
        void draw(QPainter* _painter) override;

    private:
        int offset_;
        int width_;
        Qt::Alignment sides_;
    };
}