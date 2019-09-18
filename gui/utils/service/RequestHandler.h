#pragma once

#include <QObject>
#include "ServiceRequest.h"

namespace Utils
{
    class RequestHandler: public QObject
    {
        Q_OBJECT

    public:
        struct Result
        {
            enum class Status
            {
                Success = 0,
                Failed = 1,

            };

            Status status_;
        };

    public:
        RequestHandler(QObject* _parent = nullptr);
         ~RequestHandler();

        virtual void start() = 0;
        virtual void stop() = 0;

    Q_SIGNALS:
        void finished(ServiceRequest::Id _id, const Result& _result);

    private:

    };
}
