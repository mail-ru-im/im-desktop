#pragma once

namespace Ui
{
    class FlatMenuStyle: public QProxyStyle
    {
        Q_OBJECT

    public:
        int pixelMetric(PixelMetric _metric, const QStyleOption* _option = nullptr, const QWidget* _widget = nullptr) const override;
    };

    class FlatMenu : public QMenu
    {
    private:
        Qt::Alignment expandDirection_;
        bool iconSticked_;
        bool needAdjustSize_;

        void adjustSize();

        static int shown_;

    protected:
        void showEvent(QShowEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;

    public:
        explicit FlatMenu(QWidget* _parent = nullptr);
        ~FlatMenu();

        void setExpandDirection(Qt::Alignment _direction);
        void stickToIcon();

        void setNeedAdjustSize(bool _value);

        static constexpr auto sourceTextPropName() noexcept { return "sourceTextPropName"; }

        static int shown()
        {
            return FlatMenu::shown_;
        }

        inline Qt::Alignment expandDirection() const
        {
            return expandDirection_;
        }
    };
}