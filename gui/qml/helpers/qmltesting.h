#pragma once

namespace Testing
{
    class QmlHelper : public QObject
    {
        Q_OBJECT

    public:
        explicit QmlHelper(QObject* _parent = nullptr);

    public Q_SLOTS:
        QString accessibleName(const QString& _name) const;
    };
} // namespace Testing
