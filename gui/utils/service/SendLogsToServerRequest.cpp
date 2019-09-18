#include "SendLogsToServerRequest.h"

namespace Utils
{

/* Info */
SendLogsToServerRequest::Info::Info(const LogsInfo &_logsInfo)
    : logsInfo_(_logsInfo)
{

}

bool SendLogsToServerRequest::Info::isValid() const
{
    return true;
}

/* Request */
SendLogsToServerRequest::SendLogsToServerRequest(Id _id, Info _info)
    : ServiceRequest(_id),
      info_(_info)
{
    setType(ServiceRequest::Type::SendLogsToSupportServer);
}

const SendLogsToServerRequest::Info &SendLogsToServerRequest::getInfo() const
{
    return info_;
}


}
