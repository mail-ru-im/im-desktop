#pragma once

#include "gallery/GalleryModel.h"

namespace Qml
{
    class RootModel : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(GalleryModel* gallery READ gallery CONSTANT)

    public:
        explicit RootModel(QObject* _parent);
        GalleryModel* gallery() const;

    private:
        GalleryModel* gallery_;
    };

} // namespace Qml
