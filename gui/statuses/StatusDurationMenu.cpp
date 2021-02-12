#include "stdafx.h"
#include "StatusDurationMenu.h"
#include "fonts.h"
#include "StatusUtils.h"
#include "Status.h"
#include "styles/ThemeParameters.h"
#include "utils/utils.h"

namespace
{
    int menuWidth()
    {
        return Utils::scale_value(201);
    }

    const std::vector<std::pair<std::chrono::seconds, QString>>& timeOptions()
    {
        static const std::vector<std::pair<std::chrono::seconds, QString>> options =
        {
                { std::chrono::seconds(0), QT_TRANSLATE_NOOP("status", "Show always") },
                { std::chrono::minutes(10), Statuses::getTimeString(std::chrono::minutes(10)) },
                { std::chrono::minutes(30), Statuses::getTimeString(std::chrono::minutes(30)) },
                { std::chrono::hours(1), Statuses::getTimeString(std::chrono::hours(1)) },
                { std::chrono::hours(24), Statuses::getTimeString(std::chrono::hours(24)) },
                { std::chrono::hours(24 * 7), QT_TRANSLATE_NOOP("status", "Week")}
        };
        return options;
    }
}

namespace Statuses
{

using namespace Ui;

class StatusDurationMenu_p
{
public:
    QAction* customTimeAction_ = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// StatusDurationMenu
//////////////////////////////////////////////////////////////////////////

StatusDurationMenu::StatusDurationMenu(QWidget* _parent)
    : FlatMenu(_parent, BorderStyle::SHADOW)
    , d(std::make_unique<StatusDurationMenu_p>())
{
    setFixedWidth(menuWidth());

    auto label = new QLabel(QT_TRANSLATE_NOOP("status_popup", "Status timing"));
    label->setFont(Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold));
    Utils::ApplyStyle(label,
                      ql1s("background: transparent; height: 52dip; padding-right: 64dip; padding-top: 20dip; padding-left: 16dip; padding-bottom: 20dip; color: %1;")
                              .arg(Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID)));

    auto separator = new QWidgetAction(nullptr);
    separator->setDefaultWidget(label);
    addAction(separator);

    for (const auto& [time, description] : timeOptions())
    {
        auto action = addAction(description);
        action->setData(QVariant::fromValue(std::chrono::duration_cast<std::chrono::seconds>(time).count()));
    }

    d->customTimeAction_ = addAction(QT_TRANSLATE_NOOP("status_popup", "Select custom time"));
    connect(this, &StatusDurationMenu::triggered, this, &StatusDurationMenu::onActionTriggered);
}

StatusDurationMenu::~StatusDurationMenu() = default;

void StatusDurationMenu::onActionTriggered(QAction* _action)
{
    if (_action == d->customTimeAction_)
        Q_EMIT customTimeClicked(QPrivateSignal());
    else
        Q_EMIT durationClicked(std::chrono::seconds(_action->data().toLongLong()), QPrivateSignal());
}

}
