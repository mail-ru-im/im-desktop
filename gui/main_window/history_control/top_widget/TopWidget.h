#pragma once

namespace Ui
{
    class TopWidget : public QStackedWidget
    {
        Q_OBJECT

    public:
        explicit TopWidget(QWidget* _parent);

        enum Widgets
        {
            Main = 0,
            Selection
        };

        void updateStyle();

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        int lastIndex_ = 0;

        QColor bg_;
        QColor border_;
    };
}