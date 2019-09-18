#include "stdafx.h"
#include "ServiceRequestsFactory.h"

#include <QUrl>
#include <QUrlQuery>

#include "ServiceRequest.h"
#include "SendLogsToServerRequest.h"
#include "SendLogsToUinRequest.h"

namespace Utils
{

namespace
{
constexpr int32_t LIMIT_LOG_FILES_TO_SEND = 10;
constexpr int32_t LIMIT_RTP_DUMPS_FILES_TO_SEND = 3;

SendLogsToServerRequest* buildSendLogsToServerRequest(ServiceRequest::Id _id, bool _withRTP);
SendLogsToUinRequest* buildSendLogsToUinRequest(ServiceRequest::Id _id, const QString& _uin, bool _withRTP);
}

ServiceRequest* ServiceRequestsFactory::requestForUrl(const QString &_urlCommand)
{
    static std::atomic<ServiceRequest::Id> serviceRequestId(0);

    const QUrl url(_urlCommand);
    const QString host = url.host();
    const QUrlQuery urlQuery(url);

    const auto items = urlQuery.queryItems();
    Q_UNUSED(items);
    const QString path = url.path();
    QStringRef pathRef(&path);

    assert(host == qsl(url_command_service));
    if (host != qsl(url_command_service))
        return nullptr;

    if (pathRef.startsWith(ql1c('/')))
        pathRef = pathRef.mid(1);

    if (pathRef.startsWith(qsl(url_commandpath_service_getlogs)) || pathRef.startsWith(qsl(url_commandpath_service_getlogs_with_rtp)))
    {
        bool withRTP = pathRef.startsWith(qsl(url_commandpath_service_getlogs_with_rtp));

        serviceRequestId++;

        auto logsPath = pathRef.split(ql1c('/'));
        logsPath.removeFirst();
        if (!logsPath.size())
            // root request => send to support
            return buildSendLogsToServerRequest(serviceRequestId, withRTP);

        auto uin = logsPath.constFirst().toString();
        return buildSendLogsToUinRequest(serviceRequestId, uin, withRTP);
    }

    assert(!"unknown service command");
    return nullptr;
}

namespace
{

SendLogsToServerRequest* buildSendLogsToServerRequest(ServiceRequest::Id _id, bool _withRTP)
{
    // NOTE: artificial limit
    Utils::LogsInfo logsInfo(LIMIT_LOG_FILES_TO_SEND, _withRTP ? LIMIT_RTP_DUMPS_FILES_TO_SEND : -1);
    SendLogsToServerRequest::Info info(logsInfo);
    if (!info.isValid())
        return nullptr;

    return new SendLogsToServerRequest(_id, info);
}

SendLogsToUinRequest* buildSendLogsToUinRequest(ServiceRequest::Id _id, const QString& _uin, bool _withRTP)
{
    // NOTE: artificial limit
    Utils::LogsInfo logsInfo(LIMIT_LOG_FILES_TO_SEND, _withRTP ? LIMIT_RTP_DUMPS_FILES_TO_SEND : -1);

    SendLogsToUinRequest::Info info(_uin, logsInfo);
    if (!info.isValid())
        return nullptr;

    return new SendLogsToUinRequest(_id, info);
}

}

}
