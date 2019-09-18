#pragma once

namespace Data
{

struct FileSharingDownloadResult
{
    QString rawUri_;
    QString filename_;
    QString requestedUrl_;
    bool savedByUser_;
};

}
