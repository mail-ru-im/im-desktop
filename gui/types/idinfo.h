#pragma once

namespace Data
{
    struct IdInfo
    {
        enum class IdType
        {
            Invalid,
            User,
            Chat
        };

        QString description_;
        QString name_;
        QString nick_;
        QString sn_;

        int64_t memberCount_ = 0;
        QString stamp_;

        IdType type_ = IdType::Invalid;

        bool isValid() const noexcept { return type_ != IdType::Invalid; }
        bool isChatInfo() const noexcept { return type_ == IdType::Chat; }
    };

    IdInfo UnserializeIdInfo(core::coll_helper& helper);
}
