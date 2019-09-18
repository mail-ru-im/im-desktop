#pragma once

#include "../corelib/collection_helper.h"
#include "../utils/utils.h"

#include "Theme.h"
#include "StyleVariable.h"
#include "WallpaperId.h"

namespace Ui
{
    class CustomButton;
}

namespace Styling
{
    class ThemesContainer : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void globalThemeChanged(QPrivateSignal);
        void globalWallpaperChanged(QPrivateSignal);
        void contactWallpaperChanged(const QString& _aimId, QPrivateSignal);
        void wallpaperImageAvailable(const WallpaperId& _wpId, QPrivateSignal);
        void wallpaperPreviewAvailable(const WallpaperId& _wpId, QPrivateSignal);

    public:
        ThemesContainer(QObject* _parent = nullptr);
        void unserialize(core::coll_helper _collection);

        bool isLoaded() const { return isLoaded_; }

        [[nodiscard]] ThemePtr getCurrentTheme() const;
        [[nodiscard]] QString getCurrentThemeId() const;
        void setCurrentTheme(const QString& _themeName);

        [[nodiscard]] ThemePtr getTheme(const QString& _themeName) const;

        [[nodiscard]] std::vector<std::pair<QString, QString>> getAvailableThemes() const;

        [[nodiscard]] WallpaperPtr getContactWallpaper(const QString& _aimId) const;
        [[nodiscard]] WallpaperPtr getGlobalWallpaper() const;

        std::map<QString, int> getContactWallpaperCounters() const;
        const WallpaperId& getGlobalWallpaperId() const;
        const WallpaperId& getContactWallpaperId(const QString& _aimId) const;

        [[nodiscard]] WallpapersVector getAvailableWallpapers(const QString& _themeId) const;
        [[nodiscard]] WallpapersVector getAllAvailableWallpapers() const;

        void addUserWallpaper(const WallpaperPtr& _wallpaper);
        WallpaperPtr createUserWallpaper(const ThemePtr& _theme, const WallpaperId& _id);
        inline WallpaperPtr createUserWallpaper(const QString& _contact)
        {
            return createUserWallpaper(getCurrentTheme(), WallpaperId::fromContact(_contact));
        }

        void setGlobalWallpaper(const WallpaperId& _id);
        void setContactWallpaper(const QString& _aimId, const WallpaperId& _id);
        void resetContactWallpaper(const QString& _aimId);
        void resetAllContactWallpapers();

        void requestWallpaper(const WallpaperPtr _wallpaper);
        void requestWallpaperPreview(const WallpaperPtr _wallpaper);

        void setTryOnWallpaper(const WallpaperId& _id, const QString& _aimId);
        void setTryOnWallpaper(const WallpaperPtr _wallpaper, const QString& _aimId);
        void resetTryOnWallpaper();

        QColor getDefaultBackgroundColor();

        void unloadUnusedWallpaperImages();

    private:
        void unserializeMeta(const QString& _meta);
        void unserializeUserWallpapers(core::coll_helper _collection);
        void unserializeSettings(core::coll_helper _collection);

        void postImageUrlsToCore() const;
        void postCurrentThemeIdToCore() const;

        void postGlobalWallpaperToCore() const;
        void postContactsWallpapersToCore() const;
        void postTiledsWallpapersToCore() const;

        void onImCreated();
        void onWallpaper(const WallpaperId& _id, const QPixmap& _image);
        void onWallpaperPreview(const WallpaperId& _id, const QPixmap& _image);
        void onDialogClosed(const QString& _aimId, bool);

        WallpaperPtr getThemeDefaultWallpaper() const; // fallbacks to theme first available wallpaper if default not set

        enum class GuiCall
        {
            yes,
            no
        };
        void setCurrentTheme(const QString& _themeName, const GuiCall _fromGui);
        void setGlobalWallpaper(const WallpaperId& _wpId, const GuiCall _fromGui);
        void setContactWallpaper(const QString& _aimId, const WallpaperId& _wpId, const GuiCall _fromGui);
        void resetContactWallpaper(const QString& _aimId, const GuiCall _fromGui);
        void resetAllContactWallpapers(const GuiCall _fromGui);

        [[nodiscard]] WallpapersVector getAllWallpapersById(const WallpaperId& _id) const;

        ThemePtr currentTheme_;
        std::vector<ThemePtr> availableThemes_;

        WallpaperPtr currentWallpaper_;
        std::unordered_map<QString, WallpaperPtr, Utils::QStringHasher> contactWallpapers_;

        WallpaperPtr tryOnWallpaper_;
        QString tryOnContact_;

        QString baseUrl_;
        bool isLoaded_;
    };

    [[nodiscard]] ThemesContainer& getThemesContainer();

    QString getTryOnAccount();
}
