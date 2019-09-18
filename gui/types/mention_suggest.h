#pragma once

namespace core
{
    class coll_helper;
}

namespace Logic
{
    struct MentionSuggest
    {
        QString aimId_;
        QString friendlyName_;
        QString nick_;

        QString highlight_;

        MentionSuggest() = default;
        MentionSuggest(const QString& _aimid, const QString& _friendly, const QString& _highlight, const QString& _nick = QString());

        bool isServiceItem() const;
        void checkNick();

        void unserialize(core::coll_helper& _coll);

        bool operator==(const MentionSuggest& _other) const
        {
            return aimId_ == _other.aimId_;
        }
    };

    using MentionsSuggests = std::vector<MentionSuggest>;
}

Q_DECLARE_METATYPE(Logic::MentionSuggest);