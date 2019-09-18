#pragma once

#include <stdint.h>

namespace Utils
{

class LogsInfo
{
public:
    LogsInfo(int32_t _maxLogsFilesCount = -1, int32_t _numRTPFilesLimit = -1);

    int32_t getNumLogsFilesLimit() const;
    int32_t getNumRTPFilesLimit() const;

    bool haveNumLogsFilesLimit() const;
    bool haveNumRTPFilesLimit() const;

private:
    int32_t numLogsFilesLimit_ = -1;
    int32_t numRTPFilesLimit_ = -1;
};

}
