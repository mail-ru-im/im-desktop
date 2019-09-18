#include "animation.h"

#include <cmath>

namespace
{
    constexpr std::chrono::milliseconds animationTimeout() noexcept
    {
        return std::chrono::milliseconds(16);
    }
}

namespace
{
    std::unique_ptr<anim::AnimationManager> _manager;
}

namespace anim
{
    transition linear = [](double delta, double dt) noexcept {
        return delta * dt;
    };

    transition sineInOut = [](double delta, double dt) noexcept {
        return -(delta / 2.0) * (std::cos(M_PI * dt) - 1.0);
    };

    transition halfSine = [](double delta, double dt) noexcept {
        return delta * std::sin(M_PI * dt / 2);
    };

    transition easeOutBack = [](double delta, double dt) noexcept {
        constexpr double s = 1.70158;

        const double t = dt - 1.0;
        return delta * (t * t * ((s + 1.0) * t + s) + 1.0);
    };

    transition easeInCirc = [](double delta, double dt) noexcept {
        return -delta * (std::sqrt(1.0 - dt * dt) - 1.0);
    };

    transition easeOutCirc = [](double delta, double dt) noexcept {
        const double t = dt - 1.0;
        return delta * std::sqrt(1.0 - t * t);
    };

    transition easeInCubic = [](double delta, double dt) noexcept {
        return delta * dt * dt * dt;
    };

    transition easeOutCubic = [](double delta, double dt) noexcept {
        const double t = dt - 1.0;
        return delta * (t * t * t + 1.0);
    };

    transition easeInQuint = [](double delta, double dt) noexcept {
        const double t2 = dt * dt;
        return delta * t2 * t2 * dt;
    };

    transition easeOutQuint = [](double delta, double dt) noexcept {
        const double t = dt - 1.0;
        const double t2 = t * t;
        return delta * (t2 * t2 * t + 1.0);
    };

    transition easeInExpo = [](double delta, double dt) noexcept {
        return delta * (dt == 0.0 ? 0.0 : std::pow(2.0, 10.0 * (dt - 1.0)));
    };

    transition easeOutExpo = [](double delta, double dt) noexcept {
        return delta * (dt == 1.0 ? 1.0 : -std::pow(2.0, -10.0 * dt) + 1.0);
    };

    void BasicAnimation::start() {
        if (!_manager)
            return;

        _callbacks.start();
        _manager->start(this);
        _animating = true;
    }

    void BasicAnimation::stop() {
        if (!_manager)
            return;

        _animating = false;
        _manager->stop(this);
    }

    void AnimationManager::startManager()
    {
        _manager = std::make_unique<AnimationManager>();
    }

    void AnimationManager::stopManager()
    {
        _manager.reset();
    }

    AnimationManager::AnimationManager()
        : QObject(nullptr)
        , animating_(false)
    {
        QObject::connect(&timer_, &QTimer::timeout, this, &AnimationManager::timeout);
    }

    AnimationManager::~AnimationManager()
    {
    }

    void AnimationManager::start(BasicAnimation* _animation)
    {
        if (animating_)
        {
            starting_.insert(_animation);
            stopping_.erase(_animation);
        }
        else
        {
            if (animations_.empty())
                timer_.start(animationTimeout().count());
            animations_.insert(_animation);
        }
    }

    void AnimationManager::stop(BasicAnimation* _animation)
    {
        if (animating_)
        {
            stopping_.insert(_animation);
            starting_.erase(_animation);
        }
        else
        {
            animations_.erase(_animation);
            if (animations_.empty())
                timer_.stop();
        }
    }

    void AnimationManager::timeout()
    {
        animating_ = true;
        const auto ms = getMs();
        for (auto a : animations_)
        {
            if (stopping_.find(a) == stopping_.end())
                a->step(ms, true);
        }
        animating_ = false;

        if (!starting_.empty()) {
            for (auto a : starting_) {
                animations_.insert(a);
            }
            starting_.clear();
        }
        if (!stopping_.empty()) {
            for (auto a : stopping_) {
                animations_.erase(a);
            }
            stopping_.clear();
        }
        if (animations_.empty())
            timer_.stop();
    }
}
