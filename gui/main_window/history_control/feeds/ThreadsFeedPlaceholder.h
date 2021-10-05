#pragma once

namespace Ui
{
    class ThreadsFeedPlaceholder_p;
    class ThreadsFeedPlaceholder : public QWidget
    {
        Q_OBJECT
    public:
        ThreadsFeedPlaceholder(QWidget* _parent);
        ~ThreadsFeedPlaceholder();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        void updateGeometry();

        std::unique_ptr<ThreadsFeedPlaceholder_p> d;
    };
}
