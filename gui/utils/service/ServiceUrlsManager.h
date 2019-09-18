#pragma once

#include <QObject>
#include <unordered_map>

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

public Q_SLOTS:
    void processNewServiceURls(const QVector<QString>& _urlsVector);
    void handleServiceRequest(ServiceRequest* _request);

private Q_SLOTS:
    void onRequestHandled(ServiceRequest::Id _id, const RequestHandler::Result& _result);

private:
    void connectToHandler(RequestHandler* _handler);
    void addRequestToHandler(ServiceRequest* _request, RequestHandler* _handler);

    void stopAllRequests();
    void removeAllRequests();
    void removeRequest(ServiceRequest::Id _reqId);

private:
    using RequestHandlersContainer = std::unordered_map<ServiceRequest*, RequestHandler *>;

    RequestHandlersContainer requestToHandler_;
};

}
