#pragma once

#include "RequestHandler.h"

namespace Utils
{

class InternalServiceRequest;

class InternalServiceHandler : public RequestHandler
{
public:
    InternalServiceHandler(std::shared_ptr<InternalServiceRequest> _request);
    virtual ~InternalServiceHandler();

    void start() override;
    void stop() override;

private:
    std::shared_ptr<InternalServiceRequest> request_;
};

}
