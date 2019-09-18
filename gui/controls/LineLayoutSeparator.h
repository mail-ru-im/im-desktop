#pragma once

namespace Ui
{
    class LineLayoutSeparator : public QWidget
    {
        Q_OBJECT

    public:
        enum class Direction
        {
            Ver,
            Hor
        };

        static QColor defaultColor();

        explicit LineLayoutSeparator(int _width, Direction _direction, const QColor& _color, QWidget* _parent = nullptr);
        explicit LineLayoutSeparator(int _width, Direction _direction, QWidget* _parent = nullptr);
        ~LineLayoutSeparator();

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        QColor color_;
    };
}