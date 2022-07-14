#include "stdafx.h"

#include "SlideController.h"

namespace Utils
{

class SlideControllerPrivate
{
public:
    QPixmap frames_[2];
    QPixmap backbuffer_;

    QLabel* overlay_;

    QCursor cursor_;
    QPointer<QStackedWidget> widget_;
    QTimeLine* timeLine_;
    QColor fillColor_;
    SlideController::SlideEffects effectFlags_;
    SlideController::SlideDirection slideDir_;
    SlideController::Fading fading_;
    SlideController::CachingPolicy caching_;
    double opacity_;
    int previousIndex_;
    bool inverse_;
    bool inversed_;
    bool enabled_;

    SlideControllerPrivate(SlideController* _q);

    SlideController::SlideDirection currentDirection() const;
    void renderFrame(int _frame, double _value, QPainter& _painter) const;

    void zoomFrame(QTransform& _transform, const QRectF& _rect, qreal _value, int _frame) const;
    void swapFrame(QTransform& _transform, const QRectF& _rect, qreal _value, int _frame) const;
    void rollFrame(QTransform& _transform, const QRectF& _rect, qreal _value, int _frame) const;
    void rotateFrame(QTransform& _transform, const QRectF& _rect, qreal _value, int _frame) const;
    void shearFrame(QTransform& _transform, const QRectF& _rect, qreal _value, int _frame) const;
    void clearCache();
};


SlideControllerPrivate::SlideControllerPrivate(SlideController* _q)
    : overlay_(new QLabel())
    , widget_(nullptr)
    , timeLine_(new QTimeLine(900, _q))
    , fillColor_(Qt::transparent)
    , effectFlags_(SlideController::SlideEffect::NoEffect)
    , slideDir_(SlideController::SlideDirection::NoSliding)
    , fading_(SlideController::Fading::NoFade)
    , caching_(SlideController::CachingPolicy::CacheNone)
    , opacity_(0.0)
    , previousIndex_(0)
    , inverse_(false)
    , inversed_(false)
    , enabled_(true)
{
    overlay_->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlay_->hide();
    timeLine_->setUpdateInterval(24);
}

void SlideControllerPrivate::zoomFrame(QTransform& _transform, const QRectF& _rect, qreal _value, int) const
{
    _transform.scale(1.0 - _value, 1.0 - _value);
}

void SlideControllerPrivate::swapFrame(QTransform& _transform, const QRectF& _rect, qreal _value, int _frame) const
{
    int sign = (inversed_ ? 1 : -1);
    _value = _frame == 0 ? (1.0 - _value) : _value;
    switch (currentDirection()) {
    case SlideController::SlideDirection::SlideUp:
        _transform.translate(0, sign * _rect.height() * _value);
        break;
    case SlideController::SlideDirection::SlideDown:
        _transform.translate(0, -sign * _rect.height() * _value);
        break;
    case SlideController::SlideDirection::SlideLeft:
        _transform.translate(sign * _rect.width() * _value, 0);
        break;
    case SlideController::SlideDirection::SlideRight:
        _transform.translate(-sign * _rect.width() * _value, 0);
        break;
    case SlideController::SlideDirection::NoSliding:
        break;
    }
}

void SlideControllerPrivate::rollFrame(QTransform& _transform, const QRectF& _rect, qreal _value, int _frame) const
{
    int dx = _rect.width() * !!(_frame == 0);
    int dy = _rect.height() * !!(_frame == 0);
    double y = 0.0;
    double x = 0.0;
    switch (currentDirection())
    {
    case SlideController::SlideDirection::SlideUp:
        y = (dy - _rect.height() * _value);
        break;
    case SlideController::SlideDirection::SlideDown:
        y = (_rect.height() * _value - dy);
        break;
    case SlideController::SlideDirection::SlideLeft:
        x = (dx - _rect.width() * _value);
        break;
    case SlideController::SlideDirection::SlideRight:
        x = (_rect.width() * _value - dx);
        break;
    case SlideController::SlideDirection::NoSliding:
        break;
    }
    _transform.translate(x, y);
}

void SlideControllerPrivate::rotateFrame(QTransform& _transform, const QRectF&, qreal _value, int _frame) const
{
    if (_frame == 1 && inversed_)
    {
        _transform.rotate(_value * 90, Qt::YAxis);
        _transform.scale(1.0 - _value, 1.0);
    }
    if (_frame == 0 && !inversed_)
    {
        _transform.rotate(_value * 90, Qt::YAxis);
        _transform.scale(1.0 - _value, 1.0);
    }
}

void SlideControllerPrivate::shearFrame(QTransform& _transform, const QRectF& _rect, qreal _value, int _frame) const
{
    if (_frame == 0 && inversed_)
    {
        _transform.translate(_rect.width() * (_value), 0);
        _transform.scale(1.0 - _value, 1.0);
    }
    if (_frame == 1 && !inversed_)
    {
        _transform.translate(_rect.width() * (_value), 0);
        _transform.scale(1.0 - _value, 1.0);
    }
}

SlideController::SlideDirection SlideControllerPrivate::currentDirection() const
{
    using SlideDir = SlideController::SlideDirection;
    if (!inversed_)
        return slideDir_;
    // evaluate inversed direction
    if (slideDir_ == SlideDir::SlideUp || slideDir_ == SlideDir::SlideDown)
        return (slideDir_ == SlideDir::SlideUp ? SlideDir::SlideDown : SlideDir::SlideUp);

    if (slideDir_ == SlideDir::SlideLeft || slideDir_ == SlideDir::SlideRight)
        return (slideDir_ == SlideDir::SlideLeft ? SlideDir::SlideRight : SlideDir::SlideLeft);

    return slideDir_;
}

void SlideControllerPrivate::renderFrame(int _frame, double _value, QPainter& _painter) const
{
    QRect r = backbuffer_.rect();
    QTransform transform = _painter.transform();

    switch (fading_)
    {
    case SlideController::Fading::FadeIn:
        if (_frame == 1)
            _painter.setOpacity(1.0 - _value);
        break;
    case SlideController::Fading::FadeOut:
        if (_frame == 0)
            _painter.setOpacity(1.0 - _value);
        break;
    case SlideController::Fading::FadeInOut:
        _painter.setOpacity(_frame == 0 ? _value : (1.0 - _value));
        break;
    default: // fading is disabled or unknown
        break;
    }

    for (int i = SlideController::ZoomEffect; i < SlideController::RollEffect + 1; i <<= 1)
    {
        if (!effectFlags_.testFlag(static_cast<SlideController::SlideEffect>(i)))
            continue;

        switch (i)
        {
        case SlideController::ZoomEffect:
            zoomFrame(transform, r, _value, _frame);
            break;
        case SlideController::RotateEffect:
            rotateFrame(transform, r, _value, _frame);
            break;
        case SlideController::ShearEffect:
            shearFrame(transform, r, _value, _frame);
            break;
        case SlideController::SwapEffect:
            swapFrame(transform, r, _value, _frame);
            break;
        case SlideController::RollEffect:
            rollFrame(transform, r, _value, _frame);
            break;
        }
    }

    _painter.setTransform(transform, true);
    _painter.drawPixmap(0, 0, frames_[_frame]);
    _painter.resetTransform();
}

void SlideControllerPrivate::clearCache()
{
    // erase pixmaps and free the memory according to caching policy
    switch (caching_)
    {
    case SlideController::CachingPolicy::CacheNone: // release all pixmaps
        frames_[0] = QPixmap();
        frames_[1] = QPixmap();
        backbuffer_ = QPixmap();
        break;
    case SlideController::CachingPolicy::CacheCurrent: // release all but current pixmap
        frames_[1] = QPixmap();
        backbuffer_ = QPixmap();
        break;
    default: // do not release anything
        break;
    }
}

SlideController::SlideController(QWidget* _parent)
    : QObject(_parent)
    , d(std::make_unique<SlideControllerPrivate>(this))
{
    d->overlay_->setParent(_parent);
    connect(d->timeLine_, &QTimeLine::valueChanged, this, &SlideController::render);
    connect(d->timeLine_, &QTimeLine::finished, this, &SlideController::onAnimationFinished);
}

SlideController::~SlideController() = default;

void SlideController::setCachingPolicy(SlideController::CachingPolicy _policy)
{
    d->caching_ = _policy;
}

SlideController::CachingPolicy SlideController::cachingPolicy() const
{
    return d->caching_;
}

void SlideController::setWidget(QStackedWidget* _widget)
{
    if (d->widget_)
    {
        disconnect(d->widget_);
        d->widget_->removeEventFilter(this);
    }

    d->widget_ = _widget;
    if (_widget)
    {
        if (d->enabled_)
        {
            connect(d->widget_, &QStackedWidget::currentChanged, this, &SlideController::onCurrentChange);
            d->widget_->installEventFilter(this);
        }
    }
}

QWidget* SlideController::widget() const
{
    return d->widget_;
}

QPixmap SlideController::activePixmap() const
{
    return d->frames_[0];
}

void SlideController::setEasingCurve(const QEasingCurve& _easing)
{
    d->timeLine_->setEasingCurve(_easing);
}

QEasingCurve SlideController::easingCurve() const
{
    return d->timeLine_->easingCurve();
}

void SlideController::setEffects(SlideController::SlideEffects _flags)
{
    d->effectFlags_ = _flags;
}

SlideController::SlideEffects SlideController::effects() const
{
    return d->effectFlags_;
}

void SlideController::setSlideDirection(SlideController::SlideDirection _dir)
{
    d->slideDir_ = _dir;
}

SlideController::SlideDirection SlideController::slideDirection() const
{
    return d->slideDir_;
}

void SlideController::setFillColor(QColor _color)
{
    d->fillColor_ = _color;
}

QColor SlideController::fillColor() const
{
    return d->fillColor_;
}

void SlideController::setDuration(int _ms)
{
    d->timeLine_->setDuration(_ms);
}

int SlideController::duration() const
{
    return d->timeLine_->duration();
}

void SlideController::setFading(Fading _fading)
{
    d->fading_ = _fading;
}

SlideController::Fading SlideController::fading() const
{
    return d->fading_;
}

bool SlideController::isFaded() const
{
    return d->fading_ != Fading::NoFade;
}

void SlideController::setInverse(bool _on)
{
    d->inverse_ = _on;
}

bool SlideController::isInverse() const
{
    return d->inverse_;
}

void SlideController::setEnabled(bool _on)
{
    if (d->enabled_ == _on)
        return;

    if (d->widget_)
    {
        if (_on)
        {
            connect(d->widget_, &QStackedWidget::currentChanged, this, &SlideController::onCurrentChange, Qt::UniqueConnection);
            d->widget_->installEventFilter(this);
        }
        else
        {
            disconnect(d->widget_);
            d->widget_->removeEventFilter(this);
            d->clearCache();
        }
    }
    d->enabled_ = _on;
}

void SlideController::updateCache()
{
    QWidget* current = currentWidget();
    QWidget* previous = widget(d->previousIndex_);

    QRect r;
    switch (d->caching_)
    {
    case SlideController::CachingPolicy::CacheAll:
        if (!current)
            return;
        r = current->rect();
        d->frames_[0] = current->grab(r);
        if (previous)
            d->frames_[1] = previous->grab(r);
        if (d->widget_ != nullptr && d->backbuffer_.size() != d->widget_->size())
            d->backbuffer_ = QPixmap(d->widget_->size());
        break;

    case SlideController::CachingPolicy::CacheCurrent:
        if (!current)
            return;
        r = current->rect();
        d->frames_[0] = current->grab(r);
        break;

    default:
        break;
    }
}

bool SlideController::isEnabled() const
{
    return d->enabled_;
}

QWidget* SlideController::currentWidget() const
{
    return (d->widget_ != nullptr ? d->widget_->currentWidget() : nullptr);
}

QWidget* SlideController::widget(int _id) const
{
    return (d->widget_ != nullptr ? d->widget_->widget(_id) : nullptr);
}

int SlideController::currentIndex() const
{
    return (d->widget_ != nullptr ? d->widget_->currentIndex() : -1);
}

int SlideController::count() const
{
    return (d->widget_ != nullptr ? d->widget_->count() : 0);
}

bool SlideController::eventFilter(QObject* _object, QEvent* _event)
{
    static bool lock = false;

    if (d->previousIndex_ == -1)
        return false;

    if (_object == d->widget_)
    {
        if (_event->type() == QEvent::Resize)
        {
            QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(_event);
            if (count() < 2)
                return QObject::eventFilter(_object, _event);

            QWidget* current = currentWidget();
            QWidget* previous = widget(d->previousIndex_);
            QRect r = current->rect();
            if (!lock && previous)
            {
                lock = true;
                d->frames_[0] = current->grab(r);
                d->frames_[1] = previous->grab(r);
                lock = false;
            }
            if (resizeEvent->size() != d->backbuffer_.size())
            {
                d->backbuffer_ = QPixmap(resizeEvent->size());
                d->overlay_->resize(resizeEvent->size());
            }
        }
        else if (_event->type() == QEvent::Show)
        {
            d->timeLine_->setCurrentTime(d->timeLine_->duration());
            d->timeLine_->stop();
        }
    }
    return QObject::eventFilter(_object, _event);
}


void SlideController::onCurrentChange(int _index)
{
    if (!d->widget_->isVisible())
    {
        d->previousIndex_ = currentIndex(); // swap indices
        return; // don't show animation if target widget is not visible
    }

    if (_index == -1)
        return; // don't show animation if current index is invalid

    if (count() < 2)
        return; // don't show animation if there is less than 2 widgets

    if (d->effectFlags_ == NoEffect && !isFaded())
        return; // don't show animation if there is no fading and no other effects

    if (!isEnabled())
        return;

    if (d->previousIndex_ == -1)
        return;

    QWidget* w = currentWidget();

    d->cursor_ = d->widget_->cursor(); // save current cursor
    if (const auto child = w->childAt(w->mapFromGlobal(QCursor::pos())))
        d->widget_->setCursor(child->cursor()); // set the underlying widget cursor

    QRect r = w->rect();

    // grab target and current widgets into pixmaps
    d->frames_[0] = widget(_index)->grab(r);
    d->frames_[1] = widget(d->previousIndex_)->grab(r);

    if (d->inverse_) // evaluate if we need to play animation in inversed direction
        d->inversed_ = ((_index - d->previousIndex_) > 0);

    if (d->timeLine_->state() == QTimeLine::Running)
        d->timeLine_->stop();

    // create the back buffer pixmap
    d->backbuffer_ = QPixmap(d->widget_->size());
    render(0);
    d->timeLine_->start();

    d->overlay_->setGeometry(d->widget_->rect());
    d->overlay_->raise();
    d->overlay_->show();

    d->previousIndex_ = currentIndex(); // swap indices
}

void SlideController::onAnimationFinished()
{
    if (!d->widget_)
    {
        d->clearCache();
        return;
    }

    d->widget_->setCursor(d->cursor_); // restore cursor

    QWidget* w = currentWidget();
    if (!w)
    {
        d->clearCache();
        return;
    }

    d->opacity_ = 0.0;
    w->updateGeometry();

    d->overlay_->hide();

    Q_EMIT currentIndexChanged(currentIndex(), QPrivateSignal{});
    // erase pixmaps and free the memory according to caching policy
    d->clearCache();
}

void SlideController::render(double _value)
{
    if (d->backbuffer_.isNull())
        return;

    d->backbuffer_.fill(d->fillColor_);

    QPainter painter(&d->backbuffer_);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

    if (d->inversed_)
    {
        d->renderFrame(1, _value, painter);
        d->renderFrame(0, _value, painter);
    }
    else
    {
        d->renderFrame(0, _value, painter);
        d->renderFrame(1, _value, painter);
    }

    d->overlay_->setPixmap(d->backbuffer_);

    d->widget_->update();
}

} // end namespace Utils
