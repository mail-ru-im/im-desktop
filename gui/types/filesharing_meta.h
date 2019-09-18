#pragma once

namespace Data
{

struct FileSharingMeta
{
    int64_t size_;
    int32_t duration_;
    int64_t lastModified_;
    QString filenameShort_;
    QString downloadUri_;
    QString localPath_;
    bool savedByUser_;
    bool recognize_;
    bool gotAudio_;
};

}
