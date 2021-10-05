#pragma once

namespace Ui
{
    class RotatingSpinner;
    class CustomButton;

    class LoaderSpinner : public QWidget
    {
        Q_OBJECT

    public:
        enum Option : uint16_t
        {
            NoOptions = 0x0,
            Rounded = 0x1,
            Shadowed = 0x2,
            Cancelable = 0x4,

            DefaultOptions = Rounded | Shadowed | Cancelable
        };
        Q_DECLARE_FLAGS(Options, Option);

    public:
        LoaderSpinner(QWidget* _parent = nullptr, const QSize& _size = QSize(), Options _opt = Option::DefaultOptions);
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
        Options options_;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(LoaderSpinner::Options);
}
