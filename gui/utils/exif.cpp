#include "stdafx.h"

#include "exif.h"

UTILS_EXIF_NS_BEGIN

void applyExifOrientation(const ExifOrientation orientation, InOut QPixmap &pixmap)
{
    im_assert(!pixmap.isNull());

    const auto isOrientationValid = (
        (orientation > ExifOrientation::Min) &&
        (orientation < ExifOrientation::Max));
    if (!isOrientationValid)
    {
        im_assert(!"invalid orientation value");
        return;
    }

    if (orientation == ExifOrientation::Normal)
    {
        return;
    }

    QMatrix modelMatrix;

    switch(orientation)
    {
        case ExifOrientation::FlipX:
            modelMatrix.scale(-1, 1);
            break;

        case ExifOrientation::FlipY:
            modelMatrix.scale(1, -1);
            break;

        case ExifOrientation::Rotate180C:
            modelMatrix.rotate(180);
            break;

        case ExifOrientation::Rotate90A:
            modelMatrix.rotate(-90);
            break;

        case ExifOrientation::Rotate90AFlipX:
            modelMatrix.rotate(-90);
            modelMatrix.scale(-1, 1);
            break;

        case ExifOrientation::Rotate90C:
            modelMatrix.rotate(90);
            break;

        case ExifOrientation::Rotate90CFlipX:
            modelMatrix.rotate(90);
            modelMatrix.scale(-1, 1);
            break;

        default:
            im_assert(!"IP is not expected to be here");
            return;
    }

    pixmap = pixmap.transformed(modelMatrix);
}

ExifOrientation getExifOrientation(const QString& _filename)
{
    QImageReader r(_filename);
    auto transfromation = r.transformation();

    switch (transfromation)
    {
    case QImageIOHandler::TransformationNone:
        return ExifOrientation::Normal;

    case QImageIOHandler::TransformationMirror:
        return ExifOrientation::FlipX;

    case QImageIOHandler::TransformationRotate180:
        return ExifOrientation::Rotate180C;

    case QImageIOHandler::TransformationFlip:
        return ExifOrientation::FlipY;

    case QImageIOHandler::TransformationFlipAndRotate90:
        return ExifOrientation::Rotate90CFlipX;

    case QImageIOHandler::TransformationRotate90:
        return ExifOrientation::Rotate90C;

    case QImageIOHandler::TransformationMirrorAndRotate90:
        return ExifOrientation::Rotate90AFlipX;

    case QImageIOHandler::TransformationRotate270:
        return ExifOrientation::Rotate90A;

    default:
        return ExifOrientation::Normal;
    }

    return ExifOrientation::Normal;
}

UTILS_EXIF_NS_END