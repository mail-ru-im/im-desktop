#pragma once

#include <memory>

namespace Utils
{

class RequestHandler;
class ServiceRequest;

class RequestHandlersFactory
{
public:
    RequestHandlersFactory() = delete;

    static RequestHandler *handlerForRequest(ServiceRequest* _request);
};

}
