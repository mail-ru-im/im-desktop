#include "stdafx.h"
#include "TooltipWidget.h"
#include "TextWidget.h"
#include "CustomButton.h"
#include "GradientWidget.h"
#include "../fonts.h"
#include "../utils/utils.h"
#include "../utils/features.h"
#include "../utils/InterConnector.h"
#include "../utils/graphicsEffects.h"
#include "core_dispatcher.h"
#include "../styles/ThemeParameters.h"
#include "../main_window/MainWindow.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "../main_window/containers/LastseenContainer.h"
#include "../main_window/containers/StatusContainer.h"
#include "../main_window/history_control/MessageStyle.h"
#include "../my_info.h"
#include "../styles/StyleSheetGenerator.h"
#include "../styles/StyleSheetContainer.h"
#include "UserMiniProfile.h"

namespace
{

    int mentionTooltipAvatarTopSpacing() noexcept
    {
        return Utils::scale_value(7);
    }

    int tooltipWidgetLeftMargin() noexcept
    {
        return Utils::scale_value(8);
    }

    int mentionTooltipHorMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    int mentionTooltipTopMargin() noexcept
    {
        return Utils::scale_value(8);
    }

    int mentionTooltipBottomMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getBackgroundColor()
    {
        return Styling::ColorParameter{ Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE } };
    }

    auto getLineColor()
    {
        return Styling::ColorParameter{ Styling::ThemeColorKey{ Styling::StyleVariable::GHOST_SECONDARY } };
    }

    int getGradientWidth() noexcept
    {
        return Utils::scale_value(20);
    }

    int getOutSpace() noexcept
    {
        return Utils::scale_value(8);
    }

    int getCloseButtonSize() noexcept
    {
        return Utils::scale_value(20);
    }

    int getMinArrowOffset(bool _big) noexcept
    {
        return _big ? Utils::scale_value(6) : Utils::scale_value(4);
    }

    int getCloseButtonOffset() noexcept
    {
        return Utils::scale_value(10);
    }

    int getMaxTextTooltipHeight() noexcept
    {
        return Utils::scale_value(260);
    }

    int getVerScrollbarWidth() noexcept
    {
        return Utils::scale_value(4);
    }

    int getInvertedOffset() noexcept
    {
        return Utils::scale_value(-4);
    }

    constexpr int tooltipInterval() noexcept
    {
        return 500;
    }

    int getMinWidth() noexcept
    {
        return Utils::scale_value(36);
    }

    constexpr int tooltipOffset() noexcept
    {
        return platform::is_apple() ? -1 : 0;
    }

    constexpr std::chrono::milliseconds getDurationAppear() noexcept { return std::chrono::milliseconds(200); }
    constexpr std::chrono::milliseconds getDurationDisappear() noexcept { return std::chrono::milliseconds(50); }

    Ui::TextWidget* createTooltipTextWidget(QWidget* _parent)
    {
        return new Ui::TextWidget(_parent, QString(), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS, Ui::TextRendering::EmojiSizeType::TOOLTIP);
    }

}

namespace Tooltip
{
    using namespace Ui;

    int getCornerRadiusBig() noexcept
    {
        return Utils::scale_value(6);
    }

    int getArrowHeight() noexcept
    {
        return Utils::scale_value(4);
    }

    int getShadowSize() noexcept
    {
        return Utils::scale_value(8);
    }

    int getDefaultArrowOffset() noexcept
    {
        return Utils::scale_value(20);
    }

    int getMaxMentionTooltipHeight() noexcept
    {
        return Utils::scale_value(240);
    }

    int getMaxMentionTooltipWidth()
    {
        return MessageStyle::getHistoryWidgetMaxWidth() + 2 * Tooltip::getShadowSize();
    }

    int getMentionArrowOffset() noexcept
    {
        return Utils::scale_value(10);
    }

    void drawTooltip(QPainter& _p, const QRect& _tooltipRect, const int _arrowOffset, const ArrowDirection _direction)
    {
        static const Utils::ColorParameterLayers layers =
        {
            { qsl("borderbg"), getBackgroundColor() },
            { qsl("border"), getLineColor() },
            { qsl("bg"), getBackgroundColor() },
        };
        static auto leftTop = Utils::LayeredPixmap(qsl(":/tooltip/corner"), layers);
        static QPixmap rightTop;
        static QPixmap rightBottom;
        static QPixmap leftBottom;

        if (leftTop.canUpdate() || rightTop.isNull())
        {
            rightTop = Utils::mirrorPixmapHor(leftTop.actualPixmap());
            rightBottom = Utils::mirrorPixmapVer(rightTop);
            leftBottom = Utils::mirrorPixmapVer(leftTop.cachedPixmap());
        }

        static auto topSide = Utils::LayeredPixmap(qsl(":/tooltip/side"), layers);
        static QPixmap rightSide;
        static QPixmap bottomSide;
        static QPixmap leftSide;

        if (topSide.canUpdate() || rightSide.isNull())
        {
            rightSide = topSide.actualPixmap().transformed(QTransform().rotate(90));
            bottomSide = Utils::mirrorPixmapVer(topSide.cachedPixmap());
            leftSide = topSide.cachedPixmap().transformed(QTransform().rotate(-90));
        }

        static auto arrowTop = Utils::LayeredPixmap(qsl(":/tooltip/tongue"), layers);
        static QPixmap arrowBottom;
        if (arrowTop.canUpdate() || arrowBottom.isNull())
            arrowBottom = Utils::mirrorPixmapVer(arrowTop.actualPixmap());

        im_assert(_direction != ArrowDirection::Auto);
        const auto& arrowP = _direction == ArrowDirection::Up ? arrowTop.cachedPixmap() : arrowBottom;

        const auto arrowH = _direction != ArrowDirection::None ? getArrowHeight() : 0;
        const auto rcBubble = _tooltipRect.adjusted(0, 0, 0, -arrowH);

        const auto leftTopPixmap = leftTop.cachedPixmap();
        const QRect fillRect(
            rcBubble.left() + Utils::unscale_bitmap(leftTopPixmap.width()),
            rcBubble.top() + Utils::unscale_bitmap(leftTopPixmap.height()),
            rcBubble.width() - Utils::unscale_bitmap(leftTopPixmap.width()) - Utils::unscale_bitmap(rightTop.width()),
            rcBubble.height() - Utils::unscale_bitmap(leftTopPixmap.height()) - Utils::unscale_bitmap(leftBottom.height()));

        Utils::PainterSaver ps(_p);
        _p.fillRect(fillRect, getBackgroundColor().color());

        // corners
        _p.drawPixmap(QRect(0, 0, Utils::unscale_bitmap(leftTopPixmap.width()), Utils::unscale_bitmap(leftTopPixmap.height())), leftTopPixmap);
        _p.drawPixmap(QRect(rcBubble.width() - Utils::unscale_bitmap(rightTop.width()), 0, Utils::unscale_bitmap(rightTop.width()), Utils::unscale_bitmap(rightTop.height())), rightTop);
        _p.drawPixmap(QRect(rcBubble.width() - Utils::unscale_bitmap(rightBottom.width()), rcBubble.height() - Utils::unscale_bitmap(rightBottom.height()),
            Utils::unscale_bitmap(rightBottom.width()), Utils::unscale_bitmap(rightBottom.height())), rightBottom);
        _p.drawPixmap(QRect(0, rcBubble.height() - Utils::unscale_bitmap(leftBottom.height()), Utils::unscale_bitmap(leftBottom.width()), Utils::unscale_bitmap(leftBottom.height())), leftBottom);

        // lines
        _p.drawPixmap(
            QRect(
                Utils::unscale_bitmap(leftTopPixmap.width()),
                0,
                rcBubble.width() - Utils::unscale_bitmap(leftTopPixmap.width()) - Utils::unscale_bitmap(rightTop.width()),
                Utils::unscale_bitmap(topSide.cachedPixmap().height())
            ), topSide.cachedPixmap());

        _p.drawPixmap(
            QRect(
                Utils::unscale_bitmap(leftTopPixmap.width()),
                rcBubble.height() - Utils::unscale_bitmap(bottomSide.height()),
                rcBubble.width() - Utils::unscale_bitmap(leftBottom.width()) - Utils::unscale_bitmap(rightBottom.width()),
                Utils::unscale_bitmap(bottomSide.height())
            ), bottomSide);

        _p.drawPixmap(QRect(rcBubble.width() - Utils::unscale_bitmap(rightSide.width()), Utils::unscale_bitmap(rightTop.height()), Utils::unscale_bitmap(rightSide.width()),
            rcBubble.height() - Utils::unscale_bitmap(rightTop.height()) - Utils::unscale_bitmap(rightBottom.height())), rightSide);
        _p.drawPixmap(QRect(0, Utils::unscale_bitmap(leftTopPixmap.height()), Utils::unscale_bitmap(leftSide.width()), rcBubble.height() - Utils::unscale_bitmap(leftTopPixmap.height()) - Utils::unscale_bitmap(leftBottom.height())), leftSide);

        // arrow
        if (_direction != ArrowDirection::None)
            _p.drawPixmap(QRect(_arrowOffset, _direction == ArrowDirection::Up ? 0 : (rcBubble.height() - Utils::unscale_bitmap(bottomSide.height())), Utils::unscale_bitmap(arrowP.width()), Utils::unscale_bitmap(arrowP.height())), arrowP);
    }

    void drawBigTooltip(QPainter& _p, const QRect& _tooltipRect, const int _arrowOffset, const ArrowDirection _direction)
    {
        Utils::PainterSaver ps(_p);
        _p.setRenderHint(QPainter::Antialiasing);
        _p.setPen(Qt::NoPen);

        const auto bgColorKey = Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE };
        const auto lineColorKey = Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT_ACTIVE };
        const auto bgColor = Styling::getColor(bgColorKey);
        const auto lineColor = Styling::getColor(lineColorKey);
        const auto radius = getCornerRadiusBig();
        const auto arrowH = _direction != ArrowDirection::None ? getArrowHeight() : 0;
        const auto bubbleRect = _tooltipRect.adjusted(
            Tooltip::getShadowSize(),
            Tooltip::getShadowSize() + (_direction == ArrowDirection::Down ? 0 : arrowH),
            -Tooltip::getShadowSize(),
            -Tooltip::getShadowSize() + (_direction == ArrowDirection::Down ? -arrowH : 0)
        );

        const auto lineWidth = Utils::scale_value(1);
        const auto outlineRect = bubbleRect.adjusted(-lineWidth, -lineWidth, lineWidth, lineWidth);
        _p.setBrush(bgColor);
        _p.drawRoundedRect(outlineRect, radius, radius);
        _p.setBrush(lineColor);
        _p.drawRoundedRect(outlineRect, radius, radius);

        _p.setBrush(bgColor);
        _p.drawRoundedRect(bubbleRect, radius, radius);

        if (_direction != ArrowDirection::None)
        {
            im_assert(_direction != ArrowDirection::Auto);

            static const Utils::ColorParameterLayers layers =
            {
                { qsl("border"), lineColorKey },
                { qsl("bg"), bgColorKey },
            };
            static auto arrowBottom = Utils::LayeredPixmap(qsl(":/tooltip/tongue_big"), layers);
            static auto arrowTop = Utils::mirrorPixmapVer(arrowBottom.cachedPixmap());
            if (arrowBottom.canUpdate())
                arrowTop = arrowBottom.actualPixmap();
            const auto x = _arrowOffset;
            const auto y = _direction == ArrowDirection::Down
                ? bubbleRect.bottom() + 1
                : bubbleRect.top() - arrowTop.height() / Utils::scale_bitmap_ratio();
            _p.drawPixmap(x, y, _direction == ArrowDirection::Down ? arrowBottom.cachedPixmap() : arrowTop);
        }
    }

    std::unique_ptr<TextTooltip> g_tooltip;
    std::unique_ptr<TextTooltip> ml_tooltip;
    std::unique_ptr<MentionTooltip> mn_tooltip;
    Tooltip::TooltipMode currentMode = Tooltip::TooltipMode::Default;

    MentionTooltip* getMentionTooltip(QWidget* _parent)
    {
        if (!mn_tooltip)
            mn_tooltip = std::make_unique<MentionTooltip>(_parent);

        return mn_tooltip.get();
    }

    void resetMentionTooltip()
    {
        mn_tooltip.reset();
    }

    TextTooltip* getDefaultTooltip(QWidget* _parent)
    {
        if (!g_tooltip)
            g_tooltip = std::make_unique<TextTooltip>(_parent, true);

        return g_tooltip.get();
    }

    void resetDefaultTooltip()
    {
        g_tooltip.reset();
    }

    void show(const Data::FString& _text, const QRect& _objectRect, const QSize& _maxSize, ArrowDirection _direction, Tooltip::ArrowPointPos _arrowPos, TextRendering::HorAligment _align, const QRect& _boundingRect, Tooltip::TooltipMode _mode, QWidget* _parent)
    {
        if (currentMode != _mode)
            hide();

        auto t = (_mode == Tooltip::TooltipMode::Default) ? getDefaultTooltip() : getDefaultMultilineTooltip();
        if (_parent != t->parentWidget())
        {
            if (_mode == Tooltip::TooltipMode::Default)
            {
                resetDefaultTooltip();
                t = getDefaultTooltip(_parent);
            }
            else
            {
                resetDefaultMultilineTooltip();
                t = getDefaultMultilineTooltip(_parent);
            }
        }

        t->setPointWidth(_objectRect.width());
        QRect r;
        if (QWidget* w = _parent ? _parent : Utils::InterConnector::instance().getMainWindow())
        {
            auto mwRect = QRect(w->mapToGlobal(w->rect().topLeft()), w->mapToGlobal(w->rect().bottomRight()));
            if (mwRect.contains(_objectRect))
                r = mwRect;
            else
                r = _boundingRect;
        }

        currentMode = _mode;
        t->showTooltip(_text, _objectRect, !_maxSize.isNull() ? _maxSize : QSize(-1, getMaxTextTooltipHeight()), r, _direction, _arrowPos, currentMode, _align);
    }

    void showMention(const QString& _aimId, const QRect& _objectRect, const QSize& _maxSize, ArrowDirection _direction, Tooltip::ArrowPointPos _arrowPos, const QRect& _boundingRect, Tooltip::TooltipMode _mode, QWidget* _parent)
    {
        auto t = getMentionTooltip();

        if (_parent != t->parentWidget())
        {
            resetMentionTooltip();
            t = getMentionTooltip(_parent);
        }

        t->setPointWidth(_objectRect.width());
        QRect r;
        if (QWidget* w = _parent ? _parent : Utils::InterConnector::instance().getMainWindow())
        {
            const auto mwRect = QRect(w->mapToGlobal(w->rect().topLeft()), w->mapToGlobal(w->rect().bottomRight()));
            if (mwRect.intersects(_objectRect))
                r = mwRect;
            else
                r = _boundingRect;
        }

        t->showTooltip(_aimId, _objectRect, !_maxSize.isNull() ? _maxSize : QSize(-1, getMaxTextTooltipHeight()), r, _direction, _arrowPos);
    }

    void forceShow(bool _force)
    {
        auto t = (currentMode == Tooltip::TooltipMode::Default) ? getDefaultTooltip() : getDefaultMultilineTooltip();
        t->setForceShow(_force);
    }

    void hide()
    {
        getDefaultTooltip()->hideTooltip(true);
        getDefaultMultilineTooltip()->hideTooltip(true);
        getMentionTooltip()->hideTooltip(true);
    }

    TextTooltip* getDefaultMultilineTooltip(QWidget* _parent)
    {
        if (!ml_tooltip)
            ml_tooltip = std::make_unique<TextTooltip>(_parent, true);

        return ml_tooltip.get();
    }

    void resetDefaultMultilineTooltip()
    {
        ml_tooltip.reset();
    }

    Data::FString text()
    {
        return getDefaultTooltip()->getText();
    }

    bool isVisible()
    {
        return getDefaultTooltip()->isTooltipVisible() || getDefaultMultilineTooltip()->isTooltipVisible();
    }

    bool canWheel()
    {
        return getDefaultTooltip()->canWheel();
    }

    void wheel(QWheelEvent* _e)
    {
        return getDefaultTooltip()->wheel(_e);
    }
}

namespace Ui
{
    TooltipWidget::TooltipWidget(QWidget* _parent, QWidget* _content, int _pointWidth, const QMargins& _margins, TooltipCompopents _components)
        : QWidget(_parent)
        , contentWidget_(_content)
        , buttonClose_(nullptr)
        , animScroll_(nullptr)
        , arrowOffset_(Tooltip::getDefaultArrowOffset())
        , pointWidth_(_pointWidth)
        , gradientRight_(new GradientWidget(this, Styling::ColorParameter{ Qt::transparent }, getBackgroundColor()))
        , gradientLeft_(new GradientWidget(this, getBackgroundColor(), Styling::ColorParameter{ Qt::transparent }))
        , opacityEffect_(new Utils::OpacityEffect(this))
        , opacityAnimation_(new QVariantAnimation(this))
        , components_(_components)
        , isCursorOver_(false)
        , arrowInverted_(false)
        , margins_(_margins)
        , isArrowVisible_(true)
    {
        Testing::setAccessibleName(this, qsl("AS GeneralTooltip"));

        scrollArea_ = new QScrollArea(this);
        scrollArea_->setStyleSheet(qsl("background: transparent;"));
        scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        contentWidget_->setParent(scrollArea_);
        Testing::setAccessibleName(contentWidget_, qsl("AS GeneralTooltip content"));
        scrollArea_->setWidget(contentWidget_);
        scrollArea_->setFocusPolicy(Qt::NoFocus);
        scrollArea_->setFrameShape(QFrame::NoFrame);
        Styling::setStyleSheet(scrollArea_->verticalScrollBar(), Styling::getParameters().getScrollBarQss(4, 0));

        auto margins = getMargins();
        scrollArea_->move(margins.left() + Tooltip::getShadowSize(), margins.top() + Tooltip::getShadowSize());

        Utils::grabTouchWidget(scrollArea_->viewport(), true);
        Utils::grabTouchWidget(contentWidget_);

        if (components_.testFlag(TooltipCompopent::CloseButton))
        {
            buttonClose_ = new CustomButton(this, qsl(":/controls/close_icon"), QSize(12, 12));
            Styling::Buttons::setButtonDefaultColors(buttonClose_);
            buttonClose_->setFixedSize(getCloseButtonSize(), getCloseButtonSize());
            QObject::connect(buttonClose_, &CustomButton::clicked, this, &TooltipWidget::closeButtonClicked);
        }

        setFocusPolicy(Qt::NoFocus);

        QObject::connect(scrollArea_->horizontalScrollBar(), &QScrollBar::valueChanged, this, &TooltipWidget::onScroll);

        opacityAnimation_->setStartValue(0.0);
        opacityAnimation_->setEndValue(1.0);
        connect(opacityAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            opacityEffect_->setOpacity(value.toDouble());
        });
        connect(opacityAnimation_, &QVariantAnimation::finished, this, [this]()
        {
            if (opacityAnimation_->direction() == QAbstractAnimation::Backward)
                hide();
        });

        setGraphicsEffect(opacityEffect_);
    }

    void TooltipWidget::closeButtonClicked(bool)
    {
        hideAnimated();
    }

    void TooltipWidget::paintEvent(QPaintEvent*)
    {
        const auto aDir = isArrowVisible_
            ? (arrowInverted_ ? Tooltip::ArrowDirection::Up : Tooltip::ArrowDirection::Down)
            : Tooltip::ArrowDirection::None;

        QPainter p(this);
        if (components_.testFlag(TooltipCompopent::BigArrow))
            drawBigTooltip(p, rect(), arrowOffset_, aDir);
        else
            drawTooltip(p, rect(), arrowOffset_, aDir);
    }

    void TooltipWidget::wheelEvent(QWheelEvent* _e)
    {
        if (!components_.testFlag(TooltipCompopent::HorizontalScroll))
            return;

        const int numDegrees = _e->delta() / 8;
        const int numSteps = numDegrees / 15;

        if (!numSteps || !numDegrees)
            return;

        if (numSteps > 0)
            scrollStep(Direction::left);
        else
            scrollStep(Direction::right);

        _e->accept();
    }

    void TooltipWidget::enterEvent(QEvent* _e)
    {
        isCursorOver_ = true;
        return QWidget::enterEvent(_e);
    }

    void TooltipWidget::leaveEvent(QEvent* _e)
    {
        isCursorOver_ = false;
        return QWidget::leaveEvent(_e);
    }

    void TooltipWidget::scrollStep(Direction _direction)
    {
        QRect viewRect = scrollArea_->viewport()->geometry();
        auto scrollbar = scrollArea_->horizontalScrollBar();

        const int maxVal = scrollbar->maximum();
        const int minVal = scrollbar->minimum();
        const int curVal = scrollbar->value();

        const int step = viewRect.width() / 2;

        int to = 0;

        if (_direction == TooltipWidget::Direction::right)
            to = std::min(curVal + step, maxVal);
        else
            to = std::max(curVal - step, minVal);

        if (!animScroll_)
        {
            constexpr auto duration = std::chrono::milliseconds(300);

            animScroll_ = new QPropertyAnimation(scrollbar, QByteArrayLiteral("value"), this);
            animScroll_->setEasingCurve(QEasingCurve::InQuad);
            animScroll_->setDuration(duration.count());
        }

        animScroll_->stop();
        animScroll_->setStartValue(curVal);
        animScroll_->setEndValue(to);
        animScroll_->start();
    }

    QMargins TooltipWidget::getMargins() const
    {
        return margins_;
    }

    bool TooltipWidget::isScrollVisible() const
    {
        return scrollArea_->verticalScrollBar()->isVisible() || scrollArea_->horizontalScrollBar()->isVisible();
    }

    void TooltipWidget::wheel(QWheelEvent* _e)
    {
        QApplication::sendEvent(scrollArea_->verticalScrollBar(), _e);
    }

    void TooltipWidget::scrollToTop()
    {
        scrollArea_->horizontalScrollBar()->setValue(0);
    }

    void TooltipWidget::ensureVisible(const int _x, const int _y, const int _xmargin, const int _ymargin)
    {
        scrollArea_->ensureVisible(_x, _y, _xmargin, _ymargin);
    }

    void TooltipWidget::setArrowVisible(const bool _visible)
    {
        isArrowVisible_ = _visible;
        update();
    }

    int TooltipWidget::getHeight() const
    {
        return contentWidget_->size().height();
    }

    QRect TooltipWidget::updateTooltip(const QPoint _pos, const QSize& _maxSize, const QRect& _rect, Tooltip::ArrowDirection _direction)
    {
        const int margin_left = getOutSpace();
        const int margin_right = getOutSpace();

        auto s = contentWidget_->size();
        int need_width = s.width();

        auto margins = getMargins();
        auto add_width = margins.left() + margins.right() + Tooltip::getShadowSize() * 2;
        if (components_.testFlag(TooltipCompopent::CloseButton))
            add_width += getCloseButtonOffset();

        need_width += add_width;

        int w = need_width;

        bool showGradient = false;

        if (_maxSize.width() != -1)
        {
            const int maxW = _maxSize.width() - margin_left - margin_right;
            if (need_width > maxW)
            {
                w = maxW;

                showGradient = true;
            }
        }

        auto addHeight = Tooltip::getArrowHeight() + margins.top() + margins.bottom() + Tooltip::getShadowSize() * 2;
        auto h = s.height() + addHeight;
        if (_maxSize.height() != -1 && h > _maxSize.height())
        {
            h = _maxSize.height();
            w += getVerScrollbarWidth();
        }

        w = std::max(w, getMinWidth());
        if (_rect.isValid())
            w = std::min(w, _rect.width());

        setFixedSize(w, h);
        scrollArea_->setFixedSize(w - add_width, h - addHeight);

        if (buttonClose_)
            buttonClose_->move(w - getCloseButtonSize() - Tooltip::getShadowSize(), Tooltip::getShadowSize());

        const int pointCenterX = _pos.x() + pointWidth_ / 2;

        int xPos = pointCenterX - w / 2;

        if (std::abs(xPos) < margin_left)
            xPos = margin_left;

        if (_maxSize.width() != -1 && w > _maxSize.width() - (2 * margin_right) - 1 && xPos + w > _maxSize.width() - margin_right)
            if (auto parent = parentWidget())
                xPos = std::abs(parent->width() - w) / 2;

        if (_rect.isValid())
            xPos = std::max(xPos, _rect.left());

        int yOffset = tooltipOffset();
        arrowInverted_ = false;
        if (_rect.isValid())
        {
            auto topR = _rect.topRight();
            const auto rightEdge = topR.x();
            if (xPos + w > rightEdge)
                xPos -= (xPos + w - rightEdge);

            if (_direction == Tooltip::ArrowDirection::Up || (_direction == Tooltip::ArrowDirection::Auto && (_pos.y() - height() < topR.y())))
            {
                yOffset = height();
                arrowInverted_ = true;
            }
        }

        arrowOffset_ = pointCenterX - xPos - getMinArrowOffset(components_.testFlag(TooltipCompopent::BigArrow));

        gradientRight_->setVisible(showGradient);

        if (showGradient)
        {
            const auto rcArea = scrollArea_->geometry();
            gradientRight_->setGeometry(rcArea.right() + 1 - getGradientWidth(), rcArea.top(), getGradientWidth(), rcArea.height());
            gradientRight_->raise();
        }

        gradientLeft_->setVisible(false);
        move(QPoint(xPos, _pos.y() - height() + yOffset));
        update();
        return geometry();
    }

    void TooltipWidget::showAnimated(const QPoint _pos, const QSize& _maxSize, const QRect& _rect, Tooltip::ArrowDirection _direction)
    {
        opacityEffect_->setOpacity(0.0);

        opacityAnimation_->stop();
        opacityAnimation_->setDirection(QAbstractAnimation::Forward);
        opacityAnimation_->setDuration(getDurationAppear().count());
        opacityAnimation_->start();

        showUsual(_pos, _maxSize, _rect, _direction);
    }

    void TooltipWidget::cancelAnimation()
    {
        opacityAnimation_->stop();
    }

    void TooltipWidget::showUsual(const QPoint _pos, const QSize& _maxSize, const QRect& _rect, Tooltip::ArrowDirection _direction)
    {
        opacityEffect_->setOpacity(1.0);
        opacityAnimation_->stop();

        const auto desired = updateTooltip(_pos, _maxSize, _rect, _direction);

        // when navigate pointing hand cursor with pressed mouse button on tooltip (like rewinding ptt) on macos
        // it change cursor from hand to arrow, so for disabling this behavior
        // make tooltip window transparent for mouse events when scroll hidden
        if (platform::is_apple())
            setWindowFlag(Qt::WindowTransparentForInput, !isScrollVisible()); // crutch for mac

        show();

        if (desired != geometry())
        {
            setGeometry(desired);
            update();
        }
    }

    void TooltipWidget::hideAnimated()
    {
        opacityAnimation_->stop();
        opacityAnimation_->setDirection(QAbstractAnimation::Backward);
        opacityAnimation_->setDuration(getDurationDisappear().count());
        opacityAnimation_->start();
    }

    void TooltipWidget::setPointWidth(int _width)
    {
        pointWidth_ = _width;
    }

    void TooltipWidget::resizeEvent(QResizeEvent * _e)
    {
        const auto rcArea = scrollArea_->geometry();

        gradientRight_->setGeometry(rcArea.right() + 1 - getGradientWidth(), rcArea.top(), getGradientWidth(), rcArea.height());
        gradientLeft_->setGeometry(rcArea.left(), rcArea.top(), getGradientWidth(), rcArea.height());

        QWidget::resizeEvent(_e);
    }

    void TooltipWidget::onScroll(int _value)
    {
        int maximum = scrollArea_->horizontalScrollBar()->maximum();
        const int step = scrollArea_->horizontalScrollBar()->pageStep() / 8;

        bool gradientLeftVisible = true;
        bool gradientRightVisible = true;

        if (_value == 0)
        {
            gradientLeftVisible = false;
        }
        else if (_value >= maximum - step)
        {
            Q_EMIT scrolledToLastItem();
            if (_value == maximum)
                gradientRightVisible = false;
        }

        if (gradientLeftVisible != gradientLeft_->isVisible())
        {
            gradientLeft_->setVisible(gradientLeftVisible);
            if (gradientLeftVisible)
                gradientLeft_->raise();
        }

        if (gradientRightVisible != gradientRight_->isVisible())
        {
            gradientRight_->setVisible(gradientRightVisible);
            if (gradientRightVisible)
                gradientRight_->raise();
        }
    }

    TextTooltip::TextTooltip(QWidget* _parent, bool _general)
        : QWidget(_parent)
        , tooltip_(nullptr)
        , timer_(new QTimer(this))
        , text_(createTooltipTextWidget(_parent))
        , forceShow_(false)
    {
        const auto textColor = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
        TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaledFixed(11), textColor };
        params.linkColor_ = textColor;
        text_->init(params);
        tooltip_ = new TooltipWidget(_parent, text_, 0, QMargins(Utils::scale_value(8), Utils::scale_value(4), Utils::scale_value(8), Utils::scale_value(4)));
        if (_general)
        {
            auto flag = platform::is_apple() ? Qt::SplashScreen : Qt::ToolTip;
            tooltip_->setWindowFlags(flag | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
            tooltip_->setAttribute(Qt::WA_TranslucentBackground);
            tooltip_->setAttribute(Qt::WA_NoSystemBackground);
        }

        timer_->setInterval(tooltipInterval());
        connect(timer_, &QTimer::timeout, this, &TextTooltip::check);
    }

    void TextTooltip::setPointWidth(int _width)
    {
        tooltip_->setPointWidth(_width);
    }

    void TextTooltip::showTooltip(const Data::FString& _text, const QRect& _objectRect, const QSize& _maxSize, const QRect& _rect, Tooltip::ArrowDirection _direction, Tooltip::ArrowPointPos _arrowPos, Tooltip::TooltipMode _mode, Ui::TextRendering::HorAligment align)
    {
        if (platform::is_apple())
            hideTooltip(true);

        current_ = _text;
        if (_mode == Tooltip::TooltipMode::Multiline)
            text_->setMaxWidth(0);
        text_->setAlignment(align);
        text_->setText(_text);
        const int textWidth = text_->getDesiredWidth();
        const auto& mltRect = _rect.isValid() ? _rect : _objectRect;
        if (_mode == Tooltip::TooltipMode::Multiline && textWidth >= mltRect.width() - 2 * getOutSpace())
            text_->setMaxWidthAndResize(mltRect.width() - 4 * getOutSpace());
        else
            text_->setMaxWidthAndResize(textWidth);

        QPoint p;
        switch (_arrowPos)
        {
        case Tooltip::ArrowPointPos::Top:
            p = _objectRect.topLeft();
            break;
        case Tooltip::ArrowPointPos::Center:
            p = QPoint(_objectRect.left(), _objectRect.top() + _objectRect.height() / 2);
            break;
        case Tooltip::ArrowPointPos::Bottom:
            p = _objectRect.bottomLeft();
            break;
        default:
            break;
        }

        if (isTooltipVisible())
            tooltip_->showUsual(p, _maxSize, _rect, _direction);
        else
            tooltip_->showAnimated(p, _maxSize, _rect, _direction);

        objectRect_ = _objectRect;
        timer_->start();
    }

    void TextTooltip::hideTooltip(bool _force)
    {
        if (_force)
            tooltip_->hide();
        else
            tooltip_->hideAnimated();
    }

    Data::FString TextTooltip::getText() const
    {
        return current_;
    }

    bool TextTooltip::isTooltipVisible() const
    {
        return tooltip_->isVisible();
    }

    bool TextTooltip::canWheel()
    {
        return isTooltipVisible() && tooltip_->isScrollVisible();
    }

    void TextTooltip::wheel(QWheelEvent* _e)
    {
        tooltip_->wheel(_e);
    }

    void TextTooltip::check()
    {
        if (objectRect_.contains(QCursor::pos()) || tooltip_->needToShow() || forceShow_)
            return;

        hideTooltip();
        timer_->stop();
        objectRect_ = {};
    }

    void TextTooltip::setForceShow(bool _force)
    {
        forceShow_ = _force;
    }

    MentionWidget::MentionWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        userMiniProfile_ = new UserMiniProfile(this, {}, static_cast<MiniProfileFlags>(MiniProfileFlag::isTooltip | MiniProfileFlag::isStandalone));

        QHBoxLayout* rootLayout = Utils::emptyHLayout(this);
        rootLayout->addWidget(userMiniProfile_);
        setContentsMargins(mentionTooltipHorMargin(), mentionTooltipTopMargin(), mentionTooltipHorMargin(), mentionTooltipBottomMargin());
    }

    void MentionWidget::setMaxWidthAndResize(int _width)
    {
        setFixedSize(_width, userMiniProfile_->height() + mentionTooltipTopMargin() + mentionTooltipBottomMargin());
        userMiniProfile_->updateSize(_width - 2 * mentionTooltipHorMargin());
    }

    void MentionWidget::setTooltipData(const QString& _aimid, const QString& _name, const QString& _underName, const QString& _description)
    {
        userMiniProfile_->init(_aimid, _name, _underName, _description, {});
        update();
    }

    int MentionWidget::getDesiredWidth() const noexcept
    {
        return userMiniProfile_->getDesiredWidth() + 2 * mentionTooltipHorMargin();
    }

    MentionTooltip::MentionTooltip(QWidget* _parent)
        : QWidget(_parent)
        , mentionWidget_(new MentionWidget(_parent))
        , tooltip_(new TooltipWidget(_parent, mentionWidget_, 0, QMargins(), TooltipCompopent::BigArrow))
        , timer_(new QTimer(this))
    {
        auto flag = platform::is_apple() ? Qt::SplashScreen : Qt::ToolTip;
        tooltip_->setWindowFlags(flag | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        tooltip_->setAttribute(Qt::WA_TranslucentBackground);
        tooltip_->setAttribute(Qt::WA_NoSystemBackground);

        timer_->setInterval(tooltipInterval());
        connect(timer_, &QTimer::timeout, this, &MentionTooltip::check);
        connect(GetDispatcher(), &Ui::core_dispatcher::idInfo, this, &MentionTooltip::onUserInfo);
    }

    void MentionTooltip::showTooltip(const QString& _aimId, const QRect& _objectRect, const QSize& _maxSize, const QRect& _rect, Tooltip::ArrowDirection _direction, Tooltip::ArrowPointPos _arrowPos)
    {
        maxSize_ = _maxSize;
        boundingRect_ = _rect;
        direction_ = _direction;
        arrowPos_ = _arrowPos;
        objectRect_ = _objectRect;
        seq_ = Ui::GetDispatcher()->getIdInfo(_aimId);
    }

    bool MentionTooltip::isTooltipVisible() const
    {
        return tooltip_->isVisible();
    }

    void MentionTooltip::onUserInfo(const qint64 _seq, const Data::IdInfo& _idInfo)
    {
        if (seq_ != _seq || !_idInfo.isValid())
            return;

        QString undernameText;
        if (!_idInfo.sn_.contains(u'@'))
            undernameText += u'@';

        if (_idInfo.nick_.isEmpty())
            undernameText += _idInfo.sn_;
        else
            undernameText += _idInfo.nick_;

        mentionWidget_->setTooltipData(_idInfo.sn_, _idInfo.getName(), undernameText, Features::isAppsNavigationBarVisible() ? _idInfo.description_ : QString());
        mentionWidget_->setMaxWidthAndResize(std::min(mentionWidget_->getDesiredWidth(), boundingRect_.width() - 2 * mentionTooltipHorMargin()));

        auto desktopRect = Utils::InterConnector::instance().getMainWindow()->availableVirtualGeometry();
        if (arrowPos_ == Tooltip::ArrowPointPos::Top && (objectRect_.topLeft().y() - tooltip_->getHeight()) < desktopRect.y())
        {
            arrowPos_ = Tooltip::ArrowPointPos::Bottom;
            direction_ = Tooltip::ArrowDirection::Up;
        }
        if (arrowPos_ == Tooltip::ArrowPointPos::Bottom && objectRect_.topLeft().y() + tooltip_->getHeight() > desktopRect.y() + desktopRect.height())
        {
            arrowPos_ = Tooltip::ArrowPointPos::Top;
            direction_ = Tooltip::ArrowDirection::Down;
        }

        QPoint p;
        switch (arrowPos_)
        {
        case Tooltip::ArrowPointPos::Top:
            p = objectRect_.topLeft();
            break;
        case Tooltip::ArrowPointPos::Center:
            p = QPoint(objectRect_.left(), objectRect_.top() + objectRect_.height() / 2);
            break;
        case Tooltip::ArrowPointPos::Bottom:
            p = objectRect_.bottomLeft();
            break;
        default:
            break;
        }

        if (isTooltipVisible())
            tooltip_->showUsual(p, maxSize_, boundingRect_, direction_);
        else
            tooltip_->showAnimated(p, maxSize_, boundingRect_, direction_);

        timer_->start();
    }

    void MentionTooltip::hideTooltip(bool _force)
    {
        if (_force)
            tooltip_->hide();
        else
            tooltip_->hideAnimated();
    }

    void MentionTooltip::check()
    {
        if (objectRect_.contains(QCursor::pos()))
            return;

        hideTooltip(true);
        timer_->stop();
        objectRect_ = {};
    }

    void MentionTooltip::setPointWidth(int _width)
    {
        tooltip_->setPointWidth(_width);
    }

}

