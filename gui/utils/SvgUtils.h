#pragma once

#include "styles/ThemeColor.h"

namespace Utils
{
    enum class KeepRatio
    {
        no,
        yes
    };

    QPixmap renderSvg(const QString& _resourcePath, const QSize& _scaledSize = {}, const QColor& _tintColor = {}, const KeepRatio _keepRatio = KeepRatio::yes);
    QPixmap renderSvgScaled(
        const QString& _resourcePath,
        const QSize& _unscaledSize = {},
        const QColor& _tintColor = {},
        const KeepRatio _keepRatio = KeepRatio::yes);

    template<typename T>
    using Layers = std::vector<std::pair<QString, T>>;

    using ColorLayers = Layers<QColor>;
    QPixmap renderSvgLayered(const QString& _filePath, const ColorLayers& _layers = {}, const QSize& _scaledSize = {});

    struct BasePixmap
    {
        BasePixmap(const QPixmap& _pixmap);
        virtual ~BasePixmap() {};

        QPixmap cachedPixmap() const { return pixmap_; }
        virtual QPixmap actualPixmap() { return pixmap_; }

    protected:
        void setPixmap(const QPixmap& _pixmap);

    private:
        QPixmap pixmap_;
    };

    struct BaseStyledPixmap : BasePixmap
    {
        QPixmap actualPixmap() override final;
        virtual bool canUpdate() const = 0;
        void updatePixmap();

    protected:
        BaseStyledPixmap(const QPixmap& _pixmap, const QString& _path, const QSize& _size);

        virtual QPixmap generatePixmap() = 0;

        QString path() const { return path_; }
        QSize size() const { return size_; }

    private:
        QString path_;
        QSize size_;
    };

    struct StyledPixmap final : BaseStyledPixmap
    {
        explicit StyledPixmap();
        explicit StyledPixmap(
            const QString& _resourcePath,
            const QSize& _scaledSize = {},
            const Styling::ColorParameter& _tintColor = Styling::ColorParameter(),
            KeepRatio _keepRatio = KeepRatio::yes);

        static StyledPixmap scaled(
            const QString& _resourcePath,
            const QSize& _unscaledSize = {},
            const Styling::ColorParameter& _tintColor = Styling::ColorParameter(),
            KeepRatio _keepRatio = KeepRatio::yes);

        bool canUpdate() const override;

    protected:
        QPixmap generatePixmap() override;

    private:
        friend bool operator==(const StyledPixmap& _lhs, const StyledPixmap& _rhs);

    private:
        Styling::ColorContainer color_;
        KeepRatio keepRatio_;
    };

    using ColorParameterLayers = Layers<Styling::ColorParameter>;
    using ColorContainerLayers = Layers<Styling::ColorContainer>;

    struct LayeredPixmap final : BaseStyledPixmap
    {
        LayeredPixmap();
        LayeredPixmap(const QString& _filePath, const ColorParameterLayers& _layers = {}, const QSize& _scaledSize = {});

        bool canUpdate() const override;

    protected:
        QPixmap generatePixmap() override;

    private:
        ColorContainerLayers layers_;
    };
} // namespace Utils