#pragma once

namespace Ui::Accessibility
{

    QAccessibleInterface* customWidgetsAccessibilityFactory(const QString& classname, QObject* object);

}
