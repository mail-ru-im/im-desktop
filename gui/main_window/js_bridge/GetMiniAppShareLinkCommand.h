#pragma once

#include "AbstractCommand.h"

namespace JsBridge
{
    class GetMiniAppShareLinkCommand : public AbstractCommand
    {
        Q_OBJECT

    public:
        explicit GetMiniAppShareLinkCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
        void execute() override;

    private:
        QString miniAppId_;
    };
} // namespace JsBridge
