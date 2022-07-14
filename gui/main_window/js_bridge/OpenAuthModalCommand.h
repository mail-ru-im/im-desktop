#pragma once

#include "AbstractCommand.h"

namespace JsBridge
{
    class OpenAuthModalCommand : public AbstractCommand
    {
        Q_OBJECT

    public:
        explicit OpenAuthModalCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
        void execute() override;

    private Q_SLOTS:
        void onAuthResponse(const QString& _requestId, const QString& _redirectUrlQuery);
        void onResponseTimeout();

    private:
        QTimer* timer_;
        QString requestId_;
    };
} // namespace JsBridge
