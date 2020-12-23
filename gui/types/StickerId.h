#pragma once

namespace Data
{
	struct StickerId
    {
        std::optional<QString> fsId_;
        struct StoreId
        {
            int32_t setId_ = -1;
            int32_t id_ = -1;
        };
        std::optional<StoreId> obsoleteId_; // for old clients

        StickerId() = default;
        explicit StickerId(const QString& _fsId) : fsId_(_fsId) {}
        StickerId(int32_t _setId, int32_t _id) : obsoleteId_(StoreId{ _setId , _id }) {}

        bool isEmpty() const noexcept { return !fsId_ && !obsoleteId_; }

        QString toString() const
        {
            if (fsId_)
                return *fsId_;
            else if (obsoleteId_)
                return u"ext:" % QString::number(obsoleteId_->setId_) % u":sticker:" % QString::number(obsoleteId_->id_);
            return {};
        }
    };
}