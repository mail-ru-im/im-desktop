#include "stdafx.h"

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
        preview_ = preview_.scaled(
            size_,
            aspectMode_,
            Qt::SmoothTransformation
            );

        emit resizedSignal(preview_);
    }
}