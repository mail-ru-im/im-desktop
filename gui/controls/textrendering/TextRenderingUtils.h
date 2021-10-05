#pragma once

#include "../gui/types/message.h"

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

        //! Includes width of right exceeding regular width part of last symbol of italic text
        double textVisibleWidth(const QFont& _font, const QString& _text);

        double textAscent(const QFont& _font);
        double averageCharWidth(const QFont& _font);
        int textHeight(const QFont& _font);
        bool isLinksUnderlined();

        enum class EmojiScale;
        constexpr EmojiScale getEmojiScale(int _emojiCount = 0) noexcept;

        int getEmojiSize(const QFont& _font, int _emojiCount);
        int getEmojiSizeSmartreply(const QFont& _font, int _emojiCount);

        bool isEmoji(QStringView _text, qsizetype& pos);
        bool isEmoji(QStringView _text);

        QString elideText(const QFont& _font, const QString& _text, int _width);
        Data::FStringView elideText(const QFont& _font, Data::FStringView _text, int _width);

        QString stringFromCode(int _code);
        QString spaceAsString();

        class TextWord;
        double getLineWidth(const std::vector<TextWord>& _line);
        size_t getEmojiCount(const std::vector<TextWord>& _words);
        int getHeightCorrection(const TextWord& _word, int _emojiCount);
    }
}