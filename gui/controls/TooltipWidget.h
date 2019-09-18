#pragma once
#include "TextUnit.h"
#include "../animation/animation.h"

namespace Ui
{
    class TextTooltip;
}

namespace Tooltip
{
    enum class ArrowDirection
    {
        Auto,
        Up,
        Down,
        None
    };

    enum class ArrowPointPos
    {
        Top,
        Center,
        Bottom
    };

    int getCornerRadiusBig();
    int getArrowHeight();
    int getShadowSize();
    int getDefaultArrowOffset();
    int getMaxMentionTooltipHeight();
    int getMaxMentionTooltipWidth();
    int getMentionArrowOffset();
    void drawTooltip(QPainter& _p, const QRect& _tooltipRect, const int _arrowOffset, const ArrowDirection _direction);
    void drawBigTooltip(QPainter& _p, const QRect& _tooltipRect, const int _arrowOffset, const ArrowDirection _direction);

    Ui::TextTooltip* getDefaultTooltip();
    void resetDefaultTooltip();

    void show(const QString& _text, const QRect& _objectRect, const QSize& _maxSize = QSize(-1, -1), ArrowDirection _direction = ArrowDirection::Auto, Tooltip::ArrowPointPos _arrowPos = Tooltip::ArrowPointPos::Top);
    void forceShow(bool _force);
    void hide();

    QString text();
    bool isVisible();
    bool canWheel();
    void wheel(QWheelEvent* _e);
}

namespace Ui
{
    class CustomButton;

    class GradientWidget : public QWidget
    {
        Q_OBJECT

    public:
        GradientWidget(QWidget* _parent, const QColor& _left, const QColor& _right);
        void updateColors(const QColor& _left, const QColor& _right);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        QColor left_;
        QColor right_;
    };

    class TooltipWidget : public QWidget
    {
        Q_OBJECT

    public:
        TooltipWidget(QWidget* _parent, QWidget* _content, int _pointWidth, const QMargins& _margins, bool _canClose = true, bool _bigArrow = true, bool _horizontalScroll = false);

        void showAnimated(const QPoint _pos, const QSize& _maxSize, const QRect& _rect = QRect(), Tooltip::ArrowDirection _direction = Tooltip::ArrowDirection::Auto);
        void showUsual(const QPoint _pos, const QSize& _maxSize, const QRect& _rect = QRect(), Tooltip::ArrowDirection _direction = Tooltip::ArrowDirection::Auto);
        void hideAnimated();
        void setPointWidth(int _width);
        bool needToShow() const { return isScrollVisible() && isCursorOver_; }
        bool isScrollVisible() const;
        void wheel(QWheelEvent* _e);
        void scrollToTop();
        void ensureVisible(const int _x, const int _y, const int _xmargin, const int _ymargin);
        void setArrowVisible(const bool _visible);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void wheelEvent(QWheelEvent* _e) override;
        void resizeEvent(QResizeEvent * _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;

    private Q_SLOTS:
        void closeButtonClicked(bool);
        void onScroll(int _value);

    private:
        enum Direction
        {
            left = 0,
            right = 1
        };

        void scrollStep(Direction _direction);
        QMargins getMargins() const;

    private:
        QScrollArea* scrollArea_;
        QWidget* contentWidget_;
        Ui::CustomButton* buttonClose_;
        QPropertyAnimation* animScroll_;
        int arrowOffset_;
        int pointWidth_;
        GradientWidget* gradientRight_;
        GradientWidget* gradientLeft_;
        QGraphicsOpacityEffect* opacityEffect_;
        anim::Animation opacityAnimation_;
        bool canClose_;
        bool bigArrow_;
        bool isCursorOver_;
        bool horizontalScroll_;
        bool arrowInverted_;
        QMargins margins_;
        bool isArrowVisible_;
    };

    class TextWidget : public QWidget
    {
        Q_OBJECT

    public:

        template <typename... Args>
        TextWidget(QWidget* _parent, Args&&... args)
            : QWidget(_parent)
            , maxWidth_(0)
            , opacity_(1.0)
        {
            text_ = TextRendering::MakeTextUnit(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void init(Args&&... args)
        {
            text_->init(std::forward<Args>(args)...);
            text_->getHeight(maxWidth_ ? maxWidth_ : text_->desiredWidth());
            setFixedSize(text_->cachedSize());
            update();
        }

        void setMaxWidth(int _width);
        void setMaxWidthAndResize(int _width);

        void setText(const QString& _text, const QColor& _color = QColor());
        void setOpacity(qreal _opacity);

        QString getText() const;

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        TextRendering::TextUnitPtr text_;
        int maxWidth_;
        qreal opacity_;
    };

    class TextTooltip : public QWidget
    {
        Q_OBJECT

    public:
        TextTooltip(QWidget* _parent, bool _general = false);

        void setPointWidth(int _width);

        void showTooltip(
            const QString& _text,
            const QRect& _objectRect,
            const QSize& _maxSize,
            const QRect& _rect = QRect(),
            Tooltip::ArrowDirection _direction = Tooltip::ArrowDirection::Auto,
            Tooltip::ArrowPointPos _arrowPos = Tooltip::ArrowPointPos::Top);
        void hideTooltip(bool _force = false);

        QString getText() const;
        bool isTooltipVisible() const;

        bool canWheel();
        void wheel(QWheelEvent* _e);

        void setForceShow(bool _force);

    private Q_SLOTS:
        void check();

    private:
        TextWidget* text_;
        TooltipWidget* tooltip_;
        QTimer timer_;
        QString current_;
        QRect objectRect_;
        bool forceShow_;
    };
}
