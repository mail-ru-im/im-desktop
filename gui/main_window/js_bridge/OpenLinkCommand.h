#pragma once

#include "AbstractCommand.h"

namespace JsBridge
{
    class OpenLinkCommand : public AbstractCommand
    {
        Q_OBJECT

    public:
        explicit OpenLinkCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
        void execute() override;
    };
} // namespace JsBridge
