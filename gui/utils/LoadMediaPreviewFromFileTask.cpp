#include "stdafx.h"

#include "utils.h"
#include "main_window/history_control/MessageStyle.h"
#include "LoadMediaPreviewFromFileTask.h"

namespace Utils
{

LoadMediaPreviewFromFileTask::LoadMediaPreviewFromFileTask(const QString& _path)
    : path_(_path)
{
}

LoadMediaPreviewFromFileTask::~LoadMediaPreviewFromFileTask()
{

}

void LoadMediaPreviewFromFileTask::run()
{
    if (Q_UNLIKELY(!QCoreApplication::instance()))
        return;
    if (!QFile::exists(path_))
    {
        Q_EMIT loaded(QPixmap(), QSize());
        return;
    }

    QPixmap preview;
    QSize originalSize;

    QImageReader reader(path_);
    originalSize = reader.size();

    const auto cropThreshold = Ui::MessageStyle::Preview::mediaCropThreshold();
    auto maxSize = Utils::scale_bitmap(QSize(Ui::MessageStyle::Preview::getImageWidthMax(), Ui::MessageStyle::Preview::getImageHeightMax()));
    const auto scaledSize = originalSize.scaled(maxSize, Qt::KeepAspectRatio);

    if (std::min(scaledSize.width(), scaledSize.height()) < cropThreshold)
        maxSize = originalSize;

    Utils::loadPixmapScaled(path_, maxSize, preview, originalSize, Utils::PanoramicCheck::no);

    if (Q_LIKELY(QCoreApplication::instance()))
        Q_EMIT loaded(preview, originalSize);
}

}
