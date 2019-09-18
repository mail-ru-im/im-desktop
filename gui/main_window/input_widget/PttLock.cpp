#include "stdafx.h"

#include "PttLock.h"

#include "utils/utils.h"
#include "controls/TooltipWidget.h"

namespace
{
    constexpr std::chrono::milliseconds getAnimDuration() noexcept { return std::chrono::milliseconds(425); }
    constexpr std::chrono::milliseconds delay() noexcept { return std::chrono::milliseconds(100); }

    auto getDistance()
    {
        return Utils::scale_value(84);
    }
}

namespace Ui
{
    PttLock::PttLock(QWidget* _parent)
        : QWidget(_parent)
    {
        const auto size = QSize(40, 40);
        pixmap_ = Utils::renderSvgScaled(qsl(":/ptt/lock_icon"), size);
        setFixedSize(Utils::scale_value(size));
        setAttribute(Qt::WA_TransparentForMouseEvents);

        showTimer_.setSingleShot(true);
        showTimer_.setInterval(delay().count());
        QObject::connect(&showTimer_, &QTimer::timeout, this, &PttLock::showAnimatedImpl);
    }

    PttLock::~PttLock() = default;

    void PttLock::setBottom(QPoint _p)
    {
        if (bottom_ != _p)
            bottom_ = _p;
    }

    void PttLock::showAnimated()
    {
        anim_.finish();
        showTimer_.start();
    }

    void PttLock::hideAnimated()
    {
        hideToolTip();
        showTimer_.stop();
        anim_.finish();
        anim_.start([this]()
        {
            auto p = bottom_;
            p.ry() -= getDistance();
            p.ry() += (1 - anim_.current()) * getDistance();
            move(p);
            update();
        }, 1., 0., getAnimDuration().count(), anim::easeOutBack);
    }

    void PttLock::hideForce()
    {
        hideToolTip();
        showTimer_.stop();
        anim_.finish();
        hide();
    }

    void PttLock::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setOpacity(anim_.current());
        p.drawPixmap(0, 0, pixmap_);
    }

    void PttLock::showAnimatedImpl()
    {
        show();
        anim_.finish();
        anim_.start([this]()
        {
            auto p = bottom_;
            p.ry() -= (anim_.current()) * getDistance();
            move(p);
            update();
        }, [this]()
        {
            showToolTip();
        }, 0., 1., getAnimDuration().count(), anim::easeOutBack);
    }

    void PttLock::showToolTip()
    {
        const auto r = rect();
        Tooltip::forceShow(true);
        Tooltip::show(QT_TRANSLATE_NOOP("input_widget", "Lock"), QRect(mapToGlobal(r.topLeft()), r.size()), { -1, -1 }, Tooltip::ArrowDirection::Down);
    }

    void PttLock::hideToolTip()
    {
        Tooltip::forceShow(false);
        Tooltip::hide();
    }
}
