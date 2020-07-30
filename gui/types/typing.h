#pragma once

namespace Logic
{
    struct TypingFires
    {
        QString aimId_;
        QString chatterAimId_;
        QString chatterName_;

        std::chrono::milliseconds time_;

        TypingFires() : time_(QDateTime::currentMSecsSinceEpoch()) {}
        TypingFires(QString _aimid, QString _chatterAimid, QString _chatterName)
            : aimId_(std::move(_aimid))
            , chatterAimId_(std::move(_chatterAimid))
            , chatterName_(std::move(_chatterName))
            , time_(QDateTime::currentMSecsSinceEpoch())
        {
        }

        bool operator == (const TypingFires& _other) const
        {
            return (aimId_ == _other.aimId_ && chatterAimId_ == _other.chatterAimId_);
        }

        QString getChatterName() const;
        void refreshTime();
    };
}
