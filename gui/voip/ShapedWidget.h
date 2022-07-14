#pragma once

namespace Ui
{
    // class ShapedWidget is a special widget that masks
    // out it areas that not covered by direct children
    // widgets.
    // This approach allow to use ShapedWidget inside
    // complex QStackedLayout without worring about
    // dispatching and forwarding events(mostly keyboard
    // and mouse events). We don't need to take special
    // care of tracking adding/removing and hiding/showing
    // events on child widgets since that functionality is
    // already implemented in that class.
    // Also tracking of moving/resizing of child widgets are
    // supported through combination of MaskingOption flags.
    class ShapedWidget : public QFrame
    {
        Q_OBJECT
    public:
        enum MaskingOption
        {
            NoMasking = 0x0,
            TrackChildren = 0x1, // Track child widgets moving/resizing
            IgnoreMasks = 0x2, // Ignore masks installed on child widgets
            IgnoreFrame = 0x4  // Ignore frame border mask
        };
        Q_DECLARE_FLAGS(MaskingOptions, MaskingOption)
        Q_FLAG(MaskingOptions)

        explicit ShapedWidget(QWidget* _parent = nullptr, Qt::WindowFlags _flags = Qt::WindowFlags{});
        ~ShapedWidget();

        void setOptions(MaskingOptions _options);
        MaskingOptions options() const;

    protected:
        bool eventFilter(QObject* _watched, QEvent* _event) override;

        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent* _event) override;
        void moveEvent(QMoveEvent* _event) override;
        void childEvent(QChildEvent* _event) override;

        virtual void updateMask();

    private:
        std::unique_ptr<class ShapedWidgetPrivate> d;
    };
    Q_DECLARE_OPERATORS_FOR_FLAGS(ShapedWidget::MaskingOptions)
}
