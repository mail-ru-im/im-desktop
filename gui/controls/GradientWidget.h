#pragma once

namespace Ui
{
    class GradientWidget : public QWidget
    {
        Q_OBJECT

    public:
        GradientWidget(QWidget* _parent, const QColor& _left, const QColor& _right, const double _lPos = 0.0, const double _rPos = 1.0);
        void updateColors(const QColor& _left, const QColor& _right);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        QColor left_;
        QColor right_;

        double leftPos_;
        double rightPos_;
    };
}