#pragma once

#include "TextUnit.h"
#include "animation/animation.h"

namespace Styling
{
    enum class StyleVariable;
}

namespace Ui
{
    enum class ClickType
    {
        ByMouse,
        ByKeyboard,
    };

    class ClickableWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void clicked(const ClickType _type, QPrivateSignal) const;
        void pressed(QPrivateSignal) const;
        void released(QPrivateSignal) const;
        void hoverChanged(bool, QPrivateSignal) const;
        void pressChanged(bool, QPrivateSignal) const;

    public:
        explicit ClickableWidget(QWidget* _parent);

        bool isHovered() const;
        void setHovered(const bool _isHovered);

        bool isPressed() const;

        void click(const ClickType _how);

        void setFocusColor(const QColor& _color);

        void setTooltipText(const QString& _text);
        const QString& getTooltipText() const;

        void overrideNextInFocusChain(QWidget* _next);
        QWidget* nextInFocusChain() const;

    protected:
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void paintEvent(QPaintEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void focusInEvent(QFocusEvent* _event) override;
        void focusOutEvent(QFocusEvent* _event) override;

        bool focusNextPrevChild(bool _next) override;

        virtual void onTooltipTimer();
        virtual bool canShowTooltip();
        virtual void showToolTip();
        void hideToolTip();

    private:
        void animateFocusIn();
        void animateFocusOut();

    protected:
        QColor focusColor_;
        anim::Animation animFocus_;

        QTimer tooltipTimer_;
        bool enableTooltip_ = true;
        QString tooltipText_;

        QWidget* focusChainNextOverride_ = nullptr;

    private:
        bool isHovered_ = false;
        bool isPressed_ = false;
    };

    class ClickableTextWidget : public ClickableWidget
    {
        Q_OBJECT
    public:
        ClickableTextWidget(QWidget* _parent, const QFont& _font, const QColor& _color, const TextRendering::HorAligment _textAlign = TextRendering::HorAligment::LEFT);
        ClickableTextWidget(QWidget* _parent, const QFont& _font, const Styling::StyleVariable _color, const TextRendering::HorAligment _textAlign = TextRendering::HorAligment::LEFT);

        void setText(const QString& _text);
        QSize sizeHint() const override;

        void setColor(const QColor& _color);
        void setColor(const Styling::StyleVariable _color);

        void setFont(const QFont& _font);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;

    private:
        void elideText();

        TextRendering::TextUnitPtr text_;
        int fullWidth_;
    };
}