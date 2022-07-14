#pragma once
namespace Utils
{
    class QmlHelper : public QObject
    {
        Q_OBJECT
    public:
        explicit QmlHelper(QObject* _parent = nullptr);
    public Q_SLOTS:
        int scaleValue(int _value) const;
    };
} // namespace Utils
