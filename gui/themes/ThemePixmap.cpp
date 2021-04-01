#include "stdafx.h"

#include "app_config.h"
#include "../utils/utils.h"
#include "ResourceIds.h"
#include "ThemePixmap.h"
#include "../utils/log/log.h"

namespace
{
    std::unordered_map<int, Themes::ThemePixmapSptr> ResourcePixmaps_;
    std::unordered_map<int, QString> ResourcePaths_;

    void InitResourcePaths()
    {
        if (!ResourcePaths_.empty())
            return;

        static const std::tuple<Themes::PixmapResourceId, const char*, const char*> index[] =
        {
            #include "ThemePixmapDb.inc"
        };

        int current_scale = Utils::scale_bitmap(Utils::scale_value(100));
        if (current_scale > 200)
            current_scale = 200;

        const auto scale = QString::number(current_scale);

        for (const auto &pair : index)
        {
            const auto resId = std::get<0>(pair);
            const auto resDir = std::get<1>(pair);
            const auto resFilename = std::get<2>(pair);

            QString resPath = u":/themes/standard/" % scale % u'/' % QString::fromUtf8(resDir) % u'/' % QString::fromUtf8(resFilename) % u".png";

            if (Ui::GetAppConfig().IsFullLogEnabled())
            {
                std::stringstream logData;
                logData << "INVALID ICON SIZE all icons from path\r\n";
                logData << resPath.toUtf8().constData();
                Log::write_network_log(logData.str());
            }

            const auto emplaceResult = ResourcePaths_.emplace((int)resId, std::move(resPath));
            im_assert(emplaceResult.second);
        }
    }
}

namespace Themes
{
    const ThemePixmapSptr& GetPixmap(const PixmapResourceId id)
    {
        im_assert(id > PixmapResourceId::Min);
        im_assert(id < PixmapResourceId::Max);

        const auto iterCache = ResourcePixmaps_.find((int)id);
        if (iterCache != ResourcePixmaps_.end())
        {
            return iterCache->second;
        }

        auto themePixmap = std::make_shared<ThemePixmap>(id);

        const auto result = ResourcePixmaps_.emplace((int)id, themePixmap);
        im_assert(result.second);

        return result.first->second;
    }

    ThemePixmap::ThemePixmap(const PixmapResourceId resourceId)
        : ResourceId_(resourceId)
    {
        im_assert(ResourceId_ > PixmapResourceId::Min);
        im_assert(ResourceId_ < PixmapResourceId::Max);
    }

    ThemePixmap::ThemePixmap(const QPixmap& _px)
        : ResourceId_(PixmapResourceId::Invalid)
        , Pixmap_(_px)
    {
    }

    void ThemePixmap::Draw(QPainter &p, const int32_t x, const int32_t y) const
    {
        PreparePixmap();

        p.drawPixmap(x, y, Pixmap_);
    }

    void ThemePixmap::Draw(QPainter &p, const int32_t x, const int32_t y, const int32_t w, const int32_t h) const
    {
        im_assert(w > 0);
        im_assert(h > 0);

        PreparePixmap();

        p.drawPixmap(x, y, w, h, Pixmap_);
    }

    void ThemePixmap::Draw(QPainter &p, const QRect &r) const
    {
        im_assert(r.width() > 0);
        im_assert(r.height() > 0);
        PreparePixmap();
        p.drawPixmap(r, Pixmap_);
    }

    int32_t ThemePixmap::GetWidth() const
    {
        PreparePixmap();

        return (Pixmap_.width() / Pixmap_.devicePixelRatio());
    }

    int32_t ThemePixmap::GetHeight() const
    {
        PreparePixmap();

        return (Pixmap_.height() / Pixmap_.devicePixelRatio());
    }

    QRect ThemePixmap::GetRect() const
    {
        PreparePixmap();
#ifdef __APPLE__
        return QRect(0, 0, Pixmap_.width() / Pixmap_.devicePixelRatio(), Pixmap_.height() / Pixmap_.devicePixelRatio());
#else
        return Pixmap_.rect();
#endif
    }

    QSize ThemePixmap::GetSize() const
    {
        PreparePixmap();

#ifdef __APPLE__
        return QSize(Pixmap_.width() / Pixmap_.devicePixelRatio(), Pixmap_.height() / Pixmap_.devicePixelRatio());
#else
        return Pixmap_.size();
#endif
    }

    QPixmap ThemePixmap::GetPixmap() const
    {
        PreparePixmap();
        return Pixmap_;
    }

    void ThemePixmap::PreparePixmap() const
    {
        if (!Pixmap_.isNull())
            return;

        InitResourcePaths();

        const auto pathIter = ResourcePaths_.find((int)ResourceId_);
        if (pathIter == ResourcePaths_.end())
        {
            im_assert(!"unknown resource identifier");
            return;
        }

        Pixmap_.load(pathIter->second);

        im_assert(!Pixmap_.isNull() || !"failed to load a pixmap from resources");

        Utils::check_pixel_ratio(Pixmap_);
    }
}
