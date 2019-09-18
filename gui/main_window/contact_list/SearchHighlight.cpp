#include "stdafx.h"

#include "SearchHighlight.h"
#include "styles/ThemeParameters.h"
#include "utils/utils.h"

namespace Ui
{
    QColor getHighlightColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_RAINBOW_WARNING);
    }

    QColor getTextHighlightedColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY);
    }

    HighlightFindResult findNextHighlight(const QString & _text, const highlightsV& _highlights, const int _posStart, const HighlightPosition _hlPos)
    {
        static const auto delimeters = qsl("!\"#$%&\'()*+,-./[\\]^_`{|}~:;<=>?@ \n\r\t");

        std::vector<HighlightFindResult> results;
        results.reserve(_highlights.size());

        for (const auto& h : _highlights)
        {
            if (h.isEmpty())
                continue;

            auto ndx = _text.indexOf(h, _posStart, Qt::CaseInsensitive);

            if (ndx != -1)
            {
                if (_hlPos == HighlightPosition::wordStart)
                {
                    if (ndx == 0)
                    {
                        results.push_back(HighlightFindResult(ndx, h.length()));
                    }
                    else
                    {
                        while (ndx != -1 && !delimeters.contains(_text.at(ndx - 1)))
                            ndx = _text.indexOf(h, ndx + h.length(), Qt::CaseInsensitive);

                        if (ndx != -1)
                            results.push_back(HighlightFindResult(ndx, h.length()));
                    }
                }
                else
                {
                    results.push_back(HighlightFindResult(ndx, h.length()));
                }
            }
        }

        if (results.empty())
            return HighlightFindResult(-1, 0);

        const auto it = std::min_element(results.begin(), results.end(), [](const auto& _res1, const auto& _res2)
        {
            if (_res1.indexStart_ == _res2.indexStart_)
                return _res1.length_ > _res2.length_;

            return _res1.indexStart_ < _res2.indexStart_;
        });
        return *it;
    }

    TextRendering::TextUnitPtr createHightlightedText(
        const QString& _text,
        const QFont& _font,
        const QColor& _baseColor,
        const QColor& _hlColor,
        const int _maxLines,
        const highlightsV& _highlights,
        const Data::MentionMap& _mentions,
        const HighlightPosition _hlPos)
    {
        if (_text.isEmpty())
            return TextRendering::TextUnitPtr();

        std::list<TextRendering::TextUnitPtr> units;
        const auto appendUnit = [&units, &_font, &_mentions, _maxLines](const QString& _fragment, const QColor& _color, const bool _hl)
        {
            auto unit = TextRendering::MakeTextUnit(_fragment, _mentions, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            unit->init(_font, _color, _color, _color, getHighlightColor(), TextRendering::HorAligment::LEFT, _maxLines);
            unit->setHighlighted(_hl);
            units.push_back(std::move(unit));
        };

        const auto convertedText = Utils::convertMentions(_text, _mentions);
        int curIndex = 0;
        auto res = findNextHighlight(convertedText, _highlights, curIndex, _hlPos);
        while (res.indexStart_ != -1)
        {
            if (curIndex < res.indexStart_ && res.indexStart_ - curIndex > 0)
                appendUnit(convertedText.mid(curIndex, res.indexStart_ - curIndex), _baseColor, false);

            appendUnit(convertedText.mid(res.indexStart_, res.length_), _hlColor, true);

            curIndex = res.indexStart_ + res.length_;
            res = findNextHighlight(convertedText, _highlights, curIndex, _hlPos);
        }

        if (convertedText.length() - curIndex > 0)
            appendUnit(convertedText.mid(curIndex, convertedText.length() - curIndex), _baseColor, false);

        auto firstUnit = std::move(units.front());
        units.pop_front();
        for (auto&& u : units)
            firstUnit->append(std::move(u));

        firstUnit->evaluateDesiredSize();

        return firstUnit;
    }
}


