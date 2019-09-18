#pragma once

namespace Emoji
{

    struct EmojiRecord;
    using EmojiRecordSptr = std::shared_ptr<EmojiRecord>;
    using EmojiRecordSptrVec = std::vector<EmojiRecordSptr>;

    class EmojiCode;

    namespace mac {
        QImage getEmoji(const EmojiCode& code, int rectSize);
        bool supportEmoji(const EmojiCode& code);
        void setEmojiVector(const EmojiRecordSptrVec& vector);
        bool isSkipEmojiFullCode(const EmojiCode& code);
    }
}
