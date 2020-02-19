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

RequestHandler* RequestHandlersFactory::handlerForRequest(ServiceRequest *_request)
{
    if (!_request)
        return nullptr;

    switch (_request->getType())
    {
    case ServiceRequest::Type::SendLogsToUin:
    {
        auto uinRequest = dynamic_cast<SendLogsToUinRequest *>(_request);
        assert(uinRequest);
        if (!uinRequest)
            return nullptr;

        return new SendLogsToUinHandler(uinRequest);
    }
        break;
    case ServiceRequest::Type::SendLogsToSupportServer:
    {
        auto toServerRequest = dynamic_cast<SendLogsToServerRequest *>(_request);
        assert(toServerRequest);
        if (!toServerRequest)
            return nullptr;

        return new SendLogsToServerHandler(toServerRequest);
    }
        break;
    case ServiceRequest::Type::Internal:
    {
        auto internalRequest = dynamic_cast<InternalServiceRequest *>(_request);
        assert(internalRequest);
        if (!internalRequest)
            return nullptr;
        return new InternalServiceHandler(internalRequest);
    }
        break;
    default:
        assert(!"handle enum ServiceRequest::Type in handler factory");
    }

    return nullptr;
}

}
