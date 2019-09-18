#pragma once

namespace Themes
{
    enum class PixmapResourceId;

    class ThemePixmap
    {
    public:
        ThemePixmap(const PixmapResourceId resourceId);
        ThemePixmap(const QPixmap& _px);

        void Draw(QPainter &p, const int32_t x, const int32_t y) const;
        void Draw(QPainter &p, const int32_t x, const int32_t y, const int32_t w, const int32_t h) const;
        void Draw(QPainter &p, const QRect &r) const;

        int32_t GetWidth() const;
        int32_t GetHeight() const;
        QRect GetRect() const;
        QSize GetSize() const;
        QPixmap GetPixmap() const;

    private:
        void PreparePixmap() const;

        const PixmapResourceId ResourceId_;
        mutable QPixmap Pixmap_;
    };

    using ThemePixmapSptr = std::shared_ptr<ThemePixmap>;

    const ThemePixmapSptr& GetPixmap(const PixmapResourceId id);
}