#pragma once

#include <QAbstractNativeEventFilter>

namespace Utils
{

class NativeEventFilter: public QAbstractNativeEventFilter
{
public:
    virtual bool nativeEventFilter(const QByteArray& _eventType, void* _message, long*) Q_DECL_OVERRIDE;
};

}
