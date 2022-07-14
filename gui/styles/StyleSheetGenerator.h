#pragma once

#include "ThemeColor.h"
#include "ThemeParameters.h"

namespace Styling
{
    class BaseStyleSheetGenerator
    {
    public:
        BaseStyleSheetGenerator() = default;
        virtual ~BaseStyleSheetGenerator() = default;

        virtual QString generate() const = 0;
    };

    class ArrayStyleSheetGenerator : public BaseStyleSheetGenerator
    {
    public:
        ArrayStyleSheetGenerator(const QString& _pattern, std::vector<ThemeColorKey>&& _colors);

        QString generate() const override;

    private:
        std::vector<ThemeColorKey> colors_;
        QString pattern_;
    };

    class MapStyleSheetGenerator : public BaseStyleSheetGenerator
    {
    public:
        MapStyleSheetGenerator(const QString& _pattern, std::vector<std::pair<QString, StyleVariable>>&& _colors);

        QString generate() const override;

    private:
        std::vector<std::pair<QString, StyleVariable>> colors_;
        QString pattern_;
    };

    class CombinedStyleSheetGenerator : public BaseStyleSheetGenerator
    {
    public:
        CombinedStyleSheetGenerator(std::vector<std::unique_ptr<BaseStyleSheetGenerator>>&& _styleSheets);

        QString generate() const override;

    private:
        std::vector<std::unique_ptr<BaseStyleSheetGenerator>> styleSheets_;
    };
} // namespace Styling
