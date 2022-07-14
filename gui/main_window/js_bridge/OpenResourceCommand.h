#pragma once

#include "AbstractCommand.h"

namespace Ui
{
    class WebAppPage;
}

namespace Data
{
    struct IdInfo;
}

namespace JsBridge
{
    class OpenResourceCommand : public AbstractCommand
    {
        Q_OBJECT

    public:
        explicit OpenResourceCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);

    protected:
        const QString& chatId() const;
        void execute() final;
        virtual void setNotFoundErrorCode() = 0;
        virtual void onIdInfoSuccessResponse(const QString& _aimId) = 0;

    private Q_SLOTS:
        void onGetIdInfo(const qint64 _seq, const Data::IdInfo& _idInfo);
        void onTimeout();

    private:
        QTimer* timer_;
        QString chatId_;
        qint64 seq_ = 0;
    };

    class OpenProfileCommand : public OpenResourceCommand
    {
        Q_OBJECT

    public:
        explicit OpenProfileCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);

    protected:
        void setNotFoundErrorCode() override;
        void onIdInfoSuccessResponse(const QString& _aimId) override;

    private:
        Ui::WebAppPage* webPage_;
    };

    class OpenDialogCommand : public OpenResourceCommand
    {
        Q_OBJECT

    public:
        explicit OpenDialogCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);

    protected:
        void setNotFoundErrorCode() override;
        void onIdInfoSuccessResponse(const QString& _aimId) override;
    };
} // namespace JsBridge
