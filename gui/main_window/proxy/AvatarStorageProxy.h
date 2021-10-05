#pragma once

namespace Logic
{
    class AvatarStorageProxy
    {
    public:

        enum Flags
        {
            ReplaceFavorites = 1 << 0,
            ReplaceService = 1 << 1,
        };

        AvatarStorageProxy(int32_t _flags) : flags_(_flags) {}

        QPixmap Get(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate);
        QPixmap GetRounded(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate, bool _mini_icons);

    private:
        int32_t flags_;
    };

    AvatarStorageProxy getAvatarStorageProxy(int32_t _flags);

}
