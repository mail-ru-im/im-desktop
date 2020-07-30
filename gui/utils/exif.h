#pragma once

#include "../namespaces.h"

UTILS_EXIF_NS_BEGIN

enum class ExifOrientation
{
    Min                     = 0,

    Normal                  = 1,
    FlipX                   = 2,
    Rotate180C              = 3,
    FlipY                   = 4,
    Rotate90CFlipX          = 5,
    Rotate90C               = 6,
    Rotate90AFlipX          = 7,
    Rotate90A               = 8,

    Max
};

void applyExifOrientation(const ExifOrientation orientation, InOut QPixmap &pixmap);

ExifOrientation getExifOrientation(const char *buf, const size_t bufSize);

UTILS_EXIF_NS_END