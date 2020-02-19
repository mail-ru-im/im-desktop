#pragma once

namespace Emoji
{

    struct EmojiRecord;
    using EmojiRecordVec = std::vector<EmojiRecord>;

    class EmojiCode;

    namespace mac {
        QImage getEmoji(const EmojiCode& code, int rectSize);
        bool supportEmoji(const EmojiCode& code);
        void setEmojiVector(const EmojiRecordVec& vector);
        bool isSkipEmojiFullCode(const EmojiCode& code);
    }
}
