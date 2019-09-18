#include "stdafx.h"

#include "HistogramUtils.h"

#include "utils/utils.h"
#include "utils/PainterPath.h"

namespace Ui::histogram
{
    int getSampleWidth() noexcept
    {
        return Utils::scale_value(2);
    }

    int getSampleSpace() noexcept
    {
        return Utils::scale_value(4);
    }

    double getVolumeFactor(double _volume) noexcept
    {
        if constexpr (platform::is_apple())
        {
            auto formula = [](auto x)
            {
                return x * x - 0.05 * x + 0.025; // magic formula for mac amplitude
            };

            return (_volume == 0.0 || _volume == 1.0 || qFuzzyIsNull(_volume) || qFuzzyCompare(_volume, 1.0)) ? 1.0 : 1.0 / formula(_volume);
        }
        else
        {
            Q_UNUSED(_volume);
            return 1.0;
        }
    }

    int getSampleHeight(double sample, double volumeFactor, int maxHeight) noexcept
    {
        auto getNormalizeAmpl = [](double sample)
        {
            const auto t = std::clamp(sample, 0.0, 1.0);
            return 1 - (1 - t) * (1 - t);
        };
        const static auto sampleWidth = getSampleWidth();
        return std::clamp(int(getNormalizeAmpl(sample * volumeFactor) * maxHeight), sampleWidth, maxHeight);
    }

    QPainterPath makeSamplePath(double sample, double volumeFactor, int currentPos, int maxHeight)
    {
        const static auto sampleWidth = getSampleWidth();
        const auto val = getSampleHeight(sample, volumeFactor, maxHeight);
        const auto r = QRect(currentPos, maxHeight - val, sampleWidth, val);
        return Utils::renderMessageBubble(r, sampleWidth / 2, sampleWidth / 2, Utils::RenderBubbleFlags::AllRounded);
    }

    QPainterPath makeRelativeSamplePath(int height, int maxHeight)
    {
        const static auto sampleWidth = getSampleWidth();
        const auto r = QRect(0, maxHeight - height, sampleWidth, height);
        return Utils::renderMessageBubble(r, sampleWidth / 2, sampleWidth / 2, Utils::RenderBubbleFlags::AllRounded);
    }

    QPixmap makeRelativePixmap(int height, int maxHeight, const QColor& color)
    {
        const auto path = makeRelativeSamplePath(height, maxHeight);

        QPixmap pixmap(int(path.boundingRect().width()), maxHeight);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.fillPath(path, color);

        return pixmap;
    }
}
