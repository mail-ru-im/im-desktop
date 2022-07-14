#include "stdafx.h"
#include "OpenLinkCommand.h"
#include "../../utils/UrlUtils.h"
#include "../../utils/UrlParser.h"

namespace JsBridge
{
    OpenLinkCommand::OpenLinkCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : AbstractCommand(_reqId, _requestParams, _webPage)
    {}

    void OpenLinkCommand::execute()
    {
        if (const QString url = getParamAsString(qsl("url")); !url.isEmpty())
        {
            Utils::UrlParser p;
            p.process(url);
            if (!p.hasUrl())
            {
                setErrorCode(qsl("BAD_REQUEST"), qsl("Invalid url provided"));
                return;
            }

            Utils::openUrl(url, Utils::OpenUrlConfirm::Yes);
            addSuccessParameter({});
            return;
        }
        setErrorCode(qsl("BAD_REQUEST"), qsl("No url provided"));
    }
} // namespace JsBridge
