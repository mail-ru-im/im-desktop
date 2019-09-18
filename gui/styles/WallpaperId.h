#pragma once

namespace Styling
{
    struct WallpaperId
    {
        QString id_;

        WallpaperId() = default;
        WallpaperId(const int _id) : id_(QString::number(_id)) {}
        WallpaperId(const QString& _id) : id_(_id) {}
        WallpaperId(QString&& _id) : id_(std::move(_id)) {}

        bool operator==(const WallpaperId& _other) const { return id_ == _other.id_; }
        bool operator!=(const WallpaperId& _other) const { return !(*this == _other); }

        bool isUser() const { return id_.startsWith(ql1c('u')); }
        bool isValid() const { return !id_.isEmpty(); }

        static WallpaperId fromContact(const QString& _contact)
        {
            assert(!_contact.isEmpty());
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