#include "QObjectWatcher.h"

namespace Utils
{

QObjectWatcher::QObjectWatcher(QObject *parent)
    : QObject(parent)
{

}

bool QObjectWatcher::addObject(QObject *_object)
{
    if (!_object)
        return false;

    bool res = connectToObject(_object);
    addObjectToList(_object);

    return res;
}

const QObjectWatcher::ObjectsList &QObjectWatcher::allObjects() const
{
    return objectsList_;
}

bool QObjectWatcher::connectToObject(QObject *_object)
{
    auto conn = connect(_object, &QObject::destroyed,
                             this, &QObjectWatcher::onObjectDeleted);

    return conn;
}

void QObjectWatcher::deleteObjectFromList(QObject *_object)
{
    objectsList_.removeAll(_object);
}

void QObjectWatcher::addObjectToList(QObject *_object)
{
    objectsList_.push_back(_object);
}

void QObjectWatcher::onObjectDeleted(QObject *_object)
{
    deleteObjectFromList(_object);
}

}
