#include "ServiceRequest.h"

namespace  Utils
{

ServiceRequest::ServiceRequest(Id _id)
    : id_(_id),
      type_(ServiceRequest::Type::Invalid)
{

}

ServiceRequest::Type ServiceRequest::getType() const
{
    return type_;
}

ServiceRequest::Id ServiceRequest::getId() const
{
    return id_;
}

void ServiceRequest::setType(ServiceRequest::Type _type)
{
    type_ = _type;
}

}
