#pragma once

namespace Ui
{
    class RotatingSpinner;
    class CustomButton;

    class LoaderSpinner : public QWidget
    {
        Q_OBJECT

    public:
        LoaderSpinner(QWidget* _parent = nullptr, const QSize& _size = QSize(), bool _rounded = true, bool _shadow = true);
        ~LoaderSpinner();

        void startAnimation(const QColor& _spinnerColor = QColor(), const QColor& _bgColor = QColor());
        void stopAnimation();

    Q_SIGNALS:
        void clicked();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        RotatingSpinner* spinner_;
        CustomButton* button_;
        QSize size_;
        bool rounded_;
        bool shadow_;
    };
}
