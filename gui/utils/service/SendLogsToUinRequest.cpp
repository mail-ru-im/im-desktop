#include "SendLogsToUinRequest.h"

namespace Utils
{

/* Info */

SendLogsToUinRequest::Info::Info(const QString &_uin, const LogsInfo& _logsInfo)
    : uin_(_uin),
      logsInfo_(_logsInfo)
{

}

bool SendLogsToUinRequest::Info::isValid() const
{
    return !uin_.isEmpty();
}

/* Request */
SendLogsToUinRequest::SendLogsToUinRequest(Id _id, const Info &_info)
    : ServiceRequest(_id),
      info_(_info)
{
    setType(ServiceRequest::Type::SendLogsToUin);
}

const SendLogsToUinRequest::Info& SendLogsToUinRequest::getInfo() const
{
    return info_;
}

}
