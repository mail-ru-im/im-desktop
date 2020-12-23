#include "stdafx.h"

#include "AvatarStorageProxy.h"
#include "cache/avatars/AvatarStorage.h"
#include "../contact_list/FavoritesUtils.h"
#include "utils/utils.h"
#include "my_info.h"

namespace
{
    using PixmapCache = std::unordered_map<int, QPixmap>;
    std::unordered_map<int, QPixmap> g_favoritesAvatarsCache;

    QPixmap getFavoritesAvatarFromCache(int _sizePx)
    {
        auto& cached = g_favoritesAvatarsCache[_sizePx];
        if (cached.isNull())
            cached = Favorites::avatar(_sizePx);
        return cached;
    }
}

namespace Logic
{

QPixmap AvatarStorageProxy::Get(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate)
{
    if (flags_ & ReplaceFavorites && Favorites::isFavorites(_aimId))
        return getFavoritesAvatarFromCache(_sizePx);

    return GetAvatarStorage()->Get(_aimId, _displayName, _sizePx, _isDefault, _regenerate);
}

QPixmap AvatarStorageProxy::GetRounded(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate, bool _mini_icons)
{
    if (flags_ & ReplaceFavorites && Favorites::isFavorites(_aimId))
        return getFavoritesAvatarFromCache(_sizePx);

    return GetAvatarStorage()->GetRounded(_aimId, _displayName, _sizePx, _isDefault, _regenerate, _mini_icons);
}


AvatarStorageProxy getAvatarStorageProxy(int32_t _flags)
{
    return AvatarStorageProxy(_flags);
}

}
