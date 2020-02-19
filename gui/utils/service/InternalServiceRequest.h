#pragma once

#include "ServiceRequest.h"

namespace Utils
{

class InternalServiceRequest : public ServiceRequest
{
public:
    enum class CommandType
    {
        ClearAvatars = 0,
        ClearCache = 1,
        OpenDebug = 2,
        CheckForUpdate = 3,
    };

public:
    InternalServiceRequest(ServiceRequest::Id _id, CommandType _type);
    virtual ~InternalServiceRequest() = default;

    CommandType getCommandType() const { return type_; };

private:
    CommandType type_;
};

}
