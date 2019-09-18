#pragma once

#include <stdint.h>

namespace Utils
{

class ServiceRequest
{
public:
    enum class Type
    {
        Invalid = 0,
        SendLogsToSupportServer = 1,
        SendLogsToUin = 2,
    };

    using Id = int32_t;

public:
    ServiceRequest(Id _id);
    virtual ~ServiceRequest() = default;

    Type getType() const;
    Id getId() const;

protected:
    void setType(Type _type);

private:
    Id id_;
    Type type_;
};

}
