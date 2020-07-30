#pragma once

namespace HistoryControl
{

    QString formatFileSize(const int64_t size);

    QString formatProgressText(const int64_t bytesTotal, const int64_t bytesTransferred);

}