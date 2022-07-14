#include "stdafx.h"

#include "GetLanguageCommand.h"

#include "../../gui_settings.h"

namespace JsBridge
{
    GetLanguageCommand::GetLanguageCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : AbstractCommand(_reqId, _requestParams, _webPage)
    {}

    void GetLanguageCommand::execute()
    {
        const QString lang = Ui::get_gui_settings()->get_value<QString>(settings_language, QString {});
        addSuccessParameter(QJsonObject { { QPair { qsl("language"), lang } } });
    }
} // namespace JsBridge
