#include "stdafx.h"

#include "FriendlyContaInerProxy.h"
#include "main_window/containers/FriendlyContainer.h"
#include "../contact_list/FavoritesUtils.h"
#include "my_info.h"

namespace Logic
{

QString FriendlyContainerProxy::getFriendly(const QString& _aimid) const
{
    if (flags_ & ReplaceFavorites && _aimid == Ui::MyInfo()->aimId())
        return Favorites::name();

    return GetFriendlyContainer()->getFriendly(_aimid);
}

Data::Friendly FriendlyContainerProxy::getFriendly2(const QString& _aimid) const
{
    if (flags_ & ReplaceFavorites && _aimid == Ui::MyInfo()->aimId())
        return {Favorites::name(), Favorites::name()};

    return GetFriendlyContainer()->getFriendly2(_aimid);
}

FriendlyContainerProxy getFriendlyContainerProxy(int32_t _flags)
{
    return FriendlyContainerProxy(_flags);
}

}
