#pragma once

#include "../../common.shared/threads/thread_types.h"
#include "chat_member.h"

namespace core
{
    class coll_helper;
}

namespace Data
{
    struct ParentTopic
    {
        using Type = core::threads::parent_topic::type;

        explicit ParentTopic(Type _type)
            : type_{ _type}
        {}
        virtual ~ParentTopic() = default;
        virtual void serialize(core::coll_helper& _coll) const = 0;
        virtual void unserialize(const core::coll_helper& _coll) = 0;

        const Type type_;

        static Type unserialize_type(const core::coll_helper& _coll);
        static std::shared_ptr<ParentTopic> createParentTopic(Type _type);
    };

    struct MessageParentTopic : ParentTopic
    {
        explicit MessageParentTopic()
            : ParentTopic(Type::message)
        {}

        QString chat_;
        int64_t msgId_ = -1;

        static QString getChat(const ParentTopic* _topic);

        void serialize(core::coll_helper& _coll) const override;
        void unserialize(const core::coll_helper& _coll) override;
    };

    struct TaskParentTopic : ParentTopic
    {
        explicit TaskParentTopic()
            : ParentTopic(Type::task)
        {}

        QString taskId_;

        void serialize(core::coll_helper& _coll) const override;
        void unserialize(const core::coll_helper& _coll) override;
    };

    struct ThreadUpdate
    {
        QString threadId_;
        std::shared_ptr<ParentTopic> parent_;
        int repliesCount_ = 0;
        int unreadMessages_ = 0;
        int unreadMentionsMe_ = 0;
        int64_t yoursLastRead_ = -1;
        int64_t yoursLastReadMention_ = -1;
        bool isSubscriber_ = false;

        void unserialize(const core::coll_helper& _coll);
    };

    struct ThreadInfoShort
    {
        QString parentChatId_;
        int64_t msgId_ = -1;
        bool areYouSubscriber_ = false;
        bool isTaskThread_ = false;
    };

    struct ThreadInfo : public ThreadInfoShort
    {
        QString threadId_;
        int subscribersCount_ = -1;

        QVector<Data::ChatMemberInfo> subscribers_;

        void unserialize(const core::coll_helper& _coll);
    };

    using MessageThreadUpdatesMap = std::unordered_map<int64_t, std::shared_ptr<ThreadUpdate>>;

    struct ThreadUpdates
    {
        MessageThreadUpdatesMap messages_;
    };

    struct MessageThreadData
    {
        QString id_;
        std::shared_ptr<ParentTopic> parentTopic_;
        bool threadFeedMessage_ = false;
    };
}
