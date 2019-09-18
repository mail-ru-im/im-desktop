#pragma once

#include <QObject>
#include "../namespaces.h"

namespace Utils
{

class QObjectWatcher: public QObject
{
    Q_OBJECT

public:
    using ObjectsList = QObjectList;

public:
    QObjectWatcher(QObject *parent = nullptr);

    bool addObject(QObject *_object);
    const ObjectsList& allObjects() const;

private:
    bool connectToObject(QObject *_object);
    void deleteObjectFromList(QObject *_object);
    void addObjectToList(QObject *_object);

private Q_SLOTS:
    void onObjectDeleted(QObject* _object);

private:
    ObjectsList objectsList_;
};

}
