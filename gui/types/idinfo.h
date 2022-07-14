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
        QString firstName_;
        QString middleName_;
        QString lastName_;

        bool isBot_ = false;

        int64_t memberCount_ = 0;
        QString stamp_;

        IdType type_ = IdType::Invalid;

        bool isValid() const noexcept { return type_ != IdType::Invalid; }
        bool isChatInfo() const noexcept { return type_ == IdType::Chat; }
        QString getName() const;
    };

    IdInfo UnserializeIdInfo(core::coll_helper& helper);
}
