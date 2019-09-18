#pragma once

#include <QString>

namespace Utils
{
    class ServiceRequest;

    class ServiceRequestsFactory
    {
    public:
        ServiceRequestsFactory() = delete;

        static ServiceRequest* requestForUrl(const QString& _urlCommand);
    };
}
