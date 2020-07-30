#pragma once

#include <QString>

namespace Utils
{
    class ServiceRequest;

    class ServiceRequestsFactory
    {
    public:
        ServiceRequestsFactory() = delete;

        static std::unique_ptr<ServiceRequest> requestForUrl(const QString& _urlCommand);
    };
}
