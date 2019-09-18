#pragma once

#include <QString>
#include "ServiceRequest.h"
#include "LogsInfo.h"

namespace Utils
{

class SendLogsToUinRequest: public ServiceRequest
{
public:
    struct Info
    {
        Info(const QString& _uin, const LogsInfo& _logsInfo);
        bool isValid() const;

        QString uin_;
        LogsInfo logsInfo_;
    };

public:
    SendLogsToUinRequest(ServiceRequest::Id _id, const Info& _info);
    virtual ~SendLogsToUinRequest() = default;

    const Info& getInfo() const;

private:
    Info info_;
};

}
