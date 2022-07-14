#pragma once

#include "AbstractCommand.h"

namespace Ui
{
    class WebAppPage;
}

namespace JsBridge
{
    class SetTitleCommand : public AbstractCommand
    {
        Q_OBJECT

    public:
        explicit SetTitleCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage);
        void execute() override;

    private:
        Ui::WebAppPage* webPage_;
    };
} // namespace JsBridge
