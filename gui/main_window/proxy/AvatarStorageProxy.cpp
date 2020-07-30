#include "stdafx.h"

#include "AvatarStorageProxy.h"
#include "cache/avatars/AvatarStorage.h"
#include "../contact_list/FavoritesUtils.h"
#include "utils/utils.h"
#include "my_info.h"

namespace
{
    using PixmapCache = std::unordered_map<int, Logic::QPixmapSCptr>;
    std::unordered_map<int, Logic::QPixmapSCptr> g_favoritesAvatarsCache;

    const Logic::QPixmapSCptr& getFavoritesAvatarFromCache(int _sizePx)
    {
        if (auto it = g_favoritesAvatarsCache.find(_sizePx); it != g_favoritesAvatarsCache.end())
            return it->second;

        auto pixmap = Favorites::avatar(_sizePx);
        g_favoritesAvatarsCache[_sizePx] = std::make_shared<QPixmap>(std::move(pixmap));

        return g_favoritesAvatarsCache[_sizePx];
    }
}

namespace Logic
{

const QPixmapSCptr& AvatarStorageProxy::Get(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate)
{
    if (flags_ & ReplaceFavorites && Favorites::isFavorites(_aimId))
        return getFavoritesAvatarFromCache(_sizePx);

    return GetAvatarStorage()->Get(_aimId, _displayName, _sizePx, _isDefault, _regenerate);
}

const QPixmapSCptr& AvatarStorageProxy::GetRounded(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate, bool _mini_icons)
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
