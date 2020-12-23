#pragma once
#include "TextUnit.h"

namespace Ui
{
    class TextTooltip;
    class CustomButton;
    class GradientWidget;
}

namespace Utils
{
    class OpacityEffect;
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

    void show(const QString& _text, const QRect& _objectRect, const QSize& _maxSize = QSize(0, 0), ArrowDirection _direction = ArrowDirection::Auto, Tooltip::ArrowPointPos _arrowPos = Tooltip::ArrowPointPos::Top, const QRect& _boundingRect = QRect());
    void forceShow(bool _force);
    void hide();

    QString text();
    bool isVisible();
    bool canWheel();
    void wheel(QWheelEvent* _e);
}

namespace Ui
{
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
        QRect updateTooltip(const QPoint _pos, const QSize& _maxSize, const QRect& _rect = QRect(), Tooltip::ArrowDirection _direction = Tooltip::ArrowDirection::Auto);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void wheelEvent(QWheelEvent* _e) override;
        void resizeEvent(QResizeEvent * _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;

    Q_SIGNALS:
        void scrolledToLastItem();
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
        Utils::OpacityEffect* opacityEffect_;
        QVariantAnimation* opacityAnimation_;
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

    Q_SIGNALS:
        void linkActivated(const QString& _link, QPrivateSignal) const;

    public:

        template <typename... Args>
        TextWidget(QWidget* _parent, Args&&... args)
            : QWidget(_parent)
        {
            setMouseTracking(true);
            text_ = TextRendering::MakeTextUnit(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void init(Args&&... args)
        {
            text_->init(std::forward<Args>(args)...);
            desiredWidth_ = text_->desiredWidth();
            text_->getHeight(maxWidth_ ? maxWidth_ : desiredWidth_);
            setFixedSize(text_->cachedSize());
            update();
        }

        void applyLinks(const std::map<QString, QString>& _links)
        {
            text_->applyLinks(_links);
        }

        void setLineSpacing(int _v)
        {
            text_->setLineSpacing(_v);
        }

        int getDesiredWidth() const noexcept { return desiredWidth_; }

        void setMaxWidth(int _width);
        void setMaxWidthAndResize(int _width);

        void setText(const QString& _text, const QColor& _color = QColor());
        void setOpacity(qreal _opacity);
        void setColor(const QColor& _color);
        void setAlignment(const TextRendering::HorAligment _align);

        QString getText() const;

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;

    private:
        TextRendering::TextUnitPtr text_;
        int maxWidth_ = 0;
        int desiredWidth_ = 0;
        qreal opacity_ = 1.0;
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
