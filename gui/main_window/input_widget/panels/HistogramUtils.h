#pragma once

namespace Ui::histogram
{
    [[nodiscard]] int getSampleWidth() noexcept;

    [[nodiscard]] int getSampleSpace() noexcept;

    [[nodiscard]] double getVolumeFactor(double _v) noexcept;

    [[nodiscard]] int getSampleHeight(double sample, double volumeFactor, int maxHeight) noexcept;

    [[nodiscard]] QPainterPath makeSamplePath(double sample, double volumeFactor, int currentPos, int maxHeight);

    [[nodiscard]] QPainterPath makeRelativeSamplePath(int height, int maxHeight);

    [[nodiscard]] QPixmap makeRelativePixmap(int height, int maxHeight, const QColor& color);
}