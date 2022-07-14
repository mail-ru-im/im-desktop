#pragma once

namespace Ui
{
    class WebAppPage;
}

namespace JsBridge
{
    class AbstractCommand : public QObject
    {
        Q_OBJECT

    public:
        explicit AbstractCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
        virtual ~AbstractCommand() = default;

        virtual void execute() = 0;

    Q_SIGNALS:
        void commandReady(const QJsonObject& _reply, QPrivateSignal);

    protected:
        QString getParamAsString(const QString& _paramName) const;
        void addSuccessParameter(const QJsonObject& _successParams);
        void setErrorCode(const QString& _code, const QString& _reason);
        const QString& reqId() const;

    private:
        const QJsonObject requestParams_;
        const QString reqId_;
    };

    // Caller side takes ownership of command. Don't forget to remove returned one
    AbstractCommand* makeCommand(const QString& _reqId, const QString& _name, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
} // namespace JsBridge
