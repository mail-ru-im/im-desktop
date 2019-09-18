#include "stdafx.h"

#include "contact_profile.h"
#include "utils/utils.h"

namespace Logic
{
    bool address::unserialize(Ui::gui_coll_helper _coll)
    {
        city_ = _coll.get<QString>("city");
        state_ = _coll.get<QString>("state");
        country_ = _coll.get<QString>("country");

        return true;
    }

    bool phone::unserialize(Ui::gui_coll_helper _coll)
    {
        phone_ = QString::fromUtf8(_coll.get_value_as_string("phone"));
        type_ = QString::fromUtf8(_coll.get_value_as_string("type"));

        return true;
    }


    contact_profile::contact_profile()
    {
    }

    contact_profile::~contact_profile()
    {
    }

    bool contact_profile::unserialize(Ui::gui_coll_helper _coll)
    {
        aimid_ = QString::fromUtf8(_coll.get_value_as_string("aimid"));
        first_name_ = QString::fromUtf8(_coll.get_value_as_string("firstname"));
        last_name_ = QString::fromUtf8(_coll.get_value_as_string("lastname"));
        friendly_ = QString::fromUtf8(_coll.get_value_as_string("friendly"));
        friendly_name_ = QString::fromUtf8(_coll.get_value_as_string("friendly_name"));
        if (_coll.is_value_exist("nick_name"))
            nick_ = QString::fromUtf8(_coll.get_value_as_string("nick_name"));
        relationship_ = QString::fromUtf8(_coll.get_value_as_string("relationship"));
        about_ = QString::fromUtf8(_coll.get_value_as_string("about"));
        birthdate_ = _coll.get_value_as_int64("birthdate");
        gender_ = QString::fromUtf8(_coll.get_value_as_string("gender"));

        Ui::gui_coll_helper coll_address(_coll.get_value_as_collection("homeaddress"), false);
        home_address_.unserialize(coll_address);

        if (const auto phones = _coll.get_value_as_array("phones"))
        {
            const auto size = phones->size();
            phones_.reserve(size);
            for (int i = 0; i < size; ++i)
            {
                Ui::gui_coll_helper coll_phone(phones->get_at(i)->get_as_collection(), false);
                phone p;
                p.unserialize(coll_phone);
                phones_.push_back(std::move(p));
            }
        }

        return true;
    }

    bool contact_profile::unserialize2(Ui::gui_coll_helper _coll)
    {
        {
            aimid_ = QString::fromUtf8(_coll.get_value_as_string("aimid"));
            const int pos = aimid_.indexOf(ql1s("@uin.icq"));
            if (pos != -1)
                aimid_ = std::move(aimid_).left(pos);
        }
        first_name_ = QString::fromUtf8(_coll.get_value_as_string("first_name"));
        last_name_ = QString::fromUtf8(_coll.get_value_as_string("last_name"));
        nick_ = QString::fromUtf8(_coll.get_value_as_string("nick_name"));
        friendly_ = QString::fromUtf8(_coll.get_value_as_string("friendly"));
        if (friendly_.isEmpty())
        {
            const auto sep = ql1s(first_name_.isEmpty() ? "" : " ");
            friendly_ = first_name_ % sep % last_name_;
            if (friendly_.isEmpty())
            {
                if (has_nick())
                    friendly_ = get_nick_pretty();
                else
                    friendly_ = aimid_;
            }
        }
        about_ = QString::fromUtf8(_coll.get_value_as_string("about"));
        {
            if (_coll.is_value_exist("birthdate"))
            {
                Ui::gui_coll_helper coll_birthdate(_coll.get_value_as_collection("birthdate"), false);
                if (coll_birthdate.is_value_exist("year") && coll_birthdate.is_value_exist("month") && coll_birthdate.is_value_exist("day"))
                {
                    const QDate d(coll_birthdate.get_value_as_int("year"), coll_birthdate.get_value_as_int("month"), coll_birthdate.get_value_as_int("day"));
                    birthdate_ = QDateTime(d).toMSecsSinceEpoch();
                }
            }
        }
        gender_ = QString::fromUtf8(_coll.get_value_as_string("gender"));
        home_address_ = address(QString::fromUtf8(_coll.get_value_as_string("city")),
            QString::fromUtf8(_coll.get_value_as_string("state")),
            QString::fromUtf8(_coll.get_value_as_string("country")));

        mutual_friend_count_ = _coll.get_value_as_int("mutual_count");

        return true;
    }

    const QString& contact_profile::get_aimid() const
    {
        return aimid_;
    }

    const QString& contact_profile::get_first_name() const
    {
        return first_name_;
    }

    const QString& contact_profile::get_last_name() const
    {
        return last_name_;
    }

    const QString& contact_profile::get_friendly() const
    {
        return friendly_;
    }

    const QString& contact_profile::get_friendly_name() const
    {
        return friendly_name_;
    }

    const QString& contact_profile::get_nick() const
    {
        return nick_;
    }

    bool contact_profile::has_nick() const noexcept
    {
        return !get_nick().isEmpty();
    }

    QString contact_profile::get_nick_pretty() const
    {
        if (has_nick())
            return Utils::makeNick(get_nick());
        else if (aimid_.contains(ql1c('@')) && !Utils::isChat(aimid_))
            return aimid_;

        return QString();
    }

    const QString& contact_profile::get_relationship() const
    {
        return relationship_;
    }

    const QString& contact_profile::get_about() const
    {
        return about_;
    }

    int64_t contact_profile::get_birthdate() const
    {
        return birthdate_;
    }

    const QString& contact_profile::get_gender() const
    {
        return gender_;
    }

    const address& contact_profile::get_home_address() const
    {
        return home_address_;
    }

    const address& contact_profile::get_origin_address() const
    {
        return origin_address_;
    }

    const QString& contact_profile::get_privatekey() const
    {
        return privatekey_;
    }

    const email_list& contact_profile::get_emails() const
    {
        return emails_;
    }

    const phone_list& contact_profile::get_phones() const
    {
        return phones_;
    }

    const interest_list& contact_profile::get_interests() const
    {
        return interests_;
    }

    int32_t contact_profile::get_children() const
    {
        return children_;
    }

    const QString& contact_profile::get_religion() const
    {
        return religion_;
    }

    const QString& contact_profile::get_sex_orientation() const
    {
        return sex_orientation_;
    }

    bool contact_profile::get_smoking() const
    {
        return smoking_;
    }

    int32_t contact_profile::get_mutual_friends_count() const
    {
        return mutual_friend_count_;
    }

    QString contact_profile::get_contact_name() const
    {
        const auto& friendly = get_friendly();
        if (!QStringRef(&friendly).trimmed().isEmpty())
            return friendly;

        const auto& nick = get_nick();
        if (!QStringRef(&nick).trimmed().isEmpty())
            return get_nick_pretty();

        const auto& first_name = get_first_name();
        if (!QStringRef(&first_name).trimmed().isEmpty())
            return first_name % ql1c(' ') % get_last_name();

        return get_aimid();
    }
}

