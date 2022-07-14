#include "stdafx.h"
#include "SetTitleCommand.h"
#include "../WebAppPage.h"

namespace JsBridge
{
    SetTitleCommand::SetTitleCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : AbstractCommand(_reqId, _requestParams, _webPage)
        , webPage_ { _webPage }
    {}

    void SetTitleCommand::execute()
    {
        if (webPage_)
        {
            webPage_->onSetTitle(getParamAsString(qsl("title")));
            addSuccessParameter({});
            return;
        }
        return setErrorCode(qsl("INTERNAL_ERROR"), qsl("Title did not set"));
    }
} // namespace JsBridge
