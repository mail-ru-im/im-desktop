#pragma once
#include "TextUnit.h"
#include "ContactAvatarWidget.h"
#include "types/idinfo.h"

namespace Ui
{
    class MentionTooltip;
    class TextTooltip;
    class CustomButton;
    class GradientWidget;
    class TextWidget;
    class UserMiniProfile;
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

    enum class TooltipMode
    {
        Default,
        Multiline
    };

    int getCornerRadiusBig() noexcept;
    int getArrowHeight() noexcept;
    int getShadowSize() noexcept;
    int getDefaultArrowOffset() noexcept;
    int getMaxMentionTooltipHeight() noexcept;
    int getMaxMentionTooltipWidth();
    int getMentionArrowOffset() noexcept;
    void drawTooltip(QPainter& _p, const QRect& _tooltipRect, const int _arrowOffset, const ArrowDirection _direction);
    void drawBigTooltip(QPainter& _p, const QRect& _tooltipRect, const int _arrowOffset, const ArrowDirection _direction);

    Ui::MentionTooltip* getMentionTooltip(QWidget* _parent = nullptr);
    void resetMentionTooltip();

    Ui::TextTooltip* getDefaultTooltip(QWidget* _parent = nullptr);
    void resetDefaultTooltip();

    Ui::TextTooltip* getDefaultMultilineTooltip(QWidget* _parent = nullptr);
    void resetDefaultMultilineTooltip();

    void show(const Data::FString& _text, const QRect& _objectRect, const QSize& _maxSize = {}, ArrowDirection _direction = ArrowDirection::Auto, Tooltip::ArrowPointPos _arrowPos = Tooltip::ArrowPointPos::Top, Ui::TextRendering::HorAligment _align = Ui::TextRendering::HorAligment::LEFT, const QRect& _boundingRect = {}, Tooltip::TooltipMode _mode = Tooltip::TooltipMode::Default, QWidget* _parent = nullptr);
    void showMention(const QString& _aimId, const QRect& _objectRect, const QSize& _maxSize = {}, ArrowDirection _direction = ArrowDirection::Auto, Tooltip::ArrowPointPos _arrowPos = Tooltip::ArrowPointPos::Top, const QRect& _boundingRect = {}, Tooltip::TooltipMode _mode = Tooltip::TooltipMode::Default, QWidget* _parent = nullptr);
    void forceShow(bool _force);
    void hide();

    Data::FString text();
    bool isVisible();
    bool canWheel();
    void wheel(QWheelEvent* _e);

    using namespace std::chrono_literals;
    constexpr inline std::chrono::milliseconds getDefaultShowDelay() noexcept
    {
        return 400ms;
    }
}

namespace Ui
{
    enum class TooltipCompopent
    {
        None = 0,
        CloseButton = 1 << 1,
        BigArrow = 1 << 2,
        HorizontalScroll = 1 << 3,

        All = CloseButton | BigArrow | HorizontalScroll
    };
    Q_DECLARE_FLAGS(TooltipCompopents, TooltipCompopent)
    Q_DECLARE_OPERATORS_FOR_FLAGS(Ui::TooltipCompopents)

    class TooltipWidget : public QWidget
    {
        Q_OBJECT

    public:
        TooltipWidget(QWidget* _parent, QWidget* _content, int _pointWidth, const QMargins& _margins, TooltipCompopents _components = TooltipCompopent::None);

        void showAnimated(const QPoint _pos, const QSize& _maxSize, const QRect& _rect = {}, Tooltip::ArrowDirection _direction = Tooltip::ArrowDirection::Auto);
        void showUsual(const QPoint _pos, const QSize& _maxSize, const QRect& _rect = {}, Tooltip::ArrowDirection _direction = Tooltip::ArrowDirection::Auto);
        void hideAnimated();
        void cancelAnimation();
        void setPointWidth(int _width);
        bool needToShow() const { return isScrollVisible() && isCursorOver_; }
        bool isScrollVisible() const;
        void wheel(QWheelEvent* _e);
        void scrollToTop();
        void ensureVisible(const int _x, const int _y, const int _xmargin, const int _ymargin);
        void setArrowVisible(const bool _visible);
        int getHeight() const;
        QRect updateTooltip(const QPoint _pos, const QSize& _maxSize, const QRect& _rect = {}, Tooltip::ArrowDirection _direction = Tooltip::ArrowDirection::Auto);

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
        TooltipCompopents components_;
        bool isCursorOver_;
        bool arrowInverted_;
        QMargins margins_;
        bool isArrowVisible_;
    };

    class MentionWidget : public QWidget
    {
        Q_OBJECT

    public:
        MentionWidget(QWidget* _parent);

        void setTooltipData(const QString& _aimid, const QString& _name, const QString& _underName, const QString& _description);

        void setMaxWidthAndResize(int _width);

        int getDesiredWidth() const noexcept;

    private:
        UserMiniProfile* userMiniProfile_;
    };

    class TextTooltip : public QWidget
    {
        Q_OBJECT

    public:
        TextTooltip(QWidget* _parent, bool _general = false);

        void setPointWidth(int _width);

        void showTooltip(
            const Data::FString& _text,
            const QRect& _objectRect,
            const QSize& _maxSize,
            const QRect& _rect = {},
            Tooltip::ArrowDirection _direction = Tooltip::ArrowDirection::Auto,
            Tooltip::ArrowPointPos _arrowPos = Tooltip::ArrowPointPos::Top,
            Tooltip::TooltipMode _mode = Tooltip::TooltipMode::Default,
            Ui::TextRendering::HorAligment align = Ui::TextRendering::HorAligment::CENTER);
        void hideTooltip(bool _force = false);

        Data::FString getText() const;
        bool isTooltipVisible() const;

        bool canWheel();
        void wheel(QWheelEvent* _e);

        void setForceShow(bool _force);

    private Q_SLOTS:
        void check();

    private:
        TooltipWidget* tooltip_;
        Data::FString current_;
        QRect objectRect_;
        QTimer* timer_;
        TextWidget* text_;
        bool forceShow_;
    };

    class MentionTooltip : public QWidget
    {
        Q_OBJECT

    public:
        MentionTooltip(QWidget* _parent);

        void setPointWidth(int _width);

        void showTooltip(
            const QString& _aimId,
            const QRect& _objectRect,
            const QSize& _maxSize,
            const QRect& _rect = {},
            Tooltip::ArrowDirection _direction = Tooltip::ArrowDirection::Auto,
            Tooltip::ArrowPointPos _arrowPos = Tooltip::ArrowPointPos::Top);

        void hideTooltip(bool _force = false);
        bool isTooltipVisible() const;

    private Q_SLOTS:
        void check();
        void onUserInfo(const qint64 _seq, const Data::IdInfo& _idInfo);

    private:
        MentionWidget* mentionWidget_;
        TooltipWidget* tooltip_;
        QTimer* timer_;
        QRect objectRect_;

        QRect boundingRect_;
        QSize maxSize_;
        Tooltip::ArrowDirection direction_;
        Tooltip::ArrowPointPos arrowPos_;

        int64_t seq_;
    };
}
