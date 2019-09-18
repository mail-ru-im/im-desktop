#include "stdafx.h"

#include "RecorderHistogram.h"
#include "HistogramUtils.h"

#include "styles/ThemeParameters.h"
#include "utils/utils.h"
#include "utils/PainterPath.h"
#include "media/ptt/AudioUtils.h"

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range.hpp>

namespace
{
    constexpr std::chrono::milliseconds animationTimeout() noexcept
    {
        return std::chrono::milliseconds(200);
    }
}

namespace Ui
{
    RecorderHistogram::RecorderHistogram(QWidget* _parent)
        : ClickableWidget(_parent)
        , animation_(new QPropertyAnimation(this, QByteArrayLiteral("animationValue"), this))
    {
        animation_->setStartValue(0.0);
        animation_->setEndValue(1.0);
        animation_->setDuration(animationTimeout().count());
        animation_->setLoopCount(-1);
        //animation_->setEasingCurve(QEasingCurve::OutInQuad);
        QObject::connect(this, &ClickableWidget::clicked, this, &RecorderHistogram::onClicked);
        QObject::connect(animation_, &QAbstractAnimation::currentLoopChanged, this, &RecorderHistogram::animationLoopChanged);
    }

    RecorderHistogram::~RecorderHistogram() = default;

    void RecorderHistogram::start()
    {
        animation_->start();
        isActive_ = true;
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        clear();
        update();
    }

    void RecorderHistogram::stop()
    {
        animation_->stop();
        isActive_ = false;
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        update();
    }

    void RecorderHistogram::clear()
    {
        ampl_.clear();
        pending_.clear();
        currentSample_ = -1;
    }

    bool RecorderHistogram::isActive() const
    {
        return animation_->state() == QAbstractAnimation::State::Running;
    }

    bool RecorderHistogram::hasSamples() const
    {
        return !ampl_.empty();
    }

    void RecorderHistogram::addSpectrum(const QVector<double>& _v)
    {
        pending_.insert(pending_.end(), _v.begin(), _v.end());
    }

    void RecorderHistogram::setCurrentSample(int _current)
    {
        if (currentSample_ != _current)
        {
            currentSample_ = _current;
            update();
        }
    }

    void RecorderHistogram::setVolume(double _v)
    {
        volume_ = _v;
    }

    void RecorderHistogram::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const auto h = height();
        if (h <= 0)
            return;
        const auto w = width();
        const auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);

        while (h > 0 && int(samplePaths_.size()) <= h + 1)
            samplePaths_.push_back(histogram::makeRelativeSamplePath(samplePaths_.size(), h));

        static const auto sampleWidth = histogram::getSampleWidth();
        static const auto space = histogram::getSampleSpace();

        const double volumeFactor = histogram::getVolumeFactor(volume_);

        auto fillByDots = [sampleWidth = sampleWidth, space = space, h, &p, this](auto& currentPos, const QColor& color)
        {
            const auto currentAmpl = histogram::getSampleHeight(0.0, 1.0, h);
            while (currentPos >= 0)
            {
                p.fillPath(samplePaths_[currentAmpl], color);
                p.translate(-(sampleWidth + space), 0);

                currentPos -= sampleWidth + space;
            }
        };

        p.translate(w, 0);

        int currentPos = w;
        if (ampl_.empty())
        {
            fillByDots(currentPos, color);
            return;
        }

        auto prevSample = ampl_.back();

        auto currentAmpl = histogram::getSampleHeight(prevSample, volumeFactor, h);
        p.fillPath(samplePaths_[currentAmpl], color);
        currentPos -= sampleWidth + space;
        const auto anim = animationValue();
        for (auto x : boost::adaptors::reverse(boost::make_iterator_range(ampl_.begin(), std::prev(ampl_.end()))))
        {
            currentAmpl = histogram::getSampleHeight(x + (prevSample - x) * anim, volumeFactor, h);
            p.fillPath(samplePaths_[currentAmpl], color);
            p.translate(-(sampleWidth + space), 0);
            currentPos -= sampleWidth + space;

            if (currentPos <= 0)
                return;

            prevSample = x;
        }

        fillByDots(currentPos, color);
    }

    double RecorderHistogram::animationValue() const noexcept
    {
        return animationValue_;
    }

    void RecorderHistogram::setAnimationValue(double _v)
    {
        animationValue_ = _v;
        update();
    }

    void RecorderHistogram::onClicked()
    {
        if (ampl_.empty())
        {
            emit clickOnSample(0, 0, QPrivateSignal());
        }
        else
        {
            const auto pos = mapFromGlobal(QCursor::pos());
            int idx = pos.x() / (histogram::getSampleWidth() + histogram::getSampleSpace());

            idx = std::clamp(idx, 0, int(ampl_.size() - 1));
            emit clickOnSample(idx, 0, QPrivateSignal());
        }
    }

    void RecorderHistogram::animationLoopChanged()
    {
        if (pending_.empty())
            return;
        ampl_.push_back(*std::max_element(pending_.begin(), pending_.end()));
        pending_.clear();
        update();
    }
}
