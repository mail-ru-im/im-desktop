#include "SendLogsToUinRequest.h"
#include "InternalServiceRequest.h"

namespace Utils
{
    InternalServiceRequest::InternalServiceRequest(ServiceRequest::Id _id, CommandType _type)
        : ServiceRequest(_id)
        , type_(_type)
    {
        setType(ServiceRequest::Type::Internal);
    }
}
