#include "stdafx.h"

#include "AbstractCommand.h"

#include "ErrorCommand.h"
#include "GetLanguageCommand.h"
#include "GetMiniAppShareLinkCommand.h"
#include "GetThemeSettingsCommand.h"
#include "LoadingCompletedCommand.h"
#include "OpenAuthModalCommand.h"
#include "OpenLinkCommand.h"
#include "OpenResourceCommand.h"
#include "SetTitleCommand.h"

#include "../WebAppPage.h"
#include "../../utils/ObjectFactory.h"

namespace
{
    QStringView requestKey() noexcept { return u"reqId"; }
    QStringView successObjectKey() noexcept { return u"ok"; }
    QStringView errorObjectKey() noexcept { return u"err"; }

    using CommandFactory = Utils::ObjectFactory<JsBridge::AbstractCommand, QString, const QString& /*_reqId*/, const QJsonObject& /*_requestParams*/, Ui::WebAppPage* /*_webPage*/>;
    CommandFactory createFactory()
    {
        CommandFactory factory;
        factory.registerClass<JsBridge::LoadingCompletedCommand>(qsl("LoadingCompleted"));
        factory.registerClass<JsBridge::GetLanguageCommand>(qsl("GetLanguage"));
        factory.registerClass<JsBridge::GetMiniAppShareLinkCommand>(qsl("GetMiniAppShareLink"));
        factory.registerClass<JsBridge::GetThemeSettingsCommand>(qsl("GetThemeSettings"));
        factory.registerClass<JsBridge::OpenProfileCommand>(qsl("OpenProfile"));
        factory.registerClass<JsBridge::OpenDialogCommand>(qsl("OpenDialog"));
        factory.registerClass<JsBridge::SetTitleCommand>(qsl("SetTitle"));
        factory.registerClass<JsBridge::OpenLinkCommand>(qsl("OpenLink"));
        factory.registerClass<JsBridge::OpenAuthModalCommand>(qsl("OpenAuthModal"));
        factory.registerClass<JsBridge::EmptyFunctionNameErrorCommand>(QString {});
        factory.registerDefaultClass<JsBridge::NotExistsErrorCommand>();
        return factory;
    };
} // namespace

namespace JsBridge
{
    AbstractCommand::AbstractCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage*)
        : requestParams_ { _requestParams }
        , reqId_ { _reqId }
    {}

    QString AbstractCommand::getParamAsString(const QString& _paramName) const
    {
        if (requestParams_.contains(_paramName) && requestParams_[_paramName].isString())
            return requestParams_[_paramName].toString();

        return {};
    }

    void AbstractCommand::addSuccessParameter(const QJsonObject& _successParams) {
        QJsonObject result;
        result[requestKey()] = reqId_;
        result[successObjectKey()] = _successParams;
        Q_EMIT commandReady(result, QPrivateSignal{});
    }

    void AbstractCommand::setErrorCode(const QString& _code, const QString& _reason)
    {
        QJsonObject result;
        result[requestKey()] = reqId_;
        result[errorObjectKey()] = QJsonObject({ QPair { qsl("code"), _code }, QPair { qsl("reason"), _reason } });
        Q_EMIT commandReady(result, QPrivateSignal{});
    }

    const QString& AbstractCommand::reqId() const { return reqId_; }

    AbstractCommand* makeCommand(const QString& _reqId, const QString& _name, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
    {
        static CommandFactory factory = createFactory();
        return factory.create(_name, _reqId, _requestParams, _webPage);
    }
} // namespace JsBridge
