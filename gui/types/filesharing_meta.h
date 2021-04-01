#pragma once

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
    QPixmap preview_;
};

}
