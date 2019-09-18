#include "AbstractSearchModel.h"
#include "stdafx.h"

#include "AbstractSearchModel.h"

namespace
{
    constexpr auto defaultServerSearchEnabled = false;
}

namespace Logic
{
    AbstractSearchModel::AbstractSearchModel(QObject* _parent)
        : CustomAbstractListModel(_parent)
        , isServerSearchEnabled_(defaultServerSearchEnabled)
    {

    }

    QString AbstractSearchModel::getSearchPattern() const
    {
        return searchPattern_;
    }

    void AbstractSearchModel::repeatSearch()
    {
        setSearchPattern(getSearchPattern());
    }
}
