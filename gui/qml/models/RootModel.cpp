#include "stdafx.h"
#include "RootModel.h"

Qml::RootModel::RootModel(QObject* _parent)
    : QObject(_parent)
    , gallery_ { new GalleryModel(this) }
{
}

Qml::GalleryModel* Qml::RootModel::gallery() const
{
    return gallery_;
}
