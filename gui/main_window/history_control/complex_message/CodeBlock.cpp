#include "stdafx.h"
#include "CodeBlock.h"

#include "ComplexMessageItem.h"
#include "controls/TransparentScrollBar.h"
#include "controls/TextWidget.h"
#include "controls/GradientWidget.h"
#include "controls/textrendering/FormattedTextRendering.h"
#include "utils/utils.h"
#include "fonts.h"
#include "../MessageStyle.h"

namespace
{
    auto titleTextFont() { return Fonts::adjustedAppFont(14, Fonts::FontWeight::Medium); }

    auto scrollAreaTopMargin() noexcept { return Utils::scale_value(4) + Ui::TextRendering::getPreBorderThickness(); }
    auto scrollAreaHorizontalMargin() noexcept { return Utils::scale_value(4) + Ui::TextRendering::getPreBorderThickness(); }
    auto scrollAreaBottomMargin() noexcept { return Utils::scale_value(2) + Ui::TextRendering::getPreBorderThickness(); }

    auto textTopMargin() noexcept { return Utils::scale_value(2); }
    auto textHorizontalMargin() noexcept { return Utils::scale_value(8); }
    auto textBottomMargin() noexcept { return Utils::scale_value(6); }

    auto scrollBarAreaHeight() noexcept { return Utils::scale_value(12); }

    auto gradientWidth() noexcept { return Utils::scale_value(48); }
    auto gradientBottomMargin() noexcept { return Utils::scale_value(6) + scrollAreaBottomMargin(); }

    std::chrono::milliseconds scrollAnimationDuration() noexcept { return std::chrono::milliseconds(250); }

    QColor borderColor() { return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY); }

    QColor mixColors(QColor _alphaColor, QColor _opaqueColor) noexcept
    {
        const auto alpha = _alphaColor.alphaF();
        const auto opacity = 1.0 - alpha;
        return QColor(_alphaColor.red() * alpha + _opaqueColor.red() * opacity,
            _alphaColor.green() * alpha + _opaqueColor.green() * opacity,
            _alphaColor.blue() * alpha + _opaqueColor.blue() * opacity,
            255);
    }

    QColor mixOpaqueColors(QColor _color1, QColor _color2) noexcept
    {
        return QColor(_color1.red() / 2 + _color2.red() / 2,
            _color1.green() / 2 + _color2.green() / 2,
            _color1.blue() / 2 + _color2.blue() / 2,
            255);
    }

    QColor backgroundColor(bool _outgoing, const QString& _aimId)
    {
        auto color = borderColor();
        color.setAlphaF(Ui::TextRendering::getPreBackgoundAlpha());
        return mixColors(color, Ui::MessageStyle::getBodyColor(_outgoing, _aimId));
    }

    Styling::ColorParameter gradientColor(bool _outgoing, const QString& _aimId)
    {
        return Styling::ColorParameter { mixOpaqueColors(backgroundColor(_outgoing, _aimId), Ui::MessageStyle::getBodyColor(_outgoing, _aimId)) };
    }

    QString backgroundStyleSheet(bool _outgoing, const QString& _aimId)
    {
        const auto borderWidth = Ui::TextRendering::getPreBorderThickness();
        const auto radius = Ui::TextRendering::getPreBorderRadius();
        return qsl("background-color: %1;border-width: %2; border-style: solid; border-radius: %3; border-color: %4;")
            .arg(backgroundColor(_outgoing, _aimId).name(QColor::NameFormat::HexArgb))
            .arg(borderWidth)
            .arg(radius)
            .arg(borderColor().name(QColor::NameFormat::HexArgb));
    }

    class QuoteOverlay : public QWidget
    {
        QuoteColorAnimation* quoteAnimation_;
    public:
        explicit QuoteOverlay(QWidget* _parent, QuoteColorAnimation* _quoteAnimation)
            : QWidget(_parent)
            , quoteAnimation_{ _quoteAnimation }
        {}
    protected:
        void paintEvent(QPaintEvent* _event) override
        {
            if (const auto color = quoteAnimation_->quoteColor(); color.isValid())
            {
                QPainter p(this);
                p.fillRect(rect(), color);
            }
        }
    };
} // namespace

namespace Ui::ComplexMessage
{
    CodeBlock::CodeBlock(ComplexMessageItem* _parent, Data::FStringView _text)
        : GenericBlock(_parent, _text, MenuFlags(MenuFlagCopyable), false)
        , themeChecker_{ getChatAimid() }
    {}

    Data::FString CodeBlock::getSelectedText(const bool _isFullSelect, const TextDestination _dest) const
    {
        if (!text_)
            return {};
        const auto textType = _dest == IItemBlock::TextDestination::quote ? TextRendering::TextType::SOURCE : TextRendering::TextType::VISIBLE;
        if (textType == TextRendering::TextType::SOURCE && text_->isAllSelected())
            return getSourceText();
        auto selectedText = text_->getSelectedText(textType);
        selectedText.clearFormat();
        selectedText.addFormat(core::data::format_type::pre);
        return selectedText;
    }

    void CodeBlock::updateWith(IItemBlock* _other)
    {
        auto otherCodeBlock = dynamic_cast<CodeBlock*>(_other);
        if (!otherCodeBlock)
            return;
        if (auto text = otherCodeBlock->getSourceText(); text != getSourceText())
        {
            setTextInternal(std::move(text));
            notifyBlockContentsChanged();
        }
    }

    bool CodeBlock::isSelected() const { return text_ && text_->isSelected(); }

    bool CodeBlock::isAllSelected() const { return text_ && text_->isAllSelected(); }

    void CodeBlock::selectByPos(const QPoint& _from, const QPoint& _to, bool /*_topToBottom*/)
    {
        if (!text_)
            return;

        const QPoint offset = textOffset();
        const QPoint blockFrom = mapFromGlobal(_from);
        const QPoint textFrom = blockFrom - offset;
        const QPoint textTo = mapFromGlobal(_to) - offset;
        if (pressStart_)
        {
            const auto cursorPoint = blockFrom == pressStart_->blockPoint_ ? textTo : textFrom;
            if (pressStart_->textUnitPoint_.y() < cursorPoint.y())
                text_->select(pressStart_->textUnitPoint_, cursorPoint);
            else
                text_->select(cursorPoint, pressStart_->textUnitPoint_);
        }
        else
        {
            text_->select(textFrom, textTo);
        }
    }

    void CodeBlock::selectAll()
    {
        if (text_)
            text_->selectAll();
    }

    void CodeBlock::clearSelection()
    {
        if (text_)
            text_->clearSelection();
    }

    void CodeBlock::releaseSelection()
    {
        if (text_)
            text_->releaseSelection();
    }

    int CodeBlock::desiredWidth(int _width) const
    {
        if (!text_)
            return GenericBlock::desiredWidth(_width);
        auto bubbleWidth = text_->getDesiredWidth();
        bubbleWidth += 2 * scrollAreaHorizontalMargin();
        bubbleWidth += 2 * textHorizontalMargin();
        return std::min(_width, bubbleWidth);
    }

    int CodeBlock::getMaxWidth() const { return Utils::scale_value(1030); }

    QRect CodeBlock::setBlockGeometry(const QRect& _rect)
    {
        if (!text_)
            return _rect;

        auto height = textContainer_->height();
        height += scrollAreaTopMargin();
        if (CodeBlock::desiredWidth(CodeBlock::getMaxWidth()) > _rect.width()) // need scrollbar
            height += scrollBarAreaHeight();
        height += scrollAreaBottomMargin();

        auto r = _rect;
        r.setHeight(height);
        setGeometry(r);
        return r;
    }

    QRect CodeBlock::getBlockGeometry() const { return geometry(); }

    void CodeBlock::doubleClicked(const QPoint& _p, std::function<void(bool)> _callback)
    {
        if (text_ && !text_->isAllSelected())
        {
            const auto mappedPoint = mapFromParent(_p, getBlockGeometry());
            text_->doubleClicked(mappedPoint, true, _callback);
            update();
        }
    }

    void CodeBlock::initialize()
    {
        im_assert(!text_);
        auto rootLayout = new QStackedLayout(this);
        rootLayout->setStackingMode(QStackedLayout::StackingMode::StackAll);

        text_ = new Ui::TextWidget(this,
            getSourceText(),
            Data::MentionMap {},
            TextRendering::LinksVisible::DONT_SHOW_LINKS,
            TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);
        updateTextFont();

        initializeScrollArea();
        rootLayout->addWidget(createQuoteOverlay());
        rootLayout->addWidget(createGradientWidget());
        rootLayout->addWidget(createScrollAreaContainer());
        rootLayout->addWidget(createBackgroundWidget());

        initializeScrollAnimation();

        if (isInsideQuote() || isInsideForward())
            setCursor(Qt::PointingHandCursor);
        QuoteAnimation_->setSemiTransparent();
    }

    void CodeBlock::updateFonts()
    {
        updateTextFont();
        notifyBlockContentsChanged();
    }

    void CodeBlock::wheelEvent(QWheelEvent* _event)
    {
        if (_event->modifiers() == Qt::ShiftModifier)
        {
            const auto scrollDelta = _event->angleDelta();
            const auto verticalDelta = scrollDelta.y();
            if (scrollDelta.x() == 0 && verticalDelta != 0 && (verticalDelta % Utils::defaultMouseWheelDelta() == 0))
            {
                _event->accept();

                setHorizontalScrollAnimationCurve(QEasingCurve::OutQuad);
                scrollAnimation_->setDuration(scrollAnimationDuration().count());
                smoothScroll(scrollArea_->horizontalScrollBar(),scrollAnimation_, verticalDelta);

                return;
            }
        }
        GenericBlock::wheelEvent(_event);
    }

    void CodeBlock::mouseMoveEvent(QMouseEvent* _event)
    {
        const auto isMultiSelect = Utils::InterConnector::instance().isMultiselect(getParentComplexMessage()->getContact());
        if ((_event->buttons() | Qt::MouseButton::LeftButton) && !isMultiSelect)
        {
            const auto x = _event->pos().x();
            auto scrollableWidget = scrollArea_->widget();
            const bool canScrollLeft = scrollableWidget->x() < 0;
            const bool canScrollRight = scrollableWidget->x() + scrollableWidget->width() > scrollArea_->viewport()->width();

            if (x > width() && canScrollRight)
                scrollDirection_ = ScrollDirection::Right;
            else if (x < 0 && canScrollLeft)
                scrollDirection_ = ScrollDirection::Left;
            else
                resetScrollAnimation();

            if ((ScrollDirection::None != scrollDirection_) && (scrollAnimation_->state() != QAbstractAnimation::Running))
            {
                auto scrollBar = scrollArea_->horizontalScrollBar();
                const auto start = scrollBar->value();
                const auto end = ScrollDirection::Left == scrollDirection_ ? scrollBar->minimum() : scrollBar->maximum();
                setHorizontalScrollAnimationCurve(QEasingCurve::Linear);
                scrollAnimation_->setStartValue(start);
                scrollAnimation_->setEndValue(end);
                scrollAnimation_->setDuration(std::abs(end - start) * 2);
                scrollAnimation_->start();
            }
        }
        GenericBlock::mouseMoveEvent(_event);
    }

    void CodeBlock::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::MouseButton::LeftButton)
            pressStart_ = { _event->pos() - textOffset(), _event->pos() };
        GenericBlock::mousePressEvent(_event);
    }

    void CodeBlock::mouseReleaseEvent(QMouseEvent* _event)
    {
        resetScrollAnimation();
        pressStart_ = {};
        GenericBlock::mouseReleaseEvent(_event);
    }

    bool CodeBlock::event(QEvent* _event)
    {
        if ((_event->type() == QEvent::UpdateLater) && themeChecker_.checkAndUpdateHash())
        {
            const ComplexMessageItem* messageItem = getParentComplexMessage();
            const auto color = gradientColor(messageItem->isOutgoingPosition(), messageItem->getContact());
            const auto transparent = Styling::ColorParameter{ Qt::transparent };
            leftGradient_->updateColors(color, transparent);
            rightGradient_->updateColors(transparent, color);
            backgroundWidget_->setStyleSheet(backgroundStyleSheet(messageItem->isOutgoingPosition(), messageItem->getContact()));
        }
        return GenericBlock::event(_event);
    }

    void CodeBlock::setTextInternal(Data::FString _text)
    {
        _text.clearFormat();
        _text.addFormat(core::data::format_type::monospace);
        text_->setText(_text);
    }

    void CodeBlock::updateTextFont()
    {
        TextRendering::TextUnit::InitializeParameters params { titleTextFont(), MessageStyle::getTextColorKey() };
        params.monospaceFont_ = MessageStyle::getTextMonospaceFont();
        params.linkColor_ = MessageStyle::getLinkColorKey();
        params.selectionColor_ = MessageStyle::getTextSelectionColorKey();
        params.highlightColor_ = MessageStyle::getHighlightColorKey();
        text_->init(params);

        setTextInternal(getSourceText());
    }

    void CodeBlock::initializeScrollArea()
    {
        textContainer_ = new QWidget;
        auto textLayout = Utils::emptyHLayout(textContainer_);
        const auto horizontalMargin = textHorizontalMargin();
        textLayout->setContentsMargins(horizontalMargin, textTopMargin(), horizontalMargin, textBottomMargin());
        textLayout->addWidget(text_);

        scrollArea_ = CreateScrollAreaAndSetTrScrollBarH(this, ScrollBarPolicy::AlwaysVisible);
        scrollArea_->setFocusPolicy(Qt::NoFocus);
        scrollArea_->setStyleSheet(qsl("background-color: transparent; border: none"));
        scrollArea_->setWidget(textContainer_);
        scrollArea_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        connect(scrollArea_, &ScrollAreaWithTrScrollBar::resized, this, &CodeBlock::updateGradientIfNeeded);

        auto scrollBar = scrollArea_->getScrollBarH();
        scrollBar->getScrollButton()->setCursor(Qt::PointingHandCursor);
        connect(scrollBar->getDefaultScrollBar(), &QScrollBar::valueChanged, this, &CodeBlock::updateGradientIfNeeded);
    }

    QWidget* CodeBlock::createScrollAreaContainer()
    {
        const auto margin = scrollAreaHorizontalMargin();
        auto scrollContainer = new QWidget;
        auto scrollLayout = Utils::emptyVLayout(scrollContainer);
        scrollLayout->setContentsMargins(margin, scrollAreaTopMargin(), margin, scrollAreaBottomMargin());
        scrollLayout->addWidget(scrollArea_);
        return scrollContainer;
    }

    QWidget* CodeBlock::createBackgroundWidget()
    {
        backgroundWidget_ = new QWidget;
        const ComplexMessageItem* messageItem = getParentComplexMessage();
        backgroundWidget_->setStyleSheet(backgroundStyleSheet(messageItem->isOutgoingPosition(), messageItem->getContact()));
        backgroundWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        return backgroundWidget_;
    }

    QWidget* CodeBlock::createGradientWidget()
    {
        auto gradientContainer = new QWidget;
        gradientContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        gradientContainer->setAttribute(Qt::WA_TransparentForMouseEvents);
        auto gradientLayout = Utils::emptyHLayout(gradientContainer);
        const auto topMargin = TextRendering::getPreBorderRadius();
        const auto horizontalMargin = TextRendering::getPreBorderThickness();
        const auto bottomMargin = gradientBottomMargin();
        gradientLayout->setContentsMargins(horizontalMargin, topMargin, horizontalMargin, bottomMargin);
        const ComplexMessageItem* messageItem = getParentComplexMessage();
        const auto color = gradientColor(messageItem->isOutgoingPosition(), messageItem->getContact());
        const auto transparent = Styling::ColorParameter{ Qt::transparent };
        leftGradient_ = new AnimatedGradientWidget(gradientContainer, color, transparent);
        leftGradient_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        leftGradient_->setFixedWidth(gradientWidth());
        rightGradient_ = new AnimatedGradientWidget(gradientContainer, transparent, color);
        rightGradient_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        rightGradient_->setFixedWidth(gradientWidth());

        gradientLayout->addWidget(leftGradient_);
        gradientLayout->addStretch(0);
        gradientLayout->addWidget(rightGradient_);

        return gradientContainer;
    }

    QWidget* CodeBlock::createQuoteOverlay()
    {
        auto overlay = new QuoteOverlay(this, QuoteAnimation_);
        overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
        return overlay;
    }

    void CodeBlock::initializeScrollAnimation()
    {
        scrollAnimation_ = new QVariantAnimation(this);
        connect(scrollAnimation_, &QVariantAnimation::valueChanged, this, &CodeBlock::onScrollAnimationValueChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &CodeBlock::onMultiselectChanged);
    }

    void CodeBlock::setHorizontalScrollAnimationCurve(QEasingCurve::Type _type)
    {
        if (scrollAnimation_->easingCurve().type() != _type)
            scrollAnimation_->setEasingCurve(QEasingCurve(_type));
    }

    QPoint CodeBlock::textOffset() const
    {
        const int margin = scrollAreaHorizontalMargin();
        const int scrollViewOffset = scrollArea_->widget()->x();
        const int textOffset = textHorizontalMargin();
        return { margin + scrollViewOffset + textOffset, margin + textTopMargin() };
    }

    void CodeBlock::resetScrollAnimation()
    {
        scrollDirection_ = ScrollDirection::None;
        scrollAnimation_->stop();
    }

    void CodeBlock::onScrollAnimationValueChanged()
    {
        const auto value = scrollAnimation_->currentValue().toInt();
        scrollArea_->horizontalScrollBar()->setValue(value);
    }

    void CodeBlock::onMultiselectChanged()
    {
        if (Utils::InterConnector::instance().isMultiselect(getParentComplexMessage()->getContact()))
            resetScrollAnimation();
    }

    void CodeBlock::updateGradientIfNeeded()
    {
        if (!scrollArea_||!leftGradient_)
            return;
        auto scrollBar = scrollArea_->getScrollBarH()->getDefaultScrollBar();
        const auto scrollValue = scrollBar->value();
        if (scrollBar->minimum() != scrollValue)
            leftGradient_->fadeIn();
        else
            leftGradient_->fadeOut();

        if (scrollBar->maximum() != scrollValue)
            rightGradient_->fadeIn();
        else
            rightGradient_->fadeOut();
    }
} // namespace Ui::ComplexMessage
