#pragma once

#include "AbstractCommand.h"

namespace JsBridge
{
    class LoadingCompletedCommand : public AbstractCommand
    {
        Q_OBJECT

    public:
        explicit LoadingCompletedCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
        void execute() override;

    private:
        const bool pageIsLoaded_;
    };
} // namespace JsBridge
