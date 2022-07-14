#pragma once

#include "AbstractCommand.h"

namespace JsBridge
{
    class GetThemeSettingsCommand : public AbstractCommand
    {
        Q_OBJECT

    public:
        explicit GetThemeSettingsCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
        void execute() override;
    };
} // namespace JsBridge
