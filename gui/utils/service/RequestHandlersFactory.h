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

    static std::unique_ptr<RequestHandler> handlerForRequest(const std::shared_ptr<ServiceRequest>& _request);
};

}
