#pragma once

#include "../../common.shared/antivirus/antivirus_types.h"

namespace Data
{

struct FileSharingMeta
{
    int64_t size_ = -1;
    int32_t duration_ = 0;
    int64_t lastModified_ = -1;
    QString filenameShort_;
    QString downloadUri_;
    QString localPath_;
    QString miniPreviewUri_;
    QString fullPreviewUri_;
    bool savedByUser_ = false;
    bool recognize_ = false;
    bool gotAudio_ = false;
    bool trustRequired_ = false;
    QPixmap preview_;
    core::antivirus::check antivirusCheck_;

    void mergeWith(const FileSharingMeta& _other)
    {
        auto updateIfNeeded = [this, &_other](auto _ref)
        {
            static const FileSharingMeta empty;

            const auto& emptyValue = empty.*_ref;
            const auto& newValue = _other.*_ref;
            auto& curValue = this->*_ref;

            if (curValue != newValue && newValue != emptyValue)
                curValue = newValue;
        };

        updateIfNeeded(&FileSharingMeta::size_);
        updateIfNeeded(&FileSharingMeta::duration_);
        updateIfNeeded(&FileSharingMeta::lastModified_);
        updateIfNeeded(&FileSharingMeta::filenameShort_);
        updateIfNeeded(&FileSharingMeta::downloadUri_);
        updateIfNeeded(&FileSharingMeta::localPath_);
        updateIfNeeded(&FileSharingMeta::miniPreviewUri_);
        updateIfNeeded(&FileSharingMeta::fullPreviewUri_);
        updateIfNeeded(&FileSharingMeta::savedByUser_);
        updateIfNeeded(&FileSharingMeta::recognize_);
        updateIfNeeded(&FileSharingMeta::gotAudio_);
        updateIfNeeded(&FileSharingMeta::trustRequired_);
        updateIfNeeded(&FileSharingMeta::antivirusCheck_);

        // magic doesnt work with pixmaps =(
        if (preview_.isNull())
            preview_ = _other.preview_;
    }
};

}
