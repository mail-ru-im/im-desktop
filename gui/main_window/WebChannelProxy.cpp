#include "stdafx.h"

#include "main_window/WebAppPage.h"
#include "main_window/GroupChatOperations.h"
#include "main_window/mini_apps/MiniAppsUtils.h"

namespace Ui
{
    void WebChannelProxy::callSDKFunction(const QJsonObject& _action)
    {
        QString dataType = qsl("data");
        if (!_action.contains(dataType) && !_action[dataType].isObject())
            return;

        QString reqId, funcName;
        QJsonObject requestParams;
        const auto internalData = _action[dataType].toObject();

        if (dataType = qsl("reqId"); internalData.contains(dataType) && internalData[dataType].isString())
            reqId = internalData[dataType].toString();

        if (dataType = qsl("args"); internalData.contains(dataType) && internalData[dataType].isArray())
        {
            const auto args = internalData[dataType].toArray();
            if (args.size() > 1)
            {
                funcName = args.first().toString();
                requestParams = args[1].toObject();
            }

            Q_EMIT functionCalled(reqId, funcName, requestParams);
        }
    }

    void WebChannelProxy::postEvent(const QJsonObject& _action)
    {
        QString id;
        QString dataType;
        SidebarContentType infoType = SidebarContentType::person;

        if (dataType = qsl("type"); _action.contains(dataType) && _action[dataType].isString())
        {
            const auto theType = _action[dataType].toString();
            if (theType == u"functionCall")
            {
                callSDKFunction(_action);
                return;
            }
        }

        if (dataType = qsl("event"); _action.contains(dataType) && _action[dataType].isString())
        {
            const auto event = _action[dataType].toString();
            if (event == u"createTask")
            {
                Q_EMIT createTask();
                return;
            }
            else if (event == u"editTask" || event == u"forwardTask")
            {
                if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
                {
                    const auto data = _action[dataType].toObject();
                    if (dataType = qsl("taskId"); data.contains(dataType) && data[dataType].isString())
                        id = data[dataType].toString();
                }

                if (event == u"editTask")
                    Q_EMIT editTask(id);
                else
                    Q_EMIT forwardTask(id);

                return;
            }
            else if (event == u"message")
            {
                if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
                {
                    const auto data = _action[dataType].toObject();
                    if (dataType = qsl("userId"); data.contains(dataType) && data[dataType].isString())
                    {
                        Utils::InterConnector::instance().openDialog(data[dataType].toString());
                        return;
                    }
                }
            }
            else if (event == u"call")
            {
                if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
                {
                    const auto data = _action[dataType].toObject();
                    if (dataType = qsl("userId"); data.contains(dataType) && data[dataType].isString())
                    {
                        const auto sn = data[dataType].toString();
                        auto video = false;
                        if (dataType = qsl("video"); data.contains(dataType) && data[dataType].isBool())
                            video = data[dataType].toBool();

                        doVoipCall(sn, video ? CallType::Video : CallType::Audio, CallPlace::Profile);
                        return;
                    }
                }
            }
            else if (event == u"setTitle")
            {
                if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
                {
                    const auto data = _action[dataType].toObject();
                    if (dataType = qsl("title"); data.contains(dataType) && data[dataType].isString())
                    {
                        Q_EMIT setTitle(data[dataType].toString());
                        return;
                    }
                }
            }
            else if (event == u"setLeftButton" || event == u"setRightButton")
            {
                if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
                {
                    const auto data = _action[dataType].toObject();
                    QString text, color;
                    bool enabled = true;
                    if (dataType = qsl("name"); data.contains(dataType) && data[dataType].isString())
                        text = data[dataType].toString();

                    if (dataType = qsl("color"); data.contains(dataType) && data[dataType].isString())
                        color = data[dataType].toString();

                    if (dataType = qsl("disable"); data.contains(dataType) && data[dataType].isBool())
                        enabled = !data[dataType].toBool();

                    if (event == u"setLeftButton")
                        Q_EMIT setLeftButton(text, color, enabled);
                    else
                        Q_EMIT setRightButton(text, color, enabled);
                }
            }
            else if (event == u"initLoadingCompleted")
            {
                if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
                {
                    auto isLoadedSuccessfully = false;
                    const auto data = _action[dataType].toObject();
                    if (dataType = qsl("success"); data.contains(dataType))
                        isLoadedSuccessfully = data[dataType].toBool();

                    Q_EMIT pageLoadFinished(isLoadedSuccessfully);
                }
                return;
            }
            else if (event == u"mailClick")
            {
                if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
                {
                    const auto data = _action[dataType].toObject();
                    if (dataType = Utils::MiniApps::getMailId(); data.contains(dataType) && data[dataType].isString())
                        Q_EMIT Utils::InterConnector::instance().composeEmail(data[dataType].toString());
                }
                return;
            }
        }

        if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
        {
            const auto data = _action[dataType].toObject();
            if (dataType = qsl("sn"); data.contains(dataType) && data[dataType].isString())
            {
                id = data[dataType].toString();
                infoType = SidebarContentType::person;
            }

            if (dataType = qsl("threadId"); data.contains(dataType) && data[dataType].isString())
            {
                id = data[dataType].toString();
                infoType = data.contains(qsl("taskId")) ? SidebarContentType::task : SidebarContentType::thread;
            }
        }

        if (id.isEmpty())
            return;

        Q_EMIT showInfo(id, infoType);
    }
}