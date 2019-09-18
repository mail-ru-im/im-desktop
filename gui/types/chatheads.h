#pragma once

namespace core
{
    class coll_helper;
}

namespace Data
{
    struct ChatHead
    {
        bool operator==(const ChatHead& _other) const
        {
            return aimid_ == _other.aimid_;
        }

        QString aimid_;
        QString friendly_;

        //gui animation flags
        bool adding_ = false;
        bool removing_ = false;
    };

    using HeadsVector = QVector<ChatHead>;
    using HeadsById = QMap<qint64, HeadsVector>;

    struct ChatHeads
    {
        bool resetState_ = false;
        QString aimId_;
        HeadsById heads_;
    };

    ChatHeads UnserializeChatHeads(core::coll_helper* helper);
    bool isSameHeads(const HeadsVector& _first, const HeadsVector& _second);
}

Q_DECLARE_METATYPE(Data::ChatHead);
Q_DECLARE_METATYPE(Data::HeadsVector);
Q_DECLARE_METATYPE(Data::ChatHeads);