#pragma once

#include "TextUnit.h"
#include "../styles/StyleVariable.h"

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
        void clicked(QPrivateSignal) const;
        void clickedWithButton(ClickType _type, QPrivateSignal) const;
        void pressed(QPrivateSignal) const;
        void released(QPrivateSignal) const;
        void moved(QPrivateSignal) const;
        void hoverChanged(bool, QPrivateSignal) const;
        void pressChanged(bool, QPrivateSignal) const;
        void shown(QPrivateSignal) const;

    public:
        explicit ClickableWidget(QWidget* _parent);

        bool isHovered() const;
        void setHovered(const bool _isHovered);

        bool isPressed() const;
        void setPressed(const bool _isPressed);

        void click(const ClickType _how);

        void setFocusColor(const QColor& _color);
        void setHoverColor(const QColor& _color);
        void setBackgroundColor(const QColor& _color);

        virtual void setTooltipText(const QString& _text);
        virtual const QString& getTooltipText() const;

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
        void showEvent(QShowEvent* _e) override;

        bool focusNextPrevChild(bool _next) override;
        bool needShowTooltip() const;

        virtual void onTooltipTimer();
        virtual bool canShowTooltip();
        virtual void showToolTip();
        void hideToolTip();

    private:
        void animateFocusIn();
        void animateFocusOut();

    protected:
        QColor focusColor_;
        QColor hoverColor_;
        QColor bgColor_;
        QVariantAnimation* animFocus_;

        QTimer tooltipTimer_;
        bool enableTooltip_ = true;
        bool tooltipHasParent_ = false;
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
        ClickableTextWidget(QWidget* _parent, const QFont& _font, const Styling::ThemeColorKey& _color, const TextRendering::HorAligment _textAlign = TextRendering::HorAligment::LEFT);

        void setText(const Data::FString& _text);
        QSize sizeHint() const override;

        void setColor(const Styling::ThemeColorKey& _color);

        void setFont(const QFont& _font);
        void setLeftPadding(int _x);

        bool isElided() const { return text_ && text_->isElided(); };

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;

    private:
        void elideText();

        TextRendering::TextUnitPtr text_;
        int fullWidth_ = 0;
        int leftPadding_ = 0;
    };
}