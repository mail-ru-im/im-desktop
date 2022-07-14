#include "stdafx.h"
#include "GetMiniAppShareLinkCommand.h"
#include "../WebAppPage.h"
#include "../../url_config.h"
#include "../../utils/UrlUtils.h"

namespace JsBridge
{
    GetMiniAppShareLinkCommand::GetMiniAppShareLinkCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : AbstractCommand(_reqId, _requestParams, _webPage)
    {
        if (const auto appId = getParamAsString(qsl("miniappId")); !appId.isEmpty())
            miniAppId_ = appId;
        else
            miniAppId_ = _webPage->type();
    }

    void GetMiniAppShareLinkCommand::execute()
    {
        if (miniAppId_.isEmpty())
        {
            setErrorCode(qsl("INTERNAL_ERROR"), qsl("Mini-app id not provided"));
            return;
        }

        const auto& sharingUrl = Ui::getUrlConfig().getMiniappSharing();
        if (sharingUrl.isEmpty())
        {
            setErrorCode(qsl("INTERNAL_ERROR"), qsl("Mini-app share url not appears in config"));
            return;
        }

        QString miniAppUrlString = Utils::addHttpsIfNeeded(sharingUrl);
        if (!miniAppUrlString.endsWith(u'/'))
            miniAppUrlString += u'/';
        miniAppUrlString += miniAppId_;

        addSuccessParameter(QJsonObject { { QPair { qsl("url"), miniAppUrlString } } });
    }
} // namespace JsBridge
