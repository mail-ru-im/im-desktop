#pragma once

#include "AbstractCommand.h"

namespace JsBridge
{
    class NotExistsErrorCommand : public AbstractCommand
    {
        Q_OBJECT

    public:
        explicit NotExistsErrorCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
        void execute() override;
    };

    class EmptyFunctionNameErrorCommand : public AbstractCommand
    {
        Q_OBJECT

    public:
        explicit EmptyFunctionNameErrorCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
        void execute() override;
    };
} // namespace JsBridge
