#pragma once

namespace Ui
{
    enum class SidebarContentType;

    class WebChannelProxy : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void showInfo(const QString& _aimId, const SidebarContentType _infoType);
        void createTask();
        void editTask(const QString& _taskId);
        void forwardTask(const QString& _taskId);
        void setTitle(const QString& _title);
        void setLeftButton(const QString& _text, const QString& _color, bool _enabled);
        void setRightButton(const QString& _text, const QString& _color, bool _enabled);
        void pageLoadFinished(bool _success);
        void functionCalled(const QString& _reqId, const QString& _name, const QJsonObject& _requestParams);

    public:
        explicit WebChannelProxy(QObject* _parent = nullptr)
            : QObject(_parent)
        {
        }

    public Q_SLOTS:
        void postEvent(const QJsonObject& _action);

    private:
        void callSDKFunction(const QJsonObject& _action);
    };
}
