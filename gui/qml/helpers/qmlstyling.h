#pragma once

#include "../../styles/StyleVariable.h"

namespace Styling
{
    class QmlHelper : public QObject
    {
        Q_OBJECT
    public:
        explicit QmlHelper(QObject* _parent = nullptr);

    public Q_SLOTS:
        QColor getColor(Styling::StyleVariable _styleVariable, double _opacity = 1.0) const;
        QColor getColor(const QString& _aimId, Styling::StyleVariable _styleVariable, double _opacity = 1.0) const;
    };
} // namespace Styling
