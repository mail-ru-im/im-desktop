#pragma once

#include "message.h"

namespace Data
{

struct ThreadFeedItemData
{
    QString threadId_;
    std::shared_ptr<MessageParentTopic> parent_;
    MessageBuddySptr parentMessage_;
    Data::MessageBuddies messages_;
    int64_t olderMsgId_ = 0;
    int64_t repliesCount_ = 0;
    int64_t unreadCount_ = 0;
};

}
