#pragma once

#include "../../types/contact.h"
#include "../../main_window/contact_list/contact_profile.h"

namespace Logic
{
    class contact_profile;
    typedef std::shared_ptr<contact_profile> profile_ptr;

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

        void set_input_text(const QString& _inputText);
        const QString& get_input_text() const;

        void set_input_pos(int _pos);
        int get_input_pos() const;

        void set_contact_profile(profile_ptr _profile);
        profile_ptr getContactProfile() const;
        const QString& get_aimid() const;

        int get_outgoing_msg_count() const;
        void set_outgoing_msg_count(const int _count);

        void set_stamp(const QString& _stamp);
        const QString& get_stamp() const;

        const std::vector<phone> get_phones() const;

        bool is_channel() const;
        bool is_readonly() const;

    private:

        std::shared_ptr<Data::Contact>                contact_;
        std::shared_ptr<Logic::contact_profile>        profile_;


        bool            visible_;
        QString         input_text_;
        QString         chat_role_;
        QString         default_role_;
        QString         stamp_;
        int             input_pos_;
    };

    static_assert(std::is_move_assignable<ContactItem>::value, "ContactItem must be move assignable");
    static_assert(std::is_move_constructible<ContactItem>::value, "ContactItem must be move constructible");
}
