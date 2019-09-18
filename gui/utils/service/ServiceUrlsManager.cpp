#include "stdafx.h"

#include "ServiceUrlsManager.h"
#include "ServiceRequestsFactory.h"
#include "RequestHandlersFactory.h"

#include "ServiceRequest.h"
#include "RequestHandler.h"

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
        assert(request && "couldn't parse service request");
        if (!request)
            continue;

        handleServiceRequest(std::move(request));
    }
}

void ServiceUrlsManager::handleServiceRequest(ServiceRequest* _request)
{
    auto handler = RequestHandlersFactory::handlerForRequest(_request);
    assert(handler && "couldn't get handler for request");
    if (!handler)
        return;

    connectToHandler(handler);
    addRequestToHandler(_request, handler);

    handler->start();
}

void ServiceUrlsManager::onRequestHandled(ServiceRequest::Id _id, const RequestHandler::Result& _result)
{
    removeRequest(_id);
}

void ServiceUrlsManager::connectToHandler(RequestHandler *_handler)
{
    connect(_handler, &RequestHandler::finished,
            this, &ServiceUrlsManager::onRequestHandled);
}

void ServiceUrlsManager::addRequestToHandler(ServiceRequest *_request, RequestHandler *_handler)
{
    requestToHandler_[_request] = _handler;
}

void ServiceUrlsManager::stopAllRequests()
{
    for (auto it: requestToHandler_)
    {
        it.second->stop();
    }
}

void ServiceUrlsManager::removeAllRequests()
{
    for (auto [request, handler]: requestToHandler_)
    {
        delete request;
        handler->deleteLater();
    }

    requestToHandler_.clear();
}

void ServiceUrlsManager::removeRequest(ServiceRequest::Id _reqId)
{
    auto it = std::find_if(requestToHandler_.begin(), requestToHandler_.end(),
                           [_reqId](const RequestHandlersContainer::value_type& val)
    {
        return val.first->getId() == _reqId;
    });

    if (it != requestToHandler_.end())
        requestToHandler_.erase(it);
}

}
