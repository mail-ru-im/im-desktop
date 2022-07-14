#pragma once

namespace Ui
{

    class RubberBandWidget : public QRubberBand
    {
        Q_OBJECT
    public:
        RubberBandWidget(QWidget* _parent = nullptr);
    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void moveEvent(QMoveEvent* _event) override;
    };

    class WindowController : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
        Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth NOTIFY borderWidthChanged)
        Q_PROPERTY(Options options READ options WRITE setOptions NOTIFY optionsChanged)

    public:
        enum ResizeMode
        {
            ContinuousResize,
            OnDemandResize
        };
        Q_ENUM(ResizeMode)

            enum Option
        {
            None = 0x0,
            Resizing = 0x1,
            Dragging = 0x2,
            RubberBand = 0x4,
            RequireMouseEvents = 0x8
        };
        Q_DECLARE_FLAGS(Options, Option)
        Q_FLAG(Options)

        WindowController(QObject* _parent = nullptr);
        ~WindowController();

        void setWidget(QWidget* _target, QWidget* _watched = nullptr);
        QWidget* widget() const;

        void setEnabled(bool _on);
        bool isEnabled() const;

        void setOptions(Options _options);
        Options options() const;

        void setResizeMode(ResizeMode _mode);
        ResizeMode resizeMode() const;

        void setBorderWidth(int _width);
        int borderWidth() const;


    Q_SIGNALS:
        void borderWidthChanged(int, QPrivateSignal);
        void enabledChanged(bool, QPrivateSignal);
        void optionsChanged(Options, QPrivateSignal);

    protected:
        bool eventFilter(QObject* _watched, QEvent* _event) override;
        virtual bool mouseHover(QHoverEvent* _event);
        virtual bool mouseLeave(QEvent*);
        virtual bool mousePress(QMouseEvent* _event);
        virtual bool mouseRelease(QMouseEvent* _event);
        virtual bool mouseMove(QMouseEvent* _event);
        virtual void updateCursorShape(const QPoint& _pos);
        virtual QRubberBand* createRubberBand() const;
    private:
        std::unique_ptr<class WindowControllerPrivate> d;
    };
    Q_DECLARE_OPERATORS_FOR_FLAGS(WindowController::Options)

}

