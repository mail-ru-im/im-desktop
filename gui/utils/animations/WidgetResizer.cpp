#include "WidgetResizer.h"

namespace Utils
{

class WidgetResizerPrivate
{
public:
    QPropertyAnimation* animation_;
    bool enabled_;

    WidgetResizerPrivate(WidgetResizer* _q)
        : animation_(new QPropertyAnimation(_q))
        , enabled_(true)
    {
    }

    void initializeAnimation(QObject* _target, Qt::Alignment _align, int _lower, int _upper)
    {
        animation_->setStartValue(_lower);
        animation_->setEndValue(_upper);
        animation_->setDuration(200);
        animation_->setTargetObject(_target);
        switch (_align)
        {
        case Qt::AlignRight:
        case Qt::AlignLeft:
            animation_->setPropertyName(QByteArrayLiteral("maximumWidth"));
            break;
        case Qt::AlignTop:
        case Qt::AlignBottom:
            animation_->setPropertyName(QByteArrayLiteral("maximumHeight"));
            break;
        default:
            assert(false);
            break;
        }
    }
};

WidgetResizer::WidgetResizer(QWidget* _widget, Qt::Alignment _align, int _minimum, int _maximum)
    : QObject(_widget)
    , d(new WidgetResizerPrivate(this))
{
    assert(_widget != nullptr);
    d->initializeAnimation(_widget, _align, _minimum, _maximum);
}

WidgetResizer::~WidgetResizer()
{
}

void WidgetResizer::setEnabled(bool _on)
{
    d->enabled_ = _on;
}

bool WidgetResizer::isEnabled() const
{
    return d->enabled_;
}

void WidgetResizer::setDuration(int _msecs)
{
    d->animation_->setDuration(_msecs);
}

int WidgetResizer::duration() const
{
    return d->animation_->duration();
}

void WidgetResizer::slideIn()
{
    if (d->enabled_)
    {
        d->animation_->setDirection(QPropertyAnimation::Forward);
        d->animation_->start();
    }
    else
    {
        assert(d->animation_->targetObject() != nullptr);
        d->animation_->targetObject()->setProperty(d->animation_->propertyName().constData(), d->animation_->endValue());
    }
}

void WidgetResizer::slideOut()
{
    if (d->enabled_)
    {
        d->animation_->setDirection(QPropertyAnimation::Backward);
        d->animation_->start();
    }
    else
    {
        assert(d->animation_->targetObject() != nullptr);
        d->animation_->targetObject()->setProperty(d->animation_->propertyName().constData(), d->animation_->startValue());
    }
}

} // end namespace Utils
