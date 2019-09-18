#pragma once

#include "RequestHandler.h"

namespace Utils
{

class SendLogsToServerRequest;

class SendLogsToServerHandler: public RequestHandler
{
public:
    SendLogsToServerHandler(SendLogsToServerRequest* _request);
    virtual ~SendLogsToServerHandler();

    void start() override;
    void stop() override;

private:
    void onHaveLogsPath(const QString& _logsPath);
    bool getUserConsent() const;

private:
    SendLogsToServerRequest* request_;
};

}
