#pragma once

#include "RequestHandler.h"

namespace Utils
{

class SendLogsToUinRequest;

class SendLogsToUinHandler: public RequestHandler
{
public:
    SendLogsToUinHandler(SendLogsToUinRequest* _request);
    virtual ~SendLogsToUinHandler() = default;

    void start() override;
    void stop() override;

private:
    void sendFileToContact(const QString& _filePath, const QString& _contact);
    void onHaveLogsPath(const QString& _logsPath);

    bool getUserConsent() const;
    bool uinIsAllowed(const QString& _contactUin) const;

private:
    SendLogsToUinRequest* request_;
};

}
