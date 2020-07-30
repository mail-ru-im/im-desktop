#pragma once

#include "utils/gui_coll_helper.h"

namespace Data
{
    struct MessageReactionsData
    {
        void serialize(core::icollection* _collection) const;
        void unserialize(core::icollection* _collection);

        bool exist_ = false;
    };

    using MessageReactions = std::optional<MessageReactionsData>;

    struct Reactions
    {
        struct Reaction
        {
            QString reaction_;
            int64_t count_ = 0;

            void unserialize(core::icollection* _collection);
        };

        int64_t msgId_ = 0;
        QString myReaction_;
        std::vector<Reaction> reactions_;

        void unserialize(core::icollection* _collection);

        bool isEmpty() const;
    };


    struct ReactionsPage
    {
        struct Reaction
        {
            QString contact_;
            QString reaction_;
            int64_t time_;

            void unserialize(core::icollection* _collection);
        };

        bool exhausted_ = false;
        QString newest_;
        QString oldest_;
        std::vector<Reaction> reactions_;

        void unserialize(core::icollection* _collection);
    };

}
