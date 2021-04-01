#include "stdafx.h"
#include "ServiceRequestsFactory.h"

#include <QUrl>
#include <QUrlQuery>

#include "ServiceRequest.h"
#include "SendLogsToServerRequest.h"
#include "SendLogsToUinRequest.h"
#include "InternalServiceRequest.h"
#include "utils/utils.h"

namespace Utils
{

namespace
{
    constexpr int32_t LIMIT_LOG_FILES_TO_SEND = 10;
    constexpr int32_t LIMIT_RTP_DUMPS_FILES_TO_SEND = 3;

    std::unique_ptr<ServiceRequest> buildSendLogsToServerRequest(ServiceRequest::Id _id, bool _withRTP)
    {
        // NOTE: artificial limit
        Utils::LogsInfo logsInfo(LIMIT_LOG_FILES_TO_SEND, _withRTP ? LIMIT_RTP_DUMPS_FILES_TO_SEND : -1);
        SendLogsToServerRequest::Info info(logsInfo);
        if (!info.isValid())
            return nullptr;

        return std::make_unique<SendLogsToServerRequest>(_id, info);
    }

    std::unique_ptr<ServiceRequest> buildSendLogsToUinRequest(ServiceRequest::Id _id, const QString& _uin, bool _withRTP)
    {
        // NOTE: artificial limit
        Utils::LogsInfo logsInfo(LIMIT_LOG_FILES_TO_SEND, _withRTP ? LIMIT_RTP_DUMPS_FILES_TO_SEND : -1);

        SendLogsToUinRequest::Info info(_uin, logsInfo);
        if (!info.isValid())
            return nullptr;

        return std::make_unique<SendLogsToUinRequest> (_id, info);
    }
}

std::unique_ptr<ServiceRequest> ServiceRequestsFactory::requestForUrl(const QString &_urlCommand)
{
    static std::atomic<ServiceRequest::Id> serviceRequestId(0);

    const QUrl url(_urlCommand);
    const QString host = url.host();
    const QUrlQuery urlQuery(url);

    const auto items = urlQuery.queryItems();
    Q_UNUSED(items);
    const QString path = url.path();
    QStringRef pathRef(&path);

    im_assert(host == url_command_service());
    if (host != url_command_service())
        return nullptr;

    if (pathRef.startsWith(u'/'))
        pathRef = pathRef.mid(1);

    if (const bool withRTP = pathRef.startsWith(url_commandpath_service_getlogs_with_rtp()); withRTP || pathRef.startsWith(url_commandpath_service_getlogs()))
    {
        ++serviceRequestId;

        auto logsPath = pathRef.split(u'/');
        logsPath.removeFirst();
        if (!logsPath.size())
            // root request => send to support
            return buildSendLogsToServerRequest(serviceRequestId, withRTP);

        auto uin = logsPath.constFirst().toString();
        return buildSendLogsToUinRequest(serviceRequestId, uin, withRTP);
    }
    else if (pathRef.startsWith(url_commandpath_service_clear_cache()))
    {
        return std::make_unique<InternalServiceRequest>(serviceRequestId, InternalServiceRequest::CommandType::ClearCache);
    }
    else if (pathRef.startsWith(url_commandpath_service_clear_avatars()))
    {
        return std::make_unique<InternalServiceRequest>(serviceRequestId, InternalServiceRequest::CommandType::ClearAvatars);
    }
    else if (pathRef.startsWith(url_commandpath_service_debug()))
    {
        return std::make_unique<InternalServiceRequest>(serviceRequestId, InternalServiceRequest::CommandType::OpenDebug);
    }
    else if (pathRef.startsWith(url_commandpath_service_update()))
    {
        return std::make_unique<InternalServiceRequest>(serviceRequestId, InternalServiceRequest::CommandType::CheckForUpdate);
    }

    im_assert(!"unknown service command");
    return nullptr;
}

}
