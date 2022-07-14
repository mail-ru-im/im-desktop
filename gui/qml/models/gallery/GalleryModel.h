#pragma once

#include "GalleryTabsModel.h"

namespace Qml
{
    class GalleryModel : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(GalleryTabsModel* tabs READ galleryTabsModel CONSTANT)
        Q_PROPERTY(bool popupVisible READ isPopupVisible WRITE setPopupVisible NOTIFY popupVisibleChanged)

    public:
        explicit GalleryModel(QObject* _parent);

    public Q_SLOTS:
        GalleryTabsModel* galleryTabsModel() const;

        bool isPopupVisible() const;
        void setPopupVisible(bool _visible);

    Q_SIGNALS:
        void popupVisibleChanged(bool _popupVisible);

    private:
        GalleryTabsModel* galleryTabsModel_;
        bool popupVisible_ = false;
    };
}
