#include "stdafx.h"
#include "GalleryModel.h"


Qml::GalleryModel::GalleryModel(QObject* _parent)
    : QObject(_parent)
    , galleryTabsModel_ { new GalleryTabsModel(this) }
{
}

Qml::GalleryTabsModel* Qml::GalleryModel::galleryTabsModel() const
{
    return galleryTabsModel_;
}

bool Qml::GalleryModel::isPopupVisible() const
{
    return popupVisible_;
}

void Qml::GalleryModel::setPopupVisible(bool _visible)
{
    if (_visible != popupVisible_)
    {
        popupVisible_ = _visible;
        Q_EMIT popupVisibleChanged(_visible);
    }
}
