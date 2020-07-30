#pragma once

namespace core
{
    struct icollection;
}

namespace Data
{
    struct PollData
    {
        enum Type
        {
            None,
            PublicPoll,
            AnonymousPoll
        };

        struct Answer
        {
            QString text_;
            QString answerId_;
            int64_t votes_ = 0;
        };

        using AnswerPtr = std::shared_ptr<Answer>;

        void serialize(core::icollection* _collection) const;
        void unserialize(core::icollection* _collection);

        Type type_ = AnonymousPoll;
        int64_t votes_ = 0;
        QString id_;
        QString myAnswerId_;
        bool closed_ = false;
        bool canStop_ = false;

        std::vector<AnswerPtr> answers_;
        std::map<QString, AnswerPtr> answersIndex_;
    };

    using Poll = std::optional<PollData>;

}
