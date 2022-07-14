#include "stdafx.h"
#include "ErrorCommand.h"

namespace JsBridge
{
    NotExistsErrorCommand::NotExistsErrorCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : AbstractCommand(_reqId, _requestParams, _webPage)
    {}

    void NotExistsErrorCommand::execute() { setErrorCode(qsl("INTERNAL_ERROR"), qsl("Command not supported")); }

    EmptyFunctionNameErrorCommand::EmptyFunctionNameErrorCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : AbstractCommand(_reqId, _requestParams, _webPage)
    {}

    void EmptyFunctionNameErrorCommand::execute() { setErrorCode(qsl("BAD_REQUEST"), qsl("Empty command provided")); }
} // namespace JsBridge
