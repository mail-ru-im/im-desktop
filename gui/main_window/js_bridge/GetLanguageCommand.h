#pragma once

#include "AbstractCommand.h"

namespace JsBridge
{
    class GetLanguageCommand : public AbstractCommand
    {
        Q_OBJECT

    public:
        explicit GetLanguageCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
        void execute() override;
    };
} // namespace JsBridge
