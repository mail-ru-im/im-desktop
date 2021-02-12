#include "ZoomAnimation.h"

namespace
{
    constexpr int kZoomAnimationUpdateInterval = 4;
    constexpr int kZoomAnimationDuration = 100;
}

namespace Utils
{

class ZoomAnimationPrivate
{
public:
    QTimeLine* timeLine_;
    std::string_view propertyName_;
    double zoomFactor_;
    int stepCount_;

    ZoomAnimationPrivate(ZoomAnimation* _q, double _factor, std::string_view _propertyName)
        : timeLine_(new QTimeLine(kZoomAnimationDuration, _q))
        , propertyName_(_propertyName)
        , zoomFactor_(_factor)
        , stepCount_(0)
    {
        assert(!_propertyName.empty());
        assert(zoomFactor_ > 0.0);
        if (zoomFactor_ <= 0.0)
            zoomFactor_ = 1.0; // adjust zoom factor to avoid zero division and negative values

        timeLine_->setUpdateInterval(kZoomAnimationUpdateInterval);
    }
};

ZoomAnimation::ZoomAnimation(QWidget* _viewer, double _factor, std::string_view _propertyName)
    : QObject(_viewer)
    , d(new ZoomAnimationPrivate(this, _factor, _propertyName))
{
    assert(_viewer != nullptr);
    connect(d->timeLine_, &QTimeLine::valueChanged, this, qOverload<>(&ZoomAnimation::animate));
}

ZoomAnimation::~ZoomAnimation()
{
}

void ZoomAnimation::start()
{
    assert(parent() != nullptr);
    if (!parent())
        return;

    if (d->timeLine_->state() == QTimeLine::Running)
        d->timeLine_->stop();

    d->stepCount_ = 0;
    d->timeLine_->start();
}

void ZoomAnimation::stop()
{
    d->stepCount_ = 0;
    d->timeLine_->stop();
}

void ZoomAnimation::animate()
{
    assert(parent() != nullptr);
    QWidget* viewer = static_cast<QWidget*>(parent());
    if (Q_UNLIKELY(viewer == nullptr))
    {
        stop();
        return;
    }

    viewer->setProperty(d->propertyName_.data(), d->zoomFactor_);
}

} // end namespace Utils
