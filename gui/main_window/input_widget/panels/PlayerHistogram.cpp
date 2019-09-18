#include "stdafx.h"

#include "PlayerHistogram.h"
#include "HistogramUtils.h"

#include "styles/ThemeParameters.h"
#include "utils/utils.h"
#include "utils/PainterPath.h"

#include "media/ptt/AudioUtils.h"

#include <boost/range/adaptor/reversed.hpp>

namespace
{
    void sampling(const std::vector<double>& a, std::vector<double>& res, size_t newSize)
    {
        if (a.empty())
        {
            res.clear();
            return;
        }
        res.resize(newSize);

        for (size_t i = 0; i < newSize - 1; ++i) {
            int index = i * (a.size() - 1) / (newSize - 1);
            int p = i * (a.size() - 1) % (newSize - 1);

            res[i] = ((p * a[index + 1]) + (((newSize - 1) - p) * a[index])) / (newSize - 1);
        }
        res[newSize - 1] = a[a.size() - 1];
    }
}

namespace Ui
{
    PlayerHistogram::PlayerHistogram(QWidget* _parent)
        : ClickableWidget(_parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    PlayerHistogram::~PlayerHistogram() = default;

    void PlayerHistogram::start()
    {
        isActive_ = true;
        clear();
        update();
    }

    void PlayerHistogram::stop()
    {
        isActive_ = false;
        resample(width());
        update();
    }

    void PlayerHistogram::clear()
    {
        ampl_.clear();
        resampledAmpl_.clear();
        currentSample_ = -1;
    }

    bool PlayerHistogram::isActive() const
    {
        return isActive_;
    }

    bool PlayerHistogram::hasSamples() const
    {
        return !ampl_.empty();
    }

    void PlayerHistogram::addSpectrum(const QVector<double>& _v)
    {
        for (auto x : _v)
            ampl_.push_back(x);

        update();
    }

    void PlayerHistogram::setCurrentSample(int _current)
    {
        if (currentSample_ != _current)
        {
            currentSample_ = _current;
            update();
        }
    }

    std::optional<PlayerHistogram::ScaledCoeff> PlayerHistogram::sampleUnderCursor() const
    {
        if (resampledAmpl_.empty())
            return {};
        const auto pos = mapFromGlobal(QCursor::pos());
        if (pos.x() < 0 || pos.x() > width())
            return {};

        const int idx = std::clamp(pos.x() / (histogram::getSampleWidth() + histogram::getSampleSpace()), 0, int(resampledAmpl_.size() - 1));
        return ScaledCoeff{ idx, double(resampledAmpl_.size()) / ampl_.size() };
    }

    void PlayerHistogram::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const auto h = height();
        const auto w = width();

        static const auto sampleWidth = histogram::getSampleWidth();
        static const auto space = histogram::getSampleSpace();

        const double volumeFactor = histogram::getVolumeFactor(volume_);

        enum class Direction
        {
            direct,
            reverse
        };

        auto fillByDots = [sampleWidth = sampleWidth, space = space, w, h, &p](auto& currentPos, const QColor& color, Direction direction)
        {
            if (direction == Direction::reverse)
            {
                while (currentPos >= 0)
                {
                    p.fillPath(histogram::makeSamplePath(0, 1.0, currentPos, h), color);
                    currentPos -= sampleWidth + space;
                }
            }
            else
            {
                while (currentPos < w)
                {
                    p.fillPath(histogram::makeSamplePath(0, 1.0, currentPos, h), color);
                    currentPos += sampleWidth + space;
                }
            }
        };

        int currentPos = 0;

        if (isActive())
        {
            currentPos = w;
            const auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            if (int(resampledAmpl_.size() * (sampleWidth + space)) > w)
            {
                for (auto x : boost::adaptors::reverse(resampledAmpl_))
                {
                    p.fillPath(histogram::makeSamplePath(x, volumeFactor, currentPos, h), color);
                    currentPos -= sampleWidth + space;

                    if (currentPos <= 0)
                        break;
                }
            }
            else
            {
                for (auto x : boost::adaptors::reverse(resampledAmpl_))
                {
                    p.fillPath(histogram::makeSamplePath(x, volumeFactor, currentPos, h), color);
                    currentPos -= sampleWidth + space;

                    if (currentPos <= 0)
                        break;
                }

                fillByDots(currentPos, color, Direction::reverse);
            }
        }
        else
        {
            const auto color1 = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
            const auto color2 = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_HOVER);
            const int resmapledCurrentSample = (currentSample_ > 0  && currentSample_ != std::numeric_limits<int>::max()) ? int(currentSample_ * (double(resampledAmpl_.size()) / ampl_.size()) + 0.5) : currentSample_;
            int current = 0;
            for (auto x : resampledAmpl_)
            {
                p.fillPath(histogram::makeSamplePath(x, volumeFactor, currentPos, h), current <= resmapledCurrentSample ? color1 : color2);
                currentPos += sampleWidth + space;
                ++current;
            }
            fillByDots(currentPos, resmapledCurrentSample >= int(resampledAmpl_.size()) ? color1 : color2, Direction::direct);
        }
    }

    void PlayerHistogram::resizeEvent(QResizeEvent* _e)
    {
        ClickableWidget::resizeEvent(_e);
        if (const auto newWidgth = _e->size().width(); newWidgth != _e->oldSize().width())
            resample(newWidgth);
    }

    void PlayerHistogram::resample(int _width)
    {
        static const auto sampleWidth = histogram::getSampleWidth();
        static const auto space = histogram::getSampleSpace();

        const auto neededSize = size_t(double(_width) / (sampleWidth + space) + 0.5);

        sampling(ampl_, resampledAmpl_, neededSize);
    }

    int PlayerHistogram::getSampleWidth()
    {
        return histogram::getSampleWidth();
    }

    void PlayerHistogram::setVolume(double _v)
    {
        volume_ = _v;
    }
}