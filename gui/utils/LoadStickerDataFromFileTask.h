#pragma once

#include "cache/stickers/stickers.h"

namespace Utils
{
    class LoadStickerDataFromFileTask : public QObject, public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void loadedBatch(Ui::Stickers::StickerLoadDataV _loaded, QPrivateSignal) const;

    public:
        LoadStickerDataFromFileTask(Ui::Stickers::StickerLoadDataV _toLoad);

        void run() override;

    private:
        Ui::Stickers::StickerLoadDataV loadData_;
    };
}