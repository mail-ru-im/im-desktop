#include "stdafx.h"

#include "TextRenderingUtils.h"
#include "TextRendering.h"

#include "styles/ThemesContainer.h"

namespace Ui
{
    namespace TextRendering
    {
        const QFontMetricsF& getMetrics(const QFont& _font)
        {
            static std::map<QFont, QFontMetricsF> cachedMetrics;

            auto metrics = cachedMetrics.find(_font);
            if (metrics == cachedMetrics.end())
                metrics = cachedMetrics.insert({ _font, QFontMetricsF(_font) }).first;

            return metrics->second;
        }

        double textSpaceWidth(const QFont& _font)
        {
            static std::map<QFont, double> spaces;
            auto width = spaces.find(_font);
            if (width == spaces.end())
                width = spaces.insert({ _font, textWidth(_font, qsl(" ")) }).first;

            return width->second;
        }

        int ceilToInt(double _value)
        {
            return static_cast<int>(std::ceil(_value));
        }

        int roundToInt(double _value)
        {
            return static_cast<int>(std::round(_value));
        }

        double textWidth(const QFont& _font, const QString& _text)
        {
            return getMetrics(_font).width(_text);
        }

        double textAscent(const QFont& _font)
        {
            return getMetrics(_font).ascent();
        }

        double averageCharWidth(const QFont& _font)
        {
            return getMetrics(_font).averageCharWidth();
        }

        int textHeight(const QFont& _font)
        {
            return roundToInt(getMetrics(_font).height());
        }

        bool isLinksUnderlined()
        {
            return Styling::getThemesContainer().getCurrentTheme()->isLinksUnderlined();
        }

        EmojiScale getEmojiScale(const int _emojiCount)
        {
            switch (_emojiCount)
            {
            case 1:
                return Ui::TextRendering::EmojiScale::BIG;
            case 2:
                return Ui::TextRendering::EmojiScale::MEDIUM;
            case 3:
                return Ui::TextRendering::EmojiScale::REGULAR;
            default:
                return Ui::TextRendering::EmojiScale::NORMAL;
            }
        }

        int emojiHeight(const QFont& _font, const int _emojiCount)
        {
            int height = textHeight(_font) + Utils::scale_value(1);

            const Ui::TextRendering::EmojiScale _scale = getEmojiScale(_emojiCount);

            switch (_scale)
            {
            case Ui::TextRendering::EmojiScale::REGULAR:
                height = platform::is_apple() ? Utils::scale_value(37) : Utils::scale_value(32);
                break;
            case Ui::TextRendering::EmojiScale::MEDIUM:
                height = platform::is_apple() ? Utils::scale_value(45) : Utils::scale_value(40);
                break;
            case Ui::TextRendering::EmojiScale::BIG:
                height = platform::is_apple() ? Utils::scale_value(53) : Utils::scale_value(48);
                break;
            default:
                break;
            }
            return height;
        }

        int getEmojiSize(const QFont& _font, const int _emojiCount)
        {
            auto result = emojiHeight(_font, _emojiCount);
            if (platform::is_apple())
                result += Utils::scale_value(2);

            return result;
        }

        bool isEmoji(const QStringRef& _text)
        {
            int pos = 0;

            return (Emoji::getEmoji(_text, pos) != Emoji::EmojiCode() && Emoji::getEmoji(_text, pos) == Emoji::EmojiCode());
        }

        QString elideText(const QFont& _font, const QString& _text, const int _width)
        {
            const auto& metrics = getMetrics(_font);
            QString result;
            int approxWidth = 0;
            int i = 0;
            while (i < _text.size() && approxWidth < _width)
            {
                auto c = _text[i++];
                result += c;
                approxWidth += metrics.width(c);
            }

            i = result.size();
            while (roundToInt(metrics.width(result)) < _width && i < _text.size())
                result.push_back(_text[i++]);

            while (!result.isEmpty() && roundToInt(metrics.width(result)) > _width)
                result.chop(1);

            return result;
        }

        QString stringFromCode(const int _code)
        {
            QString result;

            if (QChar::requiresSurrogates(_code))
            {
                result.reserve(2);
                result += (QChar)(QChar::highSurrogate(_code));
                result += (QChar)(QChar::lowSurrogate(_code));
            }
            else
            {
                result.reserve(1);
                result += (QChar)_code;
            }

            return result;
        }

        double getLineWidth(const std::vector<TextWord>& _line)
        {
            const auto sum = [](double _sum, const auto& _w)
            {
                auto width = _sum + _w.cachedWidth();
                if (_w.isSpaceAfter())
                    width += _w.spaceWidth();
                return width;
            };

            return std::accumulate(_line.begin(), _line.end(), 1.0, sum);
        }

        size_t getEmojiCount(const std::vector<TextWord>& _words)
        {
            if (_words.size() > maximumEmojiCount())
                return 0;

            if (std::all_of(_words.begin(), _words.end(), [](const auto& w)
            {
                return (w.isEmoji() && !w.isSpaceAfter());
            }))
                return _words.size();
            else
                return 0;
        }

        int getHeightCorrection(const TextWord& _word, const int _emojiCount)
        {
            if constexpr (platform::is_apple())
            {
                if (_word.isResizable() && _emojiCount != 0)
                    return Utils::scale_value(3);

                if ((_word.emojiSize() - textHeight(_word.getFont())) > 2)
                    return Utils::scale_value(-4);
            }
            else if (_emojiCount != 0)
            {
                return Utils::scale_value(4);
            }

            return 0;
        }
    }
}