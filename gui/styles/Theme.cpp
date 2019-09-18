#include "stdafx.h"

#include "Theme.h"
#include "ThemesUtils.h"
#include "StyleVariable.h"

#include "../fonts.h"
#include "../utils/utils.h"

namespace
{
    template<class Map>
    auto findProperty(const Map& _map, const QString& _value)
    {
        return std::find_if(_map.begin(), _map.end(), [&_value](const auto& _prop)
        {
            return (_prop.second == _value);
        });
    }

    QColor invalidColor()
    {
        static QColor clr("#ffee00");
        return clr;
    }

    template <class T>
    void unserializeWallpaperId(const rapidjson::Value& _node, const T& _name, QString& _out)
    {
        if (!StylingUtils::unserialize_value(_node, _name, _out))
        {
            if (auto intId = -1; StylingUtils::unserialize_value(_node, _name, intId))
                _out = QString::number(intId);
        }
        //assert(!_out.isEmpty());
    }
}

namespace Styling
{
    void Property::unserialize(const JNode & _node)
    {
        if (_node.IsObject())
            StylingUtils::unserialize_value(_node, "color", color_);
        else if (_node.IsString())
            color_ = StylingUtils::getColor(_node);
    }

    QColor Property::getColor() const
    {
        if (color_.isValid())
            return color_;

        return invalidColor();
    }

    Style::Properties::const_iterator Style::getPropertyIt(const StyleVariable _variable) const
    {
        return properties_.find(_variable);
    }

    void Style::unserialize(const JNode & _node)
    {
        const auto& props = getVariableStrings();

        for (const auto& it : _node.GetObject())
        {
            const auto propName = StylingUtils::getString(it.name);
            if (const auto property = findProperty(props, propName); property != props.end())
            {
                Property tmpProperty;
                tmpProperty.unserialize(it.value);
                setProperty(stringToVariable(propName), std::move(tmpProperty));
            }
        }
    }

    QColor Style::getColor(const StyleVariable _variable) const
    {
        if (const auto propIterator = getPropertyIt(_variable); propIterator != properties_.end())
            return propIterator->second.getColor();

        return invalidColor();
    }

    Theme::Theme()
        : id_(qsl("default"))
        , name_(qsl("Noname"))
        , underlinedLinks_(Ui::TextRendering::LinksStyle::PLAIN)
    {
    }

    void Theme::addWallpaper(WallpaperPtr _wallpaper)
    {
        if (!getWallpaper(_wallpaper->getId()))
        {
            if (_wallpaper->getId() == defaultWallpaperId_)
                availableWallpapers_.insert(availableWallpapers_.begin(), _wallpaper);
            else
                availableWallpapers_.push_back(_wallpaper);
        }
    }

    WallpaperPtr Theme::getWallpaper(const WallpaperId& _id) const
    {
        const auto it = std::find_if(availableWallpapers_.begin(), availableWallpapers_.end(), [&_id](const auto& _w) { return _id == _w->getId(); });
        if (it != availableWallpapers_.end())
            return *it;

        return nullptr;
    }

    void Theme::unserialize(const JNode &_node)
    {
        if (const auto nodeIter = _node.FindMember("style"); nodeIter->value.IsObject() && !nodeIter->value.ObjectEmpty())
            Style::unserialize(nodeIter->value);

        StylingUtils::unserialize_value(_node, "id", id_);
        StylingUtils::unserialize_value(_node, "name", name_);

        if (auto underline = false; StylingUtils::unserialize_value(_node, "underline", underline) && underline)
            underlinedLinks_ = Ui::TextRendering::LinksStyle::UNDERLINED;
        unserializeWallpaperId(_node, "defaultWallpaper", defaultWallpaperId_.id_);
    }

    bool Theme::isWallpaperAvailable(const WallpaperId& _wallpaperId) const
    {
        return std::any_of(
            availableWallpapers_.begin(),
            availableWallpapers_.end(),
                    [&_wallpaperId](const auto& _wall) { return _wall->getId() == _wallpaperId; });
    }

    WallpaperPtr Theme::getFirstWallpaper() const
    {
        if (!availableWallpapers_.empty())
            return availableWallpapers_.front();

        return nullptr;
    }

    ThemeWallpaper::ThemeWallpaper(const Theme& _basicStyle)
    {
        setProperties(_basicStyle.getProperties());
    }

    void ThemeWallpaper::unserialize(const JNode &_node)
    {
        //assert(_node.HasMember("style"));
        if (const auto nodeIter = _node.FindMember("style"); nodeIter->value.IsObject() && !nodeIter->value.ObjectEmpty())
            Style::unserialize(nodeIter->value);

        unserializeWallpaperId(_node, "id", id_.id_);

        StylingUtils::unserialize_value(_node, "need_border", needBorder_);
        StylingUtils::unserialize_value(_node, "preview", previewUrl_);
        StylingUtils::unserialize_value(_node, "image", imageUrl_);
        StylingUtils::unserialize_value(_node, "color", color_);

        if (hasWallpaper())
        {
            StylingUtils::unserialize_value(_node, "tile", tile_);
            StylingUtils::unserialize_value(_node, "tint", tint_);
        }
    }

    bool ThemeWallpaper::operator==(const ThemeWallpaper & _other)
    {
        const bool equalId = (id_ == _other.id_);
        const bool equalTile = (tile_ == _other.tile_);
        const bool equalBorder = (needBorder_ == _other.needBorder_);
        const bool equalPixmap = (previewUrl_ == _other.previewUrl_ && imageUrl_ == _other.imageUrl_);

        return (equalId && equalTile && equalBorder && equalPixmap);
    }

    void ThemeWallpaper::setWallpaperImage(const QPixmap& _image) // must be tinted beforehand!
    {
        assert(!_image.isNull());

        setImageRequested(false);
        image_ = _image;
    }

    void ThemeWallpaper::setPreviewImage(const QPixmap& _image)
    {
        assert(!_image.isNull());
        setPreviewRequested(false);
        preview_ = _image;
        Utils::check_pixel_ratio(preview_);
    }

    void ThemeWallpaper::resetWallpaperImage()
    {
        setImageRequested(false);
        image_ = QPixmap();
    }
}
