#pragma once

#include "RequestHandler.h"

namespace Utils
{

class InternalServiceRequest;

class InternalServiceHandler : public RequestHandler
{
public:
    InternalServiceHandler(InternalServiceRequest* _request);
    virtual ~InternalServiceHandler();

    void start() override;
    void stop() override;

private:
    InternalServiceRequest* request_;
};

}
