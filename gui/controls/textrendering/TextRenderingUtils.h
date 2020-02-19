#pragma once

namespace Ui
{
    namespace TextRendering
    {
        constexpr size_t maximumEmojiCount() noexcept { return 3; }

        const QFontMetricsF& getMetrics(const QFont& _font);
        double textSpaceWidth(const QFont& _font);
        int ceilToInt(double _value);
        int roundToInt(double _value);
        double textWidth(const QFont& _font, const QString& _text);
        double textAscent(const QFont& _font);
        double averageCharWidth(const QFont& _font);
        int textHeight(const QFont& _font);
        bool isLinksUnderlined();

        enum class EmojiScale;
        EmojiScale getEmojiScale(const int _emojiCount = 0);
        int emojiHeight(const QFont& _font, const int _emojiCount = 0);
        int getEmojiSize(const QFont& _font, const int _emojiCount = 0);
        bool isEmoji(const QStringRef& _text);

        QString elideText(const QFont& _font, const QString& _text, const int _width);

        QString stringFromCode(const int _code);

        class TextWord;
        double getLineWidth(const std::vector<TextWord>& _line);
        size_t getEmojiCount(const std::vector<TextWord>& _words);
        int getHeightCorrection(const TextWord& _word, const int _emojiCount);
    }
}