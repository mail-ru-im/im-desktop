#include "stdafx.h"
#include "mailboxes.h"
#include "../../core.h"
#include "../../../common.shared/json_helper.h"

namespace core
{
namespace wim
{
    mailbox::mailbox(const std::string& _email, unsigned int _unreads)
        : email_(_email)
        , unread_count_(_unreads)
    {
    }

    void mailbox::set_unreads(unsigned _unreads)
    {
        unread_count_ = _unreads;
    }

    unsigned mailbox::get_unreads() const
    {
        return unread_count_;
    }

    const std::string& mailbox::get_mailbox() const
    {
        return email_;
    }

    void mailbox::unserialize(const rapidjson::Value& _node)
    {
        if (!tools::unserialize_value(_node, "email", email_))
            return;

        tools::unserialize_value(_node, "unreadCount", unread_count_);
    }

    void mailbox::serialize(core::coll_helper _collection) const
    {
        _collection.set_value_as_string("email", email_);
        _collection.set_value_as_uint("unreads", unread_count_);
    }

    void mailbox::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        _node.AddMember("email", get_mailbox(), _a);
        _node.AddMember("unreadCount", get_unreads(), _a);
    }

    void mailbox_storage::process(const mailbox_changes& _changes, std::function<void(core::coll_helper, mailbox_change::type)> _notify_callback)
    {
        is_changed_ = true;
        bool init = false;

        for (const auto& change : _changes)
        {
            coll_helper coll_mailbox(g_core->create_collection(), true);

            switch (change->type_)
            {
            case mailbox_change::type::status:
                init = mailboxes_.find(change->email_) == mailboxes_.end();
                mailboxes_[change->email_] = mailbox(change->email_, change->get_unreads_count());

                serialize(coll_mailbox);
                coll_mailbox.set_value_as_bool("init", init);
                _notify_callback(coll_mailbox, change->type_);
                break;

            case mailbox_change::type::new_mail:
                mailboxes_[change->email_] = mailbox(change->email_, change->get_unreads_count());

                coll_mailbox.set_value_as_string("email", change->email_);
                coll_mailbox.set_value_as_string("from", ((mailbox_new_mail_change*)change.get())->from_);
                coll_mailbox.set_value_as_string("subj", ((mailbox_new_mail_change*)change.get())->subject_);
                coll_mailbox.set_value_as_string("uidl", ((mailbox_new_mail_change*)change.get())->uidl_);

                _notify_callback(coll_mailbox, change->type_);

                serialize(coll_mailbox);
                _notify_callback(coll_mailbox, mailbox_change::type::status);
                break;

            case mailbox_change::type::mail_read:
            default:
                break;
            }
        }
    }

    bool mailbox_storage::unserialize(const rapidjson::Value& _node_event_data, mailbox_changes& _changes)
    {
        const auto initial_size = _changes.size();

        if (std::string_view status; tools::unserialize_value(_node_event_data, "mailbox.status", status) && !status.empty())
        {
            rapidjson::Document doc;
            doc.Parse(status.data(), status.size());

            auto change = std::make_shared<mailbox_status_change>();
            tools::unserialize_value(doc, "email", change->email_);
            tools::unserialize_value(doc, "unreadCount", change->unreads_);
            _changes.push_back(std::move(change));
        }

        if (std::string_view readed; tools::unserialize_value(_node_event_data, "mailbox.messageReaded", readed) && !readed.empty())
        {
            rapidjson::Document doc;
            doc.Parse(readed.data(), readed.size());

            auto change = std::make_shared<mailbox_read_change>();
            tools::unserialize_value(doc, "email", change->email_);
            tools::unserialize_value(doc, "uidl", change->uidl_);
            _changes.push_back(std::move(change));
        }

        if (std::string_view new_msg; tools::unserialize_value(_node_event_data, "mailbox.newMessage", new_msg) && !new_msg.empty())
        {
            rapidjson::Document doc;
            doc.Parse(new_msg.data(), new_msg.size());

            auto change = std::make_shared<mailbox_new_mail_change>();
            tools::unserialize_value(doc, "email", change->email_);
            tools::unserialize_value(doc, "unreadCount", change->unreads_);
            tools::unserialize_value(doc, "uidl", change->uidl_);
            tools::unserialize_value(doc, "from", change->from_);
            tools::unserialize_value(doc, "subject", change->subject_);
            _changes.push_back(std::move(change));
        }

        return (_changes.size() - initial_size) != 0;
    }

    void mailbox_storage::serialize(core::coll_helper _collection) const
    {
        ifptr<iarray> mailbox_array(_collection->create_array());

        if (!mailboxes_.empty())
        {
            mailbox_array->reserve(mailboxes_.size());
            for (const auto& mailbox : mailboxes_)
            {
                coll_helper coll_mailbox(_collection->create_collection(), true);
                mailbox.second.serialize(coll_mailbox);
                ifptr<ivalue> val(_collection->create_value());
                val->set_as_collection(coll_mailbox.get());
                mailbox_array->push_back(val.get());
            }
        }

        _collection.set_value_as_array("mailboxes", mailbox_array.get());
    }

    void mailbox_storage::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        rapidjson::Value node_mailboxes(rapidjson::Type::kArrayType);
        node_mailboxes.Reserve(mailboxes_.size(), _a);
        for (const auto& mailbox : mailboxes_)
        {
            rapidjson::Value node_mailbox(rapidjson::Type::kObjectType);
            mailbox.second.serialize(node_mailbox, _a);
            node_mailboxes.PushBack(std::move(node_mailbox), _a);
        }

        _node.AddMember("mailboxes", std::move(node_mailboxes), _a);
    }

    int32_t mailbox_storage::load(const std::wstring& _filename)
    {
        core::tools::binary_stream bstream;
        if (!bstream.load_from_file(_filename))
            return -1;

        bstream.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse((const char*) bstream.read(bstream.available())).HasParseError())
            return -1;

        const auto iter_mailboxes = doc.FindMember("mailboxes");
        if (iter_mailboxes == doc.MemberEnd() || !iter_mailboxes->value.IsArray())
            return -1;

        for (const auto& mb : iter_mailboxes->value.GetArray())
        {
            mailbox m;
            m.unserialize(mb);

            if (const auto& mail = m.get_mailbox(); !mail.empty())
                mailboxes_[mail] = std::move(m);
        }

        return 0;
    }
}
}