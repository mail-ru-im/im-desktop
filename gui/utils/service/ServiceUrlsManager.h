#pragma once

#include "ServiceRequest.h"
#include "RequestHandler.h"

namespace Utils
{
class ServiceUrlsManager: public QObject
{
    Q_OBJECT

public:
    ServiceUrlsManager(QObject* _parent = nullptr);
    ~ServiceUrlsManager();

public:
    void processNewServiceURls(const QVector<QString>& _urlsVector);
    void handleServiceRequest(std::shared_ptr<ServiceRequest> _request);

private:
    void onRequestHandled(ServiceRequest::Id _id, const RequestHandler::Result& _result);

private:
    void connectToHandler(const std::unique_ptr<RequestHandler>& _handler);
    std::unique_ptr<RequestHandler>& addRequestToHandler(std::shared_ptr<ServiceRequest> _request, std::unique_ptr<RequestHandler> _handler);

    void stopAllRequests();
    void removeAllRequests();
    void removeRequest(ServiceRequest::Id _reqId);

private:
    using RequestHandlersContainer = std::unordered_map<std::shared_ptr<ServiceRequest>, std::unique_ptr<RequestHandler>>;

    RequestHandlersContainer requestToHandler_;
};

}
