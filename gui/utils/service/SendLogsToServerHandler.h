#pragma once

#include "RequestHandler.h"

namespace Utils
{

class SendLogsToServerRequest;

class SendLogsToServerHandler: public RequestHandler
{
public:
    SendLogsToServerHandler(std::shared_ptr<SendLogsToServerRequest> _request);
    virtual ~SendLogsToServerHandler();

    void start() override;
    void stop() override;

private:
    void onHaveLogsPath(const QString& _logsPath);
    bool getUserConsent() const;

private:
    std::shared_ptr<SendLogsToServerRequest> request_;
};

}
