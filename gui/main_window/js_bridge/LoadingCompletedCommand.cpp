#include "stdafx.h"
#include "LoadingCompletedCommand.h"
#include "../WebAppPage.h"

namespace JsBridge
{
    LoadingCompletedCommand::LoadingCompletedCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : AbstractCommand(_reqId, _requestParams, _webPage)
        , pageIsLoaded_(_webPage->isPageLoaded())
    {}

    void LoadingCompletedCommand::execute()
    {
        if (pageIsLoaded_)
            setErrorCode(qsl("ALREADY_CALLED"), qsl("Page is already loaded"));
        else
            addSuccessParameter({});
    }
} // namespace JsBridge
