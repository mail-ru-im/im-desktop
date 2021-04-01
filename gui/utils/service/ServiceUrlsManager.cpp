#include "stdafx.h"

#include "ServiceUrlsManager.h"
#include "ServiceRequestsFactory.h"
#include "RequestHandlersFactory.h"

namespace Utils
{

ServiceUrlsManager::ServiceUrlsManager(QObject *_parent)
    : QObject(_parent)
{

}

ServiceUrlsManager::~ServiceUrlsManager()
{
    stopAllRequests();
    removeAllRequests();
}

void ServiceUrlsManager::processNewServiceURls(const QVector<QString> &_urlsVector)
{
    for (const auto& urlString: _urlsVector)
    {
        auto request = ServiceRequestsFactory::requestForUrl(urlString);
        im_assert(request && "couldn't parse service request");
        if (!request)
            continue;

        handleServiceRequest(std::move(request));
    }
}

void ServiceUrlsManager::handleServiceRequest(std::shared_ptr<ServiceRequest> _request)
{
    auto handler = RequestHandlersFactory::handlerForRequest(_request);
    im_assert(handler && "couldn't get handler for request");
    if (!handler)
        return;

    connectToHandler(handler);
    addRequestToHandler(std::move(_request), std::move(handler))->start();
}

void ServiceUrlsManager::onRequestHandled(ServiceRequest::Id _id, const RequestHandler::Result& _result)
{
    removeRequest(_id);
}

void ServiceUrlsManager::connectToHandler(const std::unique_ptr<RequestHandler>& _handler)
{
    connect(_handler.get(), &RequestHandler::finished,
            this, &ServiceUrlsManager::onRequestHandled);
}

std::unique_ptr<RequestHandler>& ServiceUrlsManager::addRequestToHandler(std::shared_ptr<ServiceRequest> _request, std::unique_ptr<RequestHandler> _handler)
{
    auto& x = requestToHandler_[std::move(_request)];
    x = std::move(_handler);
    return x;
}

void ServiceUrlsManager::stopAllRequests()
{
    for (auto& it: requestToHandler_)
        it.second->stop();
}

void ServiceUrlsManager::removeAllRequests()
{
    requestToHandler_.clear();
}

void ServiceUrlsManager::removeRequest(ServiceRequest::Id _reqId)
{
    const auto it = std::find_if(requestToHandler_.cbegin(), requestToHandler_.cend(),
                           [_reqId](const auto& val)
    {
        return val.first->getId() == _reqId;
    });

    if (it != requestToHandler_.cend())
        requestToHandler_.erase(it);
}

}
