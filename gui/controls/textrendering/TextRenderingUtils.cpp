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
                width = spaces.insert({ _font, textWidth(_font, spaceAsString()) }).first;

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
            return getMetrics(_font).horizontalAdvance(_text);
        }

        double textVisibleWidth(const QFont& _font, const QString& _text)
        {
            if (QStringView(_text).trimmed().isEmpty())
                return textWidth(_font, _text);
            else
                return getMetrics(_font).boundingRect(_text).width();
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

        constexpr EmojiScale getEmojiScale(const int _emojiCount) noexcept
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
                break;
            }

            return Ui::TextRendering::EmojiScale::NORMAL;
        }

        constexpr int emojiHeight(int _emojiCount) noexcept
        {
            switch (getEmojiScale(_emojiCount))
            {
            case Ui::TextRendering::EmojiScale::REGULAR:
                return platform::is_apple() ? 39 : 32;
            case Ui::TextRendering::EmojiScale::MEDIUM:
                return platform::is_apple() ? 47 : 40;
            case Ui::TextRendering::EmojiScale::BIG:
                return platform::is_apple() ? 55 : 48;
            default:
                break;
            }

            return 0;
        }

        int getEmojiSize(const QFont& _font, int _emojiCount)
        {
            if (const auto h = emojiHeight(_emojiCount); h > 0)
                return Utils::scale_value(h);

            return textHeight(_font) + Utils::scale_value(platform::is_apple() ? 3 : 1);
        }

        constexpr int emojiHeightSmartreply(int _emojiCount) noexcept
        {
            switch (getEmojiScale(_emojiCount))
            {
                case Ui::TextRendering::EmojiScale::REGULAR:
                    return 31;
                case Ui::TextRendering::EmojiScale::MEDIUM:
                    return 35;
                case Ui::TextRendering::EmojiScale::BIG:
                    return 39;
                default:
                    break;
            }

            return 0;
        }

        int getEmojiSizeSmartreply(const QFont& _font, int _emojiCount)
        {
            if (const auto h = emojiHeightSmartreply(_emojiCount); h > 0)
                return Utils::scale_value(h);

            return getEmojiSize(_font, _emojiCount);
        }

        bool isEmoji(QStringView _text)
        {
            qsizetype pos = 0;

            return (Emoji::getEmoji(_text, pos) != Emoji::EmojiCode() && Emoji::getEmoji(_text, pos) == Emoji::EmojiCode());
        }

        QString elideText(const QFont& _font, const QString& _text, int _width)
        {
            const auto& metrics = getMetrics(_font);
            QString result;
            int approxWidth = 0;
            int i = 0;
            while (i < _text.size() && approxWidth < _width)
            {
                auto c = _text[i++];
                result += c;
                approxWidth += metrics.horizontalAdvance(c);
            }

            i = result.size();
            while (roundToInt(metrics.horizontalAdvance(result)) < _width && i < _text.size())
                result.push_back(_text[i++]);

            while (!result.isEmpty() && roundToInt(metrics.horizontalAdvance(result)) > _width)
                result.chop(1);

            return result;
        }

        Data::FormattedStringView elideText(const QFont& _font, Data::FormattedStringView _text, int _width)
        {
            const auto elidedPlainText = elideText(_font, _text.string().toString(), _width);
            auto result = _text.mid(0, 0);
            if (result.tryToAppend(elidedPlainText))
            {
                return result;
            }
            else
            {
                im_assert(false);
                return _text;
            }
        }

        QString stringFromCode(int _code)
        {
            if (QChar::requiresSurrogates(_code))
                return QChar(QChar::highSurrogate(_code)) % QChar((QChar::lowSurrogate(_code)));
           return QString(QChar(_code));
        }

        QString spaceAsString()
        {
            return qsl(" ");
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

            if (std::all_of(_words.begin(), _words.end(), [](const auto& w) { return (w.isEmoji() && !w.isSpaceAfter()); }))
                return _words.size();

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