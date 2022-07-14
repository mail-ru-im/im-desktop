#include "stdafx.h"
#include "StyleSheetGenerator.h"
#include "ThemeParameters.h"

namespace Styling
{
    ArrayStyleSheetGenerator::ArrayStyleSheetGenerator(const QString& _pattern, std::vector<ThemeColorKey>&& _colors)
        : colors_ { std::move(_colors) }
        , pattern_ { _pattern }
    {}

    QString ArrayStyleSheetGenerator::generate() const
    {
        QString result = pattern_;
        for (const auto& color : colors_)
            result = result.arg(getColorHex(color));
        return result;
    }

    MapStyleSheetGenerator::MapStyleSheetGenerator(const QString& _pattern, std::vector<std::pair<QString, StyleVariable>>&& _colors)
        : colors_{ std::move(_colors) }
        , pattern_{ _pattern }
    {}

    QString MapStyleSheetGenerator::generate() const
    {
        QString result = pattern_;
        const auto params = getParameters();
        for (const auto& [name, color] : colors_)
            result = result.replace(name, params.getColorHex(color));
        return result;
    }

    CombinedStyleSheetGenerator::CombinedStyleSheetGenerator(std::vector<std::unique_ptr<BaseStyleSheetGenerator>>&& _styleSheets)
        : styleSheets_ { std::move(_styleSheets) }
    {}

    QString CombinedStyleSheetGenerator::generate() const
    {
        QString result;
        for (const auto& sheet : styleSheets_)
            result.append(sheet->generate());
        return result;
    }
} // namespace Styling
