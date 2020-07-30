#include "stdafx.h"
#include "RequestHandlersFactory.h"

#include "ServiceRequest.h"
#include "SendLogsToServerRequest.h"
#include "SendLogsToUinRequest.h"
#include "InternalServiceRequest.h"

#include "RequestHandler.h"
#include "SendLogsToServerHandler.h"
#include "SendLogsToUinHandler.h"
#include "InternalServiceHandler.h"


namespace Utils
{

std::unique_ptr<RequestHandler> RequestHandlersFactory::handlerForRequest(const std::shared_ptr<ServiceRequest>& _request)
{
    if (!_request)
        return nullptr;

    switch (_request->getType())
    {
    case ServiceRequest::Type::SendLogsToUin:
    {
        auto uinRequest = std::dynamic_pointer_cast<SendLogsToUinRequest>(_request);
        assert(uinRequest);
        if (!uinRequest)
            return nullptr;

        return std::make_unique<SendLogsToUinHandler>(std::move(uinRequest));
    }
        break;
    case ServiceRequest::Type::SendLogsToSupportServer:
    {
        auto toServerRequest = std::dynamic_pointer_cast<SendLogsToServerRequest>(_request);
        assert(toServerRequest);
        if (!toServerRequest)
            return nullptr;

        return std::make_unique<SendLogsToServerHandler>(std::move(toServerRequest));
    }
        break;
    case ServiceRequest::Type::Internal:
    {
        auto internalRequest = std::dynamic_pointer_cast<InternalServiceRequest>(_request);
        assert(internalRequest);
        if (!internalRequest)
            return nullptr;
        return std::make_unique<InternalServiceHandler>(std::move(internalRequest));
    }
        break;
    default:
        assert(!"handle enum ServiceRequest::Type in handler factory");
    }

    return nullptr;
}

}
