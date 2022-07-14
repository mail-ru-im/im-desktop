#include "stdafx.h"

#include "AvatarStorageProxy.h"
#include "cache/avatars/AvatarStorage.h"
#include "../contact_list/FavoritesUtils.h"
#include "../contact_list/ServiceContacts.h"
#include "utils/utils.h"
#include "my_info.h"

namespace
{
    using PixmapCache = std::unordered_map<int, QPixmap>;

    QPixmap getFavoritesAvatarFromCache(int _sizePx)
    {
        static PixmapCache g_favoritesAvatarsCache;

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            g_favoritesAvatarsCache.clear();

        auto& cached = g_favoritesAvatarsCache[_sizePx];
        if (cached.isNull())
            cached = Favorites::avatar(_sizePx);
        return cached;
    }

    QPixmap getServiceAvatarFromCache(const QString& _contactId, int _sizePx)
    {
        static std::unordered_map<QString, PixmapCache> g_serviceAvatarsCache;

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            g_serviceAvatarsCache.clear();

        auto& cached = g_serviceAvatarsCache[_contactId][_sizePx];
        if (cached.isNull())
            cached = ServiceContacts::avatar(_contactId, _sizePx);
        return cached;
    }
}

namespace Logic
{

QPixmap AvatarStorageProxy::Get(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate)
{
    if ((flags_ & ReplaceFavorites) && Favorites::isFavorites(_aimId))
        return getFavoritesAvatarFromCache(_sizePx);
    if ((flags_ & ReplaceService) && ServiceContacts::isServiceContact(_aimId))
        return getServiceAvatarFromCache(_aimId, _sizePx);

    return GetAvatarStorage()->Get(_aimId, _displayName, _sizePx, _isDefault, _regenerate);
}

QPixmap AvatarStorageProxy::GetRounded(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate, bool _mini_icons)
{
    if (flags_ & ReplaceFavorites && Favorites::isFavorites(_aimId))
        return getFavoritesAvatarFromCache(_sizePx);
    if (flags_ & ReplaceService && ServiceContacts::isServiceContact(_aimId))
        return getServiceAvatarFromCache(_aimId, _sizePx);

    return GetAvatarStorage()->GetRounded(_aimId, _displayName, _sizePx, _isDefault, _regenerate, _mini_icons);
}


AvatarStorageProxy getAvatarStorageProxy(int32_t _flags)
{
    return AvatarStorageProxy(_flags);
}

}
