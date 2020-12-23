#include "stdafx.h"

#include "utils.h"
#include "ResizePixmapTask.h"

namespace Utils
{
    ResizePixmapTask::ResizePixmapTask(const QPixmap& _preview, const QSize& _size, Qt::AspectRatioMode _aspectMode)
        : preview_(_preview)
        , size_(_size)
        , aspectMode_(_aspectMode)
    {
        assert(!preview_.isNull());
        assert(!size_.isEmpty());
    }

    void ResizePixmapTask::run()
    {
        if (Q_UNLIKELY(!QCoreApplication::instance()))
            return;
        preview_ = preview_.scaled(
            size_,
            aspectMode_,
            Qt::SmoothTransformation
            );

        Utils::check_pixel_ratio(preview_);

        if (Q_LIKELY(QCoreApplication::instance()))
            Q_EMIT resizedSignal(preview_);
    }
}