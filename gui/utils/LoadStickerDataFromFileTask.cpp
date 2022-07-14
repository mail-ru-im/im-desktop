#include "stdafx.h"

#include "LoadStickerDataFromFileTask.h"
#include "main_window/history_control/complex_message/FileSharingUtils.h"

namespace Utils
{
    LoadStickerDataFromFileTask::LoadStickerDataFromFileTask(Ui::Stickers::StickerLoadDataV _toLoad)
        : loadData_(std::move(_toLoad))
    {
    }

    void LoadStickerDataFromFileTask::run()
    {
        if (!loadData_.empty())
        {
            for (auto& p : loadData_)
            {
                if (Ui::ComplexMessage::isLottieFileSharingId(p.fsId_.fileId_))
                    p.data_ = Ui::Stickers::StickerData::makeLottieData(p.path_);
                else
                    p.data_.loadFromFile(p.path_);
            }

            Q_EMIT loadedBatch(std::move(loadData_), QPrivateSignal());
        }
    }
}