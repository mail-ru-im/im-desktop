#include "stdafx.h"
#include "WidgetFader.h"

namespace Utils
{

class WidgetFaderPrivate
{
public:
    static_assert(QEvent::MaxUser <= 65535, "QEvent::MaxUser is too big");

    struct EventDirection
    {
        uint32_t type_ : 16; // QEvent::Type
        uint32_t dir_ : 1;   // QPropertyAnimation::Direction
    };

    QVarLengthArray<EventDirection, 4> mapping_;
    QPropertyAnimation* animation_;
    bool enabled_;

    WidgetFaderPrivate(WidgetFader* _q)
        : animation_(new QPropertyAnimation(_q))
        , enabled_(true)
    {
    }

    void initializeAnimation(QObject* _target, double _lower, double _upper)
    {
        animation_->setStartValue(_lower);
        animation_->setEndValue(_upper);
        animation_->setDuration(100);
        animation_->setTargetObject(_target);
        animation_->setPropertyName(QByteArrayLiteral("opacity"));
    }

    inline QPropertyAnimation::Direction direction(EventDirection _value) const
    {
        return static_cast<QPropertyAnimation::Direction>(_value.dir_);
    }

    inline auto find(uint32_t _type) const
    {
        return std::find_if(mapping_.cbegin(), mapping_.cend(), [_type](auto _value) { return (_value.type_ == _type); });
    }

    inline auto find(uint32_t _type)
    {
        return std::find_if(mapping_.begin(), mapping_.end(), [_type](auto _value) { return (_value.type_ == _type); });
    }

    void insert(QEvent::Type _type, QPropertyAnimation::Direction _dir)
    {
        EventDirection value;
        value.type_ = static_cast<uint32_t>(_type);
        value.dir_ = static_cast<uint32_t>(_dir);
        auto it = find(_type);
        if (it == mapping_.end())
            mapping_.push_back(value);
        else
            *it = value;
    }
};

WidgetFader::WidgetFader(QWidget* _widget, double _lower, double _upper)
    : QGraphicsOpacityEffect(_widget)
    , d(new WidgetFaderPrivate(this))
{
    im_assert(_widget != nullptr);

    d->initializeAnimation(this, _lower, _upper);

    setOpacity(_upper);
    _widget->setGraphicsEffect(this);
    _widget->installEventFilter(this);
}

WidgetFader::~WidgetFader() = default;

void WidgetFader::setEventDirection(QEvent::Type _type, QPropertyAnimation::Direction _dir)
{
    d->insert(_type, _dir);
}

QPropertyAnimation::Direction WidgetFader::eventDirection(QEvent::Type _type) const
{
    auto it = d->find(static_cast<uint32_t>(_type));
    return (it == d->mapping_.end() ? QPropertyAnimation::Forward : d->direction(*it));
}

void WidgetFader::setDuration(int _msecs)
{
    d->animation_->setDuration(_msecs);
}

int WidgetFader::duration() const
{
    return d->animation_->duration();
}

void WidgetFader::setEnabled(bool _on)
{
    d->enabled_ = _on;
}

bool WidgetFader::isEnabled() const
{
    return d->enabled_;
}

void WidgetFader::setEffectEnabled(QWidget* _root, bool _on)
{
    if (!_root)
        return;

    const QList<QWidget*> children = _root->findChildren<QWidget*>(QString());
    for (auto child : children)
    {
        if (WidgetFader* fader = qobject_cast<WidgetFader*>(child->graphicsEffect()))
            fader->setEnabled(_on);
    }
}

bool WidgetFader::eventFilter(QObject* _object, QEvent* _event)
{
    if (!isEnabled())
        return QObject::eventFilter(_object, _event);

    if (_object != parent())
        return QObject::eventFilter(_object, _event);

    auto it = d->find(static_cast<uint32_t>(_event->type()));
    if (it == d->mapping_.end())
        return QObject::eventFilter(_object, _event);

    if (_event->type() == QEvent::Show && static_cast<QWidget*>(_object)->isVisible())
        return QObject::eventFilter(_object, _event);

    if (d->animation_->state() == QPropertyAnimation::Running)
        return true;

    d->animation_->setDirection(d->direction(*it));
    d->animation_->start();
    return true;
}

} // end namespace Utils
