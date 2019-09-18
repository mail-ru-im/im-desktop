#include "stdafx.h"
#include "LogsInfo.h"

namespace Utils
{

LogsInfo::LogsInfo(int32_t _maxLogsFilesCount, int32_t _numRTPFilesLimit)
    : numLogsFilesLimit_(_maxLogsFilesCount), numRTPFilesLimit_(_numRTPFilesLimit)
{}

int32_t LogsInfo::getNumLogsFilesLimit() const
{
    return numLogsFilesLimit_;
}

int32_t LogsInfo::getNumRTPFilesLimit() const
{
    return numRTPFilesLimit_;
}

bool LogsInfo::haveNumLogsFilesLimit() const
{
    return numLogsFilesLimit_ != -1;
}

bool LogsInfo::haveNumRTPFilesLimit() const
{
    return numRTPFilesLimit_ != -1;
}


}
