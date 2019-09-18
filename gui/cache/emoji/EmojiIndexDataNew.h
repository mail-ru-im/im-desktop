#pragma once

#include <vector>
#include <memory>

namespace Emoji
{
    struct EmojiRecord;
    using EmojiRecordSptr = std::shared_ptr<EmojiRecord>;

    std::vector<EmojiRecordSptr> getEmojiVector();
}
