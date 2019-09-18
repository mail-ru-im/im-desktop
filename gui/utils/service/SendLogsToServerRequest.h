#pragma once

#include "ServiceRequest.h"
#include "LogsInfo.h"

namespace Utils
{

class SendLogsToServerRequest: public ServiceRequest
{
public:
    struct Info
    {
        Info(const LogsInfo& _logsInfo);

        bool isValid() const;
        LogsInfo logsInfo_;
    };

public:
    SendLogsToServerRequest(Id _id, Info _info);
    virtual ~SendLogsToServerRequest() = default;

    const Info& getInfo() const;

private:
    Info info_;
};

}
