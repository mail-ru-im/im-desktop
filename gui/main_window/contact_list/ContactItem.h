#pragma once

#include "../../types/contact.h"
#include "../../main_window/contact_list/contact_profile.h"

namespace Logic
{
    class contact_profile;

    class ContactItem
    {
    public:

        explicit ContactItem(Data::ContactPtr _contact);

        bool operator== (const ContactItem& _other) const;

        Data::Contact* Get() const;

        bool is_visible() const;
        void set_visible(bool);
        bool is_group() const;
        bool is_online() const;
        bool is_phone() const;
        bool recently() const;
        bool recently(const QDateTime& _current) const;
        bool is_active(const QDateTime& _current) const;
        bool is_chat() const;
        bool is_muted() const;
        bool is_live_chat() const;
        bool is_official() const;
        bool is_public() const;

        bool is_checked() const;
        void set_checked(bool _isChecked);

        void set_chat_role(const QString& role);
        const QString& get_chat_role() const;

        void set_default_role(const QString& role);
        const QString& get_default_role() const;

        const QString& get_aimid() const;

        int get_outgoing_msg_count() const;
        void set_outgoing_msg_count(const int _count);

        void set_stamp(const QString& _stamp);
        const QString& get_stamp() const;

        bool is_channel() const;
        bool is_readonly() const;

    private:

        std::shared_ptr<Data::Contact>                contact_;


        bool            visible_;
        QString         chat_role_;
        QString         default_role_;
        QString         stamp_;
    };

    static_assert(std::is_move_assignable<ContactItem>::value, "ContactItem must be move assignable");
    static_assert(std::is_move_constructible<ContactItem>::value, "ContactItem must be move constructible");
}
