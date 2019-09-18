#pragma once

#include "WallpaperId.h"
#include "../controls/TextRendering.h"

namespace Styling
{
    class Property;
    class Style;

    class Theme;
    using ThemePtr = std::shared_ptr<Theme>;

    class ThemeWallpaper;
    using WallpaperPtr = std::shared_ptr<ThemeWallpaper>;
    using WallpapersVector = std::vector<WallpaperPtr>;

    using JNode = rapidjson::Value;

    enum class StyleVariable;

    class Property
    {
        QColor color_;

    public:
        Property() = default;
        Property(const QColor& _color) : color_(_color) {}

        void unserialize(const JNode& _node);
        QColor getColor() const;

        bool operator==(const Property& _other) const noexcept { return color_ == _other.color_; }
    };

    class Style
    {
    private:
        using Properties = std::unordered_map<StyleVariable, Property>;
        Properties properties_;
        Properties::const_iterator getPropertyIt(const StyleVariable _variable) const;

    public:
        void unserialize(const JNode& _node);

        QColor getColor(const StyleVariable _variable) const;

        const auto& getProperties() const { return properties_; }
        void setProperty(const StyleVariable _var, Property _property)
        {
            properties_[_var] = std::move(_property);
        }

        void setProperties(const Properties& _properties)
        {
            properties_ = _properties;
        }
    };

    class Theme : public Style
    {
        QString id_;
        QString name_;
        WallpapersVector availableWallpapers_;
        WallpaperId defaultWallpaperId_;
        Ui::TextRendering::LinksStyle underlinedLinks_;

    public:
        Theme();
        const QString& getId() const { return id_; };
        const QString& getName() const { return name_; };
        Ui::TextRendering::LinksStyle linksUnderlined() const { return underlinedLinks_; };
        void addWallpaper(WallpaperPtr _wallpaper);
        WallpaperPtr getWallpaper(const WallpaperId& _id) const;
        const WallpapersVector& getAvailableWallpapers() const { return availableWallpapers_; }
        void unserialize(const JNode& _node);

        bool isWallpaperAvailable(const WallpaperId& _wallpaperId) const;
        WallpaperPtr getFirstWallpaper() const;

        WallpaperPtr getDefaultWallpaper() const { return getWallpaper(defaultWallpaperId_); }
        const WallpaperId& getDefaultWallpaperId() const noexcept { return defaultWallpaperId_; }
    };

    class ThemeWallpaper : public Style
    {
        WallpaperId id_;

        QPixmap preview_;
        QPixmap image_;

        QString previewUrl_;
        QString imageUrl_;

        QString parentThemeId_;

        QColor color_;
        QColor tint_;

        bool tile_ = false;
        bool needBorder_ = false;
        bool isImageRequested_ = false;
        bool isPreviewRequested_ = false;

    public:
        ThemeWallpaper(const Theme& _basicStyle);
        void unserialize(const JNode& _node);
        bool operator==(const ThemeWallpaper& _other);

        void setParentThemeId(const QString& _id) { parentThemeId_ = _id; }
        const QString& getParentThemeId() const { return parentThemeId_; }

        const WallpaperId& getId() const noexcept { return id_; }
        void setId(const WallpaperId& _newId) { id_ = _newId; }

        QColor getBgColor() const { return color_; }
        QColor getTint() const { return tint_; }
        void setTint(const QColor& _tint) { tint_ = _tint; }
        bool isBorderNeeded() const noexcept { return needBorder_; }

        bool isTiled() const noexcept { return tile_; }
        void setTiled(const bool _tiled) { tile_ = _tiled; }

        bool hasWallpaper() const noexcept { return !imageUrl_.isEmpty() || id_.isUser(); };
        const QPixmap& getWallpaperImage() const { return image_; };
        const QString& getWallpaperUrl() const { return imageUrl_; };
        const QPixmap& getPreviewImage() const { return preview_; };
        const QString& getPreviewUrl() const { return previewUrl_; };

        void setWallpaperImage(const QPixmap& _image); // must be tinted beforehand!
        void setPreviewImage(const QPixmap& _image);

        void resetWallpaperImage();

        bool isImageRequested() const noexcept { return isImageRequested_; }
        void setImageRequested(const bool _requested) { isImageRequested_ = _requested; }

        bool isPreviewRequested() const noexcept { return isPreviewRequested_; }
        void setPreviewRequested(const bool _requested) { isPreviewRequested_ = _requested; }
    };
}
