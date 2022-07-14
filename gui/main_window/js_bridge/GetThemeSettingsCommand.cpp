#include "stdafx.h"
#include "GetThemeSettingsCommand.h"
#include "styles/ThemesContainer.h"

namespace JsBridge
{
    GetThemeSettingsCommand::GetThemeSettingsCommand(const QString& _reqId, const QJsonObject& _requestParams, Ui::WebAppPage* _webPage)
        : AbstractCommand(_reqId, _requestParams, _webPage)
    {}

    void GetThemeSettingsCommand::execute()
    {
        const QString theme = Styling::getThemesContainer().getCurrentTheme()->isDark() ? qsl("dark") : qsl("white");
        const QString primaryColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY).name();
        addSuccessParameter(QJsonObject { { QPair { qsl("primaryColor"), primaryColor }, QPair { qsl("scheme"), theme } } });
    }
} // namespace JsBridge
