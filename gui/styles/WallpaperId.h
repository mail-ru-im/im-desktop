#pragma once

namespace Styling
{
    class WallpaperId
    {
        QString id_;
        size_t idHash_ = 0;

    public:
        WallpaperId() = default;
        WallpaperId(const int _id) : id_(QString::number(_id)), idHash_{ qHash(id_) } {}
        WallpaperId(const QString& _id) : id_(_id), idHash_{ qHash(id_) } {}
        WallpaperId(QString&& _id) : id_(std::move(_id)), idHash_{ qHash(id_) } {}

        QString id() const { return id_; }
        size_t idHash() const { return idHash_; }

        void setId(const QString& _id) { id_ = _id; idHash_ = qHash(id_); }

        bool operator==(const WallpaperId& _other) const { return id_ == _other.id_; }
        bool operator!=(const WallpaperId& _other) const { return !(*this == _other); }

        bool isUser() const { return id_.startsWith(ql1c('u')); }
        bool isValid() const { return !id_.isEmpty(); }

        static WallpaperId fromContact(const QString& _contact)
        {
            im_assert(!_contact.isEmpty());
            if (_contact.isEmpty())
                return invalidId();

            return WallpaperId(ql1c('u') % _contact);
        }

        static const WallpaperId& invalidId()
        {
            static const WallpaperId id;
            return id;
        }
    };
}