#include "stdafx.h"
#include "OpenAuthModalCommand.h"
#include "../../core_dispatcher.h"
#include "../../utils/InterConnector.h"
#include "../../utils/UrlUtils.h"
#include "../../../gui.shared/constants.h"

namespace
{
    auto redirectUrlHost(const QString& _path) noexcept
    {
        const auto& urlShemes = Utils::urlSchemes();
        QString url = !urlShemes.empty() ? urlShemes.at(0) : QString {};
        url += u"://";
        url += QString::fromUtf16(url_command_open_auth_modal_response());
        url += u'/';
        url += _path;
        return url;
    }

    constexpr std::chrono::milliseconds authResponceTimeout = std::chrono::minutes(3);
} // namespace

namespace JsBridge
{
    OpenAuthModalCommand::OpenAuthModalCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : AbstractCommand(_reqId, _requestParams, _webPage)
        , timer_ { new QTimer(this) }
    {
        timer_->setSingleShot(true);
    }

    void OpenAuthModalCommand::execute()
    {
        const QString urlString = getParamAsString(qsl("url"));
        if (urlString.isEmpty())
        {
            setErrorCode(qsl("BAD_REQUEST"), qsl("No url provided"));
            return;
        }

        const QString redirectUrlParamName = getParamAsString(qsl("redirectUrlParamName"));
        if (redirectUrlParamName.isEmpty())
        {
            setErrorCode(qsl("BAD_REQUEST"), qsl("No redirectUrlParamName provided"));
            return;
        }

        requestId_ = QString::number(qHash(reqId()));
        QUrl url = Utils::addQueryItemsToUrl(
            QUrl { urlString },
            QUrlQuery { { redirectUrlParamName, redirectUrlHost(requestId_) } },
            Utils::AddQueryItemPolicy::ReplaceIfItemExists);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::onOpenAuthModalResponse, this, &OpenAuthModalCommand::onAuthResponse);
        connect(timer_, &QTimer::timeout, this, &OpenAuthModalCommand::onResponseTimeout);
        timer_->start(authResponceTimeout);
        Utils::openUrl(url.toString());
    }

    void OpenAuthModalCommand::onAuthResponse(const QString& _requestId, const QString& _redirectUrlQuery)
    {
        if (_requestId != requestId_)
            return;
        timer_->stop();
        addSuccessParameter(QJsonObject { { QPair { qsl("querystring"), QUrl::fromPercentEncoding(_redirectUrlQuery.toUtf8()) } } });
    }

    void OpenAuthModalCommand::onResponseTimeout() { setErrorCode(qsl("MODAL_CLOSED"), qsl("Modal timeout")); }
} // namespace JsBridge
