#pragma once

namespace Logic
{
    using QPixmapSCptr = std::shared_ptr<const QPixmap>;

    class AvatarStorageProxy
    {
    public:

        enum Flags
        {
            ReplaceFavorites = 1 << 0,
        };

        AvatarStorageProxy(int32_t _flags) : flags_(_flags) {}

        const QPixmapSCptr& Get(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate);
        const QPixmapSCptr& GetRounded(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate, bool _mini_icons);

    private:
        int32_t flags_;
    };

    AvatarStorageProxy getAvatarStorageProxy(int32_t _flags);

}
