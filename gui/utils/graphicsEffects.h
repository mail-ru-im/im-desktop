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
}