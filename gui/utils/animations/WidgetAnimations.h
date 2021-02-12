#pragma once
#include "stdafx.h"

#include "SlideController.h"
#include "WidgetFader.h"
#include "WidgetResizer.h"
#include "ZoomAnimation.h"

namespace Utils
{

class ScopedPropertyRollback
{
    Q_DISABLE_COPY_MOVE(ScopedPropertyRollback)
public:
    explicit ScopedPropertyRollback(QObject* _object, const char* _property)
        : object_(_object)
        , name_(_property)
    {
        value_ = object_->property(name_);
    }

    explicit ScopedPropertyRollback(QObject* _object, const char* _property, const QVariant& _newVal)
        : object_(_object)
        , name_(_property)
    {
        value_ = object_->property(name_);
        object_->setProperty(name_, _newVal);
    }

    void rollback()
    {
        if (object_)
            object_->setProperty(name_, value_);
        object_ = nullptr;
    }

    ~ScopedPropertyRollback()
    {
        if (object_)
            object_->setProperty(name_, value_);
    }

private:
    QVariant value_;
    QPointer<QObject> object_;
    const char* name_;
};

} // end namespace Utils



