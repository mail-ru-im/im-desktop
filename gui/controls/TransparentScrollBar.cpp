#include "stdafx.h"
#include "FlatMenu.h"
#include "TransparentScrollBar.h"
#include "../utils/utils.h"
#include "../utils/graphicsEffects.h"
#include "../utils/InterConnector.h"
#include "../styles/ThemeParameters.h"

#include "../main_window/contact_list/CustomAbstractListModel.h"

namespace
{
    constexpr auto layoutReschedInterval = std::chrono::milliseconds(100);

    constexpr std::chrono::milliseconds disappearTime = std::chrono::seconds(2);
    constexpr std::chrono::milliseconds wideTime = std::chrono::milliseconds(250);
    constexpr std::chrono::milliseconds thinTime = std::chrono::milliseconds(250);
    constexpr std::chrono::milliseconds fadeInTime = std::chrono::milliseconds(250);
    constexpr std::chrono::milliseconds fadeOutTime = std::chrono::milliseconds(250);

    constexpr auto minOpacity = 0.0;
    constexpr auto maxOpacity = 1.0;

    constexpr auto minScrollButtonWidth = 4;
    constexpr auto maxScrollButtonWidth = 4;
    constexpr auto minScrollButtonHeight = 50;

    constexpr auto backgroundWidth = 4;
    constexpr auto backgroundRightMargin = 4;
    constexpr auto backgroundUpMargin = 4;
    constexpr auto backgroundDownMargin = 4;

    constexpr auto is_show_with_small_content = false;
}

namespace Ui
{
    //////////////////////////////////////////////////////////////////////////
    // TransparentAnimation
    //////////////////////////////////////////////////////////////////////////
    TransparentAnimation::TransparentAnimation(float _minOpacity, float _maxOpacity, QWidget* _host, bool _autoFadeout)
        : QObject(_host)
        , minOpacity_(_minOpacity)
        , maxOpacity_(_maxOpacity)
        , host_(_host)
        , timer_(nullptr)
    {
        opacityEffect_ = new Utils::OpacityEffect(host_);
        opacityEffect_->setOpacity(minOpacity_);
        host_->setGraphicsEffect(opacityEffect_);

        fadeAnimation_ = new QPropertyAnimation(opacityEffect_, QByteArrayLiteral("opacity"), this);

        if (_autoFadeout)
        {
            timer_ = new QTimer(this);
            timer_->setSingleShot(true);
            timer_->setInterval(disappearTime.count());
            connect(timer_, &QTimer::timeout, this, &TransparentAnimation::fadeOut);

            timer_->start();
        }
    }

    TransparentAnimation::~TransparentAnimation()
    {
        delete opacityEffect_;
    }

    qreal TransparentAnimation::currentOpacity() const
    {
        return opacityEffect_->opacity();
    }

    void TransparentAnimation::setOpacity(const double _opacity)
    {
        opacityEffect_->setOpacity(_opacity);
    }

    void TransparentAnimation::fadeIn()
    {
        if (timer_)
            timer_->start(disappearTime.count());

        fadeAnimation_->stop();
        fadeAnimation_->setDuration(fadeInTime.count());
        fadeAnimation_->setStartValue(opacityEffect_->opacity());
        fadeAnimation_->setEndValue(maxOpacity_);
        fadeAnimation_->start();
    }

    void TransparentAnimation::fadeOut()
    {
        if (timer_)
            timer_->stop();

        emit fadeOutStarted();
        fadeAnimation_->stop();
        fadeAnimation_->setDuration(fadeOutTime.count());
        fadeAnimation_->setStartValue(opacityEffect_->opacity());
        fadeAnimation_->setEndValue(minOpacity_);
        fadeAnimation_->start();
    }








    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollButton
    //////////////////////////////////////////////////////////////////////////
    TransparentScrollButton::TransparentScrollButton(QWidget *parent)
        : QWidget(parent)
        , transparentAnimation_(new TransparentAnimation(minOpacity, maxOpacity, this))
        , isHovered_(false)
    {
        maxSizeAnimation_ = new QPropertyAnimation(this, QByteArrayLiteral("maximumWidth"), this);
        minSizeAnimation_ = new QPropertyAnimation(this, QByteArrayLiteral("minimumWidth"), this);

        connect(transparentAnimation_, &TransparentAnimation::fadeOutStarted, this, &TransparentScrollButton::fadeOutStarted);
    }

    TransparentScrollButton::~TransparentScrollButton()
    {
    }

    void TransparentScrollButton::mousePressEvent(QMouseEvent* /* event */)
    {
    }

    void TransparentScrollButton::mouseMoveEvent(QMouseEvent *event)
    {
        emit moved(event->globalPos());
    }

    void TransparentScrollButton::fadeIn()
    {
        if (isEnabled())
        {
            transparentAnimation_->fadeIn();
        }
    }

    void TransparentScrollButton::fadeOut()
    {
        transparentAnimation_->fadeOut();
    }

    void TransparentScrollButton::paintEvent(QPaintEvent* /* event */)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QPainterPath path;
        path.addRoundedRect(QRectF(0, 0, rect().width(), rect().height()), 0, 0);
        p.fillPath(path, isHovered_ ? Styling::Scrollbar::getScrollbarButtonHoveredColor() : Styling::Scrollbar::getScrollbarButtonColor());
    }

    void TransparentScrollButton::setHovered(const bool _hovered)
    {
        isHovered_ = _hovered;
    }




    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollButtonV
    //////////////////////////////////////////////////////////////////////////
    TransparentScrollButtonV::TransparentScrollButtonV(QWidget *parent)
        : TransparentScrollButton(parent)
        , minScrollButtonWidth_(Utils::scale_value(minScrollButtonWidth))
        , maxScrollButtonWidth_(Utils::scale_value(maxScrollButtonWidth))
        , minScrollButtonHeight_(Utils::scale_value(minScrollButtonHeight))
    {
        maxSizeAnimation_ = new QPropertyAnimation(this, QByteArrayLiteral("maximumWidth"), this);
        minSizeAnimation_ = new QPropertyAnimation(this, QByteArrayLiteral("minimumWidth"), this);

        setMaximumHeight(minScrollButtonHeight_);
        setMinimumHeight(minScrollButtonHeight_);
        setMaximumWidth(minScrollButtonWidth_);
        setMinimumWidth(minScrollButtonWidth_);
    }

    int TransparentScrollButtonV::getMinHeight()
    {
        return minScrollButtonHeight_;
    }

    int TransparentScrollButtonV::getMinWidth()
    {
        return minScrollButtonWidth_;
    }

    int TransparentScrollButtonV::getMaxWidth()
    {
        return maxScrollButtonWidth_;
    }

    void TransparentScrollButtonV::hoverOn()
    {
        setHovered(true);
        maxSizeAnimation_->stop();
        maxSizeAnimation_->setDuration(wideTime.count());
        maxSizeAnimation_->setStartValue(maximumWidth());
        maxSizeAnimation_->setEndValue(maxScrollButtonWidth_);
        maxSizeAnimation_->start();

        minSizeAnimation_->stop();
        minSizeAnimation_->setDuration(wideTime.count());
        minSizeAnimation_->setStartValue(maximumWidth());
        minSizeAnimation_->setEndValue(maxScrollButtonWidth_);
        minSizeAnimation_->start();
        update();
    }

    void TransparentScrollButtonV::hoverOff()
    {
        setHovered(false);
        maxSizeAnimation_->stop();
        maxSizeAnimation_->setDuration(thinTime.count());
        maxSizeAnimation_->setStartValue(maximumWidth());
        maxSizeAnimation_->setEndValue(minScrollButtonWidth_);
        maxSizeAnimation_->start();

        minSizeAnimation_->stop();
        minSizeAnimation_->setDuration(thinTime.count());
        minSizeAnimation_->setStartValue(maximumWidth());
        minSizeAnimation_->setEndValue(minScrollButtonWidth_);
        minSizeAnimation_->start();
        update();
    }





    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollButtonH
    //////////////////////////////////////////////////////////////////////////
    TransparentScrollButtonH::TransparentScrollButtonH(QWidget *parent)
        : TransparentScrollButton(parent)
        , minScrollButtonHeight_(Utils::scale_value(minScrollButtonWidth))
        , maxScrollButtonHeight_(Utils::scale_value(maxScrollButtonWidth))
        , minScrollButtonWidth_(Utils::scale_value(minScrollButtonHeight))
    {
        maxSizeAnimation_ = new QPropertyAnimation(this, QByteArrayLiteral("maximumHeight"), this);
        minSizeAnimation_ = new QPropertyAnimation(this, QByteArrayLiteral("minimumHeight"), this);

        setMaximumWidth(minScrollButtonWidth_);
        setMinimumWidth(minScrollButtonWidth_);
        setMaximumHeight(minScrollButtonHeight_);
        setMinimumHeight(minScrollButtonHeight_);
    }

    int TransparentScrollButtonH::getMinWidth()
    {
        return minScrollButtonWidth_;
    }

    int TransparentScrollButtonH::getMinHeight()
    {
        return minScrollButtonHeight_;
    }

    int TransparentScrollButtonH::getMaxHeight()
    {
        return maxScrollButtonHeight_;
    }

    void TransparentScrollButtonH::hoverOn()
    {
        setHovered(true);
        maxSizeAnimation_->stop();
        maxSizeAnimation_->setDuration(wideTime.count());
        maxSizeAnimation_->setStartValue(maximumHeight());
        maxSizeAnimation_->setEndValue(maxScrollButtonHeight_);
        maxSizeAnimation_->start();

        minSizeAnimation_->stop();
        minSizeAnimation_->setDuration(wideTime.count());
        minSizeAnimation_->setStartValue(maximumHeight());
        minSizeAnimation_->setEndValue(maxScrollButtonHeight_);
        minSizeAnimation_->start();
        update();
    }

    void TransparentScrollButtonH::hoverOff()
    {
        setHovered(true);
        maxSizeAnimation_->stop();
        maxSizeAnimation_->setDuration(thinTime.count());
        maxSizeAnimation_->setStartValue(maximumHeight());
        maxSizeAnimation_->setEndValue(minScrollButtonHeight_);
        maxSizeAnimation_->start();

        minSizeAnimation_->stop();
        minSizeAnimation_->setDuration(thinTime.count());
        minSizeAnimation_->setStartValue(maximumHeight());
        minSizeAnimation_->setEndValue(minScrollButtonHeight_);
        minSizeAnimation_->start();
        update();
    }



    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollBar
    //////////////////////////////////////////////////////////////////////////
    TransparentScrollBar::TransparentScrollBar()
        : QWidget(nullptr, Qt::FramelessWindowHint)
        , view_(nullptr)
        , transparentAnimation_(new TransparentAnimation(minOpacity, maxOpacity, this))
    {
        setAttribute(Qt::WA_TranslucentBackground);
        //    setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    QPointer<TransparentScrollButton> TransparentScrollBar::getScrollButton() const
    {
        return scrollButton_;
    }

    void TransparentScrollBar::setGetContentSizeFunc(std::function<QSize()> _getContentSize)
    {
        getContentSize_ = _getContentSize;
    }

    QScrollBar* TransparentScrollBar::getDefaultScrollBar() const
    {
        return scrollBar_;
    }

    void TransparentScrollBar::setScrollArea(QAbstractScrollArea* _view)
    {
        view_ = _view;
        setDefaultScrollBar(_view);

        setParent(view_);
        init();
    }

    void TransparentScrollBar::setParentWidget(QWidget* _view)
    {
        view_ = _view;

        setParent(view_);
        init();
    }

    void TransparentScrollBar::init()
    {
        scrollButton_ = createScrollButton(view_);
        resize(Utils::scale_value(backgroundWidth), 0);

        scrollButton_->installEventFilter(this);
        view_->installEventFilter(this);
        installEventFilter(this);

        connect(getDefaultScrollBar(), &QScrollBar::valueChanged, this, &TransparentScrollBar::updatePos);
        connect(scrollButton_, &TransparentScrollButton::moved, this, &TransparentScrollBar::onScrollBtnMoved);

        connect(scrollButton_, &TransparentScrollButton::fadeOutStarted, transparentAnimation_, &TransparentAnimation::fadeOut);
    }

    TransparentScrollBar::~TransparentScrollBar()
    {
        removeEventFilter(scrollButton_);
        removeEventFilter(view_);
        removeEventFilter(this);
    }

    void TransparentScrollBar::fadeIn()
    {
        const auto canShow =
            scrollBar_->minimum() != scrollBar_->maximum() &&
            scrollButton_ &&
            (is_show_with_small_content || calcScrollBarRatio() < 1);

        if (canShow)
        {
            show();
            scrollButton_->show();
            scrollButton_->fadeIn();
            transparentAnimation_->fadeIn();
        }
    }

    void TransparentScrollBar::fadeOut()
    {
        if (scrollButton_ && (is_show_with_small_content || calcScrollBarRatio() < 1))
        {
            scrollButton_->fadeOut();
            transparentAnimation_->fadeOut();
        }
    }

    void TransparentScrollBar::mousePressEvent(QMouseEvent *event)
    {
        moveToGlobalPos(event->globalPos());
    }


    bool TransparentScrollBar::eventFilter(QObject *obj, QEvent *event)
    {
        switch (event->type())
        {
        case QEvent::Enter:
            setMouseTracking(true);
            if (obj == view_)
            {
                if constexpr (!platform::is_apple())
                    fadeIn();
            }
            else if (obj == this)
            {
                scrollButton_->hoverOn();
            }
            else if (obj == scrollButton_)
            {
                scrollButton_->hoverOn();
            }
            break;

        case QEvent::Leave:
            if (obj == view_)
            {
                scrollButton_->fadeOut();
            }
            else if (obj == this)
            {
                scrollButton_->hoverOff();
            }
            else if (obj == scrollButton_)
            {
                scrollButton_->hoverOff();
            }
            break;

        case QEvent::Resize:
            if (obj == view_)
            {
                onResize(static_cast<QResizeEvent*>(event));
            }
            break;
        case QEvent::MouseMove:
            if constexpr (!platform::is_apple())
                fadeIn();
            break;
        default:
            ;
        }
        return QWidget::eventFilter( obj, event );
    }

    void TransparentScrollBar::onScrollBtnMoved(QPoint point)
    {
        moveToGlobalPos(point);
    }

    void TransparentScrollBar::updatePos()
    {
        updatePosition();
    }

    void TransparentScrollBar::paintEvent(QPaintEvent * /* event */)
    {
        QPainter p(this);
        p.fillRect(rect(), Styling::Scrollbar::getScrollbarBackgroundColor());
    }

    void TransparentScrollBar::resizeEvent(QResizeEvent * /* event */)
    {
        updatePos();
    }







    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollBarV
    //////////////////////////////////////////////////////////////////////////
    void TransparentScrollBarV::init()
    {
        TransparentScrollBar::init();
    }

    double TransparentScrollBarV::calcScrollBarRatio()
    {
        const auto contentSize = getContentSize_();

        const auto viewportHeight = view_->height();
        if (contentSize.height() < 0)
            return -1;

        return 1.0 * viewportHeight / contentSize.height();
    }

    double TransparentScrollBarV::calcButtonHeight()
    {
        auto scrollButton = qobject_cast<TransparentScrollButtonV*>(getScrollButton());

        return std::max((int)(calcScrollBarRatio() * view_->height()), scrollButton->getMinHeight());
    }

    double TransparentScrollBarV::calcButtonWidth()
    {
        auto scrollButton = qobject_cast<TransparentScrollButtonV*>(getScrollButton());

        return std::max((int)(calcScrollBarRatio() * view_->width()), scrollButton->getMinHeight());
    }

    void TransparentScrollBarV::updatePosition()
    {
        auto scrollButton = qobject_cast<TransparentScrollButtonV*>(getScrollButton());

        const auto ratio = calcScrollBarRatio();

        const auto scrollBar = getDefaultScrollBar();
        const auto max = scrollBar->maximum();
        if (max == 0)
        {
            scrollButton->hide();
            hide();
            return;
        }

        if (ratio < 0)
        {
            scrollButton->hide();
            hide();
            return;
        }

        if (!is_show_with_small_content && ratio >= 1)
        {
            scrollButton->hide();
            hide();
            return;
        }
        else
        {
            scrollButton->show();
            show();
            if (!scrollButton->isVisible())
                fadeIn();
        }

        scrollButton->setFixedSize(scrollButton->width(), calcButtonHeight());
        const auto x = pos().x() + (width() - scrollButton->getMaxWidth()) / 2;

        if (max == 0)
        {
            scrollButton->move(x, pos().y());
            return;
        }

        const auto val = scrollBar->value();
        const auto maxY = height() - scrollButton->height();
        const auto y = (maxY * val) / max + pos().y();

        scrollButton->move(x, y);
    }

    void TransparentScrollBarV::setDefaultScrollBar(QAbstractScrollArea* _view)
    {
        scrollBar_ = _view->verticalScrollBar();
    }

    QPointer<TransparentScrollButton> TransparentScrollBarV::createScrollButton(QWidget* _parent)
    {
        return QPointer<TransparentScrollButton>(new TransparentScrollButtonV(_parent));
    }

    void TransparentScrollBarV::onResize(QResizeEvent* _e)
    {
        const auto x = _e->size().width() - width() - Utils::scale_value(backgroundRightMargin);
        const auto y = Utils::scale_value(backgroundUpMargin);
        const auto w = width();
        const auto h = _e->size().height() - Utils::scale_value(backgroundDownMargin) - Utils::scale_value(backgroundUpMargin);

        move(x, y);
        resize(w, h);

        updatePos();
    }

    void TransparentScrollBarV::moveToGlobalPos(QPoint _moveTo)
    {
        auto scrollBar = getDefaultScrollBar();
        float max = scrollBar->maximum();
        QPoint viewGlobalPos = view_->parentWidget()->mapToGlobal(view_->pos());

        float minPx = viewGlobalPos.y();
        float maxPx = viewGlobalPos.y() + view_->height();

        // TODO (*) : use init relation position on scrollBar
        float ratio = (_moveTo.y() - minPx) / (maxPx - minPx - getScrollButton()->height());
        float value = ratio * max;

        scrollBar->setValue(value);
    }


    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollBarH
    //////////////////////////////////////////////////////////////////////////
    void TransparentScrollBarH::init()
    {
        TransparentScrollBar::init();
    }

    double TransparentScrollBarH::calcScrollBarRatio()
    {
        const auto contentSize = getContentSize_();

        const auto viewportWidth = view_->width();
        if (contentSize.width() < 0)
            return -1;

        return 1.0 * viewportWidth / contentSize.width();
    }

    double TransparentScrollBarH::calcButtonHeight()
    {
        auto scrollButton = qobject_cast<TransparentScrollButtonH*>(getScrollButton());

        return std::max((int)(calcScrollBarRatio() * view_->height()), scrollButton->getMinHeight());
    }

    double TransparentScrollBarH::calcButtonWidth()
    {
        auto scrollButton = qobject_cast<TransparentScrollButtonH*>(getScrollButton());

        return std::max((int)(calcScrollBarRatio() * view_->width()), scrollButton->getMinWidth());
    }

    void TransparentScrollBarH::updatePosition()
    {
        auto scrollButton = qobject_cast<TransparentScrollButtonH*>(getScrollButton());

        const auto ratio = calcScrollBarRatio();

        if (ratio < 0)
        {
            scrollButton->hide();
            hide();
            return;
        }

        if (!is_show_with_small_content && ratio >= 1)
        {
            scrollButton->hide();
            hide();
            return;
        }
        else
        {
            scrollButton->show();
            show();
            if (!scrollButton->isVisible())
            {
                fadeIn();
            }
        }

        const auto button_width = calcButtonWidth();

        scrollButton->setFixedSize(button_width, scrollButton->height());
        const auto scrollBar = getDefaultScrollBar();
        const auto val = scrollBar->value();
        const auto max = scrollBar->maximum();

        const auto y = pos().y() + (height() - scrollButton->getMaxHeight()) / 2;

        if (max == 0)
        {
            scrollButton->move(pos().x(), y);
            return;
        }

        const auto maxX = width() - scrollButton->width();
        const auto x = (maxX * val) / max + pos().x();

        scrollButton->move(x, y);
    }

    QPointer<TransparentScrollButton> TransparentScrollBarH::createScrollButton(QWidget* _parent)
    {
        return QPointer<TransparentScrollButton>(new TransparentScrollButtonH(_parent));
    }

    void TransparentScrollBarH::setDefaultScrollBar(QAbstractScrollArea* _view)
    {
        scrollBar_ = _view->horizontalScrollBar();
    }

    void TransparentScrollBarH::onResize(QResizeEvent* _e)
    {
        const auto x = Utils::scale_value(backgroundUpMargin);
        const auto y = _e->size().height() - height() - Utils::scale_value(backgroundRightMargin);
        const auto h = height();
        const auto w = _e->size().width() - 2* Utils::scale_value(backgroundDownMargin);

        move(x, y);
        resize(w, h);

        updatePos();
    }

    void TransparentScrollBarH::moveToGlobalPos(QPoint _moveTo)
    {
        auto scrollBar = getDefaultScrollBar();
        float max = scrollBar->maximum();
        QPoint viewGlobalPos = view_->parentWidget()->mapToGlobal(view_->pos());

        float minPx = viewGlobalPos.x();
        float maxPx = viewGlobalPos.x() + view_->width();

        // TODO (*) : use init relation position on scrollBar
        float ratio = (_moveTo.x() - minPx) / (maxPx - minPx - getScrollButton()->width());
        float value = ratio * max;

        scrollBar->setValue(value);
    }



    //////////////////////////////////////////////////////////////////////////
    // Abs widget with transp scrollbar
    //////////////////////////////////////////////////////////////////////////
    AbstractWidgetWithScrollBar::AbstractWidgetWithScrollBar()
        : scrollBarV_(nullptr)
        , scrollBarH_(nullptr)
    {

    }

    TransparentScrollBarV* AbstractWidgetWithScrollBar::getScrollBarV() const
    {
        return scrollBarV_;
    }

    void AbstractWidgetWithScrollBar::setScrollBarV(TransparentScrollBarV* _scrollBar)
    {
        scrollBarV_ = _scrollBar;
    }

    TransparentScrollBarH* AbstractWidgetWithScrollBar::getScrollBarH() const
    {
        return scrollBarH_;
    }

    void AbstractWidgetWithScrollBar::setScrollBarH(TransparentScrollBarH* _scrollBar)
    {
        scrollBarH_ = _scrollBar;
    }

    void AbstractWidgetWithScrollBar::mouseMoveEvent(QMouseEvent* /* event */)
    {
        if constexpr (!platform::is_apple())
            fadeIn();
    }

    void AbstractWidgetWithScrollBar::wheelEvent(QWheelEvent* /* event */)
    {
        fadeIn();
    }

    void AbstractWidgetWithScrollBar::fadeIn()
    {
        if (getScrollBarV())
        {
            getScrollBarV()->fadeIn();
        }

        if (getScrollBarH())
        {
            getScrollBarH()->fadeIn();
        }
    }

    void AbstractWidgetWithScrollBar::updateGeometries()
    {
        if (getScrollBarV())
        {
            getScrollBarV()->updatePos();
        }

        if (getScrollBarH())
        {
            getScrollBarH()->updatePos();
        }
    }

    // ListViewWithTrScrollBar

    ListViewWithTrScrollBar::ListViewWithTrScrollBar(QWidget *parent)
        : FocusableListView(parent)
    {
        setMouseTracking(true);
    }

    ListViewWithTrScrollBar::~ListViewWithTrScrollBar()
    {
    }

    QSize ListViewWithTrScrollBar::contentSize() const
    {
        return FocusableListView::contentsSize();
    }

    void ListViewWithTrScrollBar::mouseMoveEvent(QMouseEvent* event)
    {
        AbstractWidgetWithScrollBar::mouseMoveEvent(event);
        FocusableListView::mouseMoveEvent(event);
    }

    void ListViewWithTrScrollBar::wheelEvent(QWheelEvent *event)
    {
        AbstractWidgetWithScrollBar::wheelEvent(event);
        FocusableListView::wheelEvent(event);
    }

    void ListViewWithTrScrollBar::updateGeometries()
    {
        AbstractWidgetWithScrollBar::updateGeometries();
        FocusableListView::updateGeometries();
    }

    void ListViewWithTrScrollBar::setScrollBarV(TransparentScrollBarV* _scrollBar)
    {
        AbstractWidgetWithScrollBar::setScrollBarV(_scrollBar);
        getScrollBarV()->setScrollArea(this);
    }

    // MouseMoveEventFilter
    MouseMoveEventFilterForTrScrollBar::MouseMoveEventFilterForTrScrollBar(TransparentScrollBar* _scrollbar)
        : QObject(_scrollbar)
        , scrollbar_(_scrollbar)
    {
    }

    bool MouseMoveEventFilterForTrScrollBar::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::MouseMove && scrollbar_)
            scrollbar_->updatePos();

        return QObject::eventFilter(_obj, _event);
    }

    ScrollAreaWithTrScrollBar::ScrollAreaWithTrScrollBar(QWidget *parent)
        : QScrollArea(parent)
    {
        setMouseTracking(true);
        installEventFilter(this);
        setFocusPolicy(Qt::NoFocus);
    }

    ScrollAreaWithTrScrollBar::~ScrollAreaWithTrScrollBar()
    {
    }

    QSize ScrollAreaWithTrScrollBar::contentSize() const
    {
        if (const auto w = QScrollArea::widget())
            return w->size();
        return {};
    }

    void ScrollAreaWithTrScrollBar::mouseMoveEvent(QMouseEvent* event)
    {
        AbstractWidgetWithScrollBar::mouseMoveEvent(event);
        QScrollArea::mouseMoveEvent(event);
    }

    void ScrollAreaWithTrScrollBar::fadeIn()
    {
        AbstractWidgetWithScrollBar::fadeIn();
    }

    void ScrollAreaWithTrScrollBar::wheelEvent(QWheelEvent *event)
    {
        AbstractWidgetWithScrollBar::wheelEvent(event);
        QScrollArea::wheelEvent(event);
    }

    void ScrollAreaWithTrScrollBar::resizeEvent(QResizeEvent *event)
    {
        QScrollArea::resizeEvent(event);
        emit resized();
    }

    void ScrollAreaWithTrScrollBar::updateGeometry()
    {
        AbstractWidgetWithScrollBar::updateGeometries();
        QScrollArea::updateGeometry();
    }

    void ScrollAreaWithTrScrollBar::setScrollBarV(TransparentScrollBarV* _scrollBar)
    {
        AbstractWidgetWithScrollBar::setScrollBarV(_scrollBar);
        getScrollBarV()->setScrollArea(this);
    }

    void ScrollAreaWithTrScrollBar::setScrollBarH(TransparentScrollBarH* _scrollBar)
    {
        AbstractWidgetWithScrollBar::setScrollBarH(_scrollBar);
        getScrollBarH()->setScrollArea(this);
    }

    bool ScrollAreaWithTrScrollBar::eventFilter(QObject *obj, QEvent *event)
    {
        if (event->type() == QEvent::Resize)
        {
            if (getScrollBarV())
                getScrollBarV()->updatePos();

            if (getScrollBarH())
                getScrollBarH()->updatePos();
        }
        return QScrollArea::eventFilter( obj, event );
    }

    void ScrollAreaWithTrScrollBar::setWidget(QWidget* widget)
    {
        MouseMoveEventFilterForTrScrollBar* filter = nullptr;

        if (getScrollBarV())
            filter = new MouseMoveEventFilterForTrScrollBar(getScrollBarV());
        else if (getScrollBarH())
            filter = new MouseMoveEventFilterForTrScrollBar(getScrollBarH());

        widget->setMouseTracking(true);

        if (filter)
            widget->installEventFilter(filter);

        QScrollArea::setWidget(widget);
    }

    // TextEditExWithScrollBar

    TextEditExWithTrScrollBar::TextEditExWithTrScrollBar(QWidget *parent)
        : QTextBrowser(parent)
    {
        setMouseTracking(true);
    }

    TextEditExWithTrScrollBar::~TextEditExWithTrScrollBar()
    {
    }

    QSize TextEditExWithTrScrollBar::contentSize() const
    {
        QSize sizeRect(document()->size().width(), document()->size().height());
        return sizeRect;
    }

    void TextEditExWithTrScrollBar::mouseMoveEvent(QMouseEvent* event)
    {
        AbstractWidgetWithScrollBar::mouseMoveEvent(event);
        QTextBrowser::mouseMoveEvent(event);
    }

    void TextEditExWithTrScrollBar::wheelEvent(QWheelEvent *event)
    {
        AbstractWidgetWithScrollBar::wheelEvent(event);
        QTextBrowser::wheelEvent(event);
    }

    void TextEditExWithTrScrollBar::updateGeometry()
    {
        AbstractWidgetWithScrollBar::updateGeometries();
        QTextBrowser::updateGeometry();
    }

    void TextEditExWithTrScrollBar::setScrollBarV(TransparentScrollBarV* _scrollBar)
    {
        AbstractWidgetWithScrollBar::setScrollBarV(_scrollBar);
        getScrollBarV()->setScrollArea(this);
    }

    // HELPERS

    ScrollAreaWithTrScrollBar* CreateScrollAreaAndSetTrScrollBarV(QWidget* parent)
    {
        auto result = new ScrollAreaWithTrScrollBar(parent);
        result->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto scrollBar = new TransparentScrollBarV();
        result->setScrollBarV(scrollBar);
        scrollBar->setGetContentSizeFunc([result](){ return result->contentSize(); });

        return result;
    }

    ScrollAreaWithTrScrollBar* CreateScrollAreaAndSetTrScrollBarH(QWidget* parent)
    {
        auto result = new ScrollAreaWithTrScrollBar(parent);
        result->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto scrollBar = new TransparentScrollBarH();
        result->setScrollBarH(scrollBar);
        scrollBar->setGetContentSizeFunc([result]() { return result->contentSize(); });

        return result;
    }


    ListViewWithTrScrollBar* CreateFocusableViewAndSetTrScrollBar(QWidget* parent)
    {
        auto result = new ListViewWithTrScrollBar(parent);
        auto scrollBar = new TransparentScrollBarV();
        result->setScrollBarV(scrollBar);
        result->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        result->setFocusPolicy(Qt::NoFocus);
        scrollBar ->setGetContentSizeFunc([result](){ return result->contentSize(); });

        return result;
    }

    QTextBrowser* CreateTextEditExWithTrScrollBar(QWidget* parent)
    {
        auto result = new TextEditExWithTrScrollBar(parent);

        auto scrollBar = new TransparentScrollBarV();
        result->setScrollBarV(scrollBar );
        result->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollBar->setGetContentSizeFunc([result](){ return result->contentSize(); });

        return result;
    }

    // ListViewWithTrScrollBar

    FocusableListView::FocusableListView(QWidget *_parent)
        : QListView(_parent)
        , selectByMouseHoverEnabled_(false)
        , rescheduleLayoutTimer_(new QTimer(this))
    {
        rescheduleLayoutTimer_->setSingleShot(true);
        rescheduleLayoutTimer_->setInterval(layoutReschedInterval.count());

        connect(rescheduleLayoutTimer_, &QTimer::timeout, this, &FocusableListView::scheduleDelayedItemsLayout);
        connect(this, &QListView::pressed, this, Utils::QOverload<const QModelIndex&>::of(&QListView::update));
    }

    void FocusableListView::setSelectByMouseHover(const bool _enabled)
    {
        selectByMouseHoverEnabled_ = _enabled;
    }

    bool FocusableListView::getSelectByMouseHover() const
    {
        return selectByMouseHoverEnabled_;
    }

    void FocusableListView::updateSelectionUnderCursor()
    {
        updateSelectionUnderCursor(mapFromGlobal(QCursor::pos()));
    }

    void FocusableListView::enterEvent(QEvent *_e)
    {
        QListView::enterEvent(_e);
        if (platform::is_apple() && !FlatMenu::shown())
            emit Utils::InterConnector::instance().forceRefreshList(model(), true);
    }

    void FocusableListView::leaveEvent(QEvent *_e)
    {
        QListView::leaveEvent(_e);
        if (platform::is_apple() && !FlatMenu::shown())
            emit Utils::InterConnector::instance().forceRefreshList(model(), false);

        if (selectByMouseHoverEnabled_)
        {
            auto sm = selectionModel();
            const auto prevSelected = sm->currentIndex();

            {
                QSignalBlocker sb(sm);
                sm->clear();
            }

            if (prevSelected.isValid())
                update(prevSelected);
        }
    }

    void FocusableListView::mouseMoveEvent(QMouseEvent * _e)
    {
        QListView::mouseMoveEvent(_e);

        updateSelectionUnderCursor(_e->pos());
    }

    void FocusableListView::wheelEvent(QWheelEvent * _e)
    {
        QListView::wheelEvent(_e);

        updateSelectionUnderCursor(_e->pos());

        emit wheeled();
    }

    void FocusableListView::resizeEvent(QResizeEvent * e)
    {
        rescheduleLayoutTimer_->start();
        QListView::resizeEvent(e);
    }

    QItemSelectionModel::SelectionFlags FocusableListView::selectionCommand(const QModelIndex & index, const QEvent * event) const
    {
        QItemSelectionModel::SelectionFlags flags = QAbstractItemView::selectionCommand(index, event);

        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove)
            flags = QItemSelectionModel::NoUpdate;

        return flags;
    }

    void FocusableListView::updateSelectionUnderCursor(const QPoint& _pos)
    {
        const auto index = indexAt(_pos);

        if (selectByMouseHoverEnabled_)
            selectItem(index);
        updateCursor(index);

        emit mousePosChanged(_pos, index);
    }

    void FocusableListView::selectItem(const QModelIndex& _index)
    {
        auto sm = selectionModel();
        auto prevSelected = sm->currentIndex();

        if (prevSelected == _index)
            return;

        {
            QSignalBlocker sb(sm);

            if (_index.isValid())
                sm->setCurrentIndex(_index, QItemSelectionModel::ClearAndSelect);
            else
                sm->clear();
        }

        if (prevSelected.isValid())
            update(prevSelected);

        if (_index.isValid())
            update(_index);
    }

    void FocusableListView::updateCursor(const QModelIndex& _index)
    {
        auto showPointing = _index.isValid();
        if (showPointing)
        {
            if (const auto customModel = qobject_cast<Logic::CustomAbstractListModel*>(model()))
                showPointing = customModel->isClickableItem(_index);
        }

        const auto cur = showPointing ? Qt::PointingHandCursor : Qt::ArrowCursor;

        if (cursor().shape() != cur)
            setCursor(cur);
    }
}
