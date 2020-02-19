#include "stdafx.h"
#include "mailboxes.h"
#include "../../core.h"
#include "../../tools/json_helper.h"

namespace core
{
namespace wim
{
    mailbox::mailbox()
        : unreadCount_(0)
    {
    }

    mailbox::mailbox(const std::string& _email)
        : email_(_email)
        , unreadCount_(0)
    {
    }

    void mailbox::setUnreads(unsigned _unreads)
    {
        unreadCount_ = _unreads;
    }

    unsigned mailbox::get_unreads() const
    {
        return unreadCount_;
    }

    const std::string& mailbox::get_mailbox() const
    {
        return email_;
    }

    void mailbox::unserialize(const rapidjson::Value& _node)
    {
        if (!tools::unserialize_value(_node, "email", email_))
            return;

        tools::unserialize_value(_node, "unreadCount", unreadCount_);
    }

    void mailbox::serialize(core::coll_helper _collection) const
    {
        _collection.set_value_as_string("email", email_);
        _collection.set_value_as_uint("unreads", unreadCount_);
    }

    void mailbox::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        _node.AddMember("email", get_mailbox(), _a);
        _node.AddMember("unreadCount", get_unreads(), _a);
    }

    mailbox_storage::mailbox_storage()
        : is_changed_(false)
    {

    }

    void mailbox_storage::process(const mailbox_changes& _changes, std::function<void(core::coll_helper, mailbox_change::type)> _notify_callback)
    {
        is_changed_ = true;
        bool init = false;

        for (const auto& change : _changes)
        {
            mailbox m(change->email_);
            coll_helper coll_mailbox(g_core->create_collection(), true);

            switch (change->type_)
            {
            case mailbox_change::status:
                init = mailboxes_.find(change->email_) == mailboxes_.end();
                m.setUnreads(((mailbox_status_change*)change.get())->unreads_);
                mailboxes_[change->email_] = std::move(m);

                serialize(coll_mailbox);
                coll_mailbox.set_value_as_bool("init", init);
                _notify_callback(coll_mailbox, change->type_);
                break;

            case mailbox_change::new_mail:
                m.setUnreads(((mailbox_new_mail_change*)change.get())->unreads_);
                mailboxes_[change->email_] = std::move(m);

                coll_mailbox.set_value_as_string("email", change->email_);
                coll_mailbox.set_value_as_string("from", ((mailbox_new_mail_change*)change.get())->from_);
                coll_mailbox.set_value_as_string("subj", ((mailbox_new_mail_change*)change.get())->subject_);
                coll_mailbox.set_value_as_string("uidl", ((mailbox_new_mail_change*)change.get())->uidl_);

                _notify_callback(coll_mailbox, change->type_);

                serialize(coll_mailbox);
                _notify_callback(coll_mailbox, mailbox_change::status);
                break;

            case mailbox_change::mail_read:
            default:
                break;
            }
        }
    }

    bool mailbox_storage::unserialize(const rapidjson::Value& _node_event_data, mailbox_changes& _changes)
    {
        const auto initial_size = _changes.size();

        auto event_status = _node_event_data.FindMember("mailbox.status");
        if (event_status != _node_event_data.MemberEnd() && event_status->value.IsString())
        {
            rapidjson::Document doc;
            doc.Parse(event_status->value.GetString());

            auto change = std::make_shared<mailbox_status_change>();

            auto iter_email = doc.FindMember("email");
            if (iter_email != doc.MemberEnd() && iter_email->value.IsString())
                change->email_ = rapidjson_get_string(iter_email->value);

            auto iter_unreads = doc.FindMember("unreadCount");
            if (iter_unreads != doc.MemberEnd() && iter_unreads->value.IsUint())
                change->unreads_ = iter_unreads->value.GetUint();

            _changes.push_back(std::move(change));
        }

        auto event_read = _node_event_data.FindMember("mailbox.messageReaded");
        if (event_read != _node_event_data.MemberEnd() && event_read->value.IsString())
        {
            rapidjson::Document doc;
            doc.Parse(event_read->value.GetString());

            auto change = std::make_shared<mailbox_read_change>();

            auto iter_email = doc.FindMember("email");
            if (iter_email != doc.MemberEnd() && iter_email->value.IsString())
                change->email_ = rapidjson_get_string(iter_email->value);

            auto iter_id = doc.FindMember("uidl");
            if (iter_id != doc.MemberEnd() && iter_id->value.IsString())
                change->uidl_ = rapidjson_get_string(iter_id->value);

            _changes.push_back(std::move(change));
        }

        auto event_new = _node_event_data.FindMember("mailbox.newMessage");
        if (event_new != _node_event_data.MemberEnd() && event_new->value.IsString())
        {
            rapidjson::Document doc;
            doc.Parse(event_new->value.GetString());

            auto change = std::make_shared<mailbox_new_mail_change>();

            auto iter_email = doc.FindMember("email");
            if (iter_email != doc.MemberEnd() && iter_email->value.IsString())
                change->email_ = rapidjson_get_string(iter_email->value);

            auto iter_unreads = doc.FindMember("unreadCount");
            if (iter_unreads != doc.MemberEnd() && iter_unreads->value.IsUint())
                change->unreads_ = iter_unreads->value.GetUint();

            auto iter_id = doc.FindMember("uidl");
            if (iter_id != doc.MemberEnd() && iter_id->value.IsString())
                change->uidl_ = rapidjson_get_string(iter_id->value);

            auto iter_from = doc.FindMember("from");
            if (iter_from != doc.MemberEnd() && iter_from->value.IsString())
                change->from_ = rapidjson_get_string(iter_from->value);

            auto iter_subject = doc.FindMember("subject");
            if (iter_subject != doc.MemberEnd() && iter_subject->value.IsString())
                change->subject_ = rapidjson_get_string(iter_subject->value);

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

    bool mailbox_storage::is_changed() const
    {
        return is_changed_;
    }

    int32_t mailbox_storage::save(const std::wstring& _filename)
    {
        if (!is_changed())
            return 0;

        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& allocator = doc.GetAllocator();
        rapidjson::Value node_mailboxes(rapidjson::Type::kArrayType);
        node_mailboxes.Reserve(mailboxes_.size(), allocator);
        for (const auto& mailbox : mailboxes_)
        {
            rapidjson::Value node_mailbox(rapidjson::Type::kObjectType);
            mailbox.second.serialize(node_mailbox, allocator);
            node_mailboxes.PushBack(std::move(node_mailbox), allocator);
        }

        doc.AddMember("mailboxes", std::move(node_mailboxes), allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        auto json_string = rapidjson_get_string_view(buffer);

        if (!json_string.length())
        {
            return - 1;
        }

        core::tools::binary_stream bstream;
        bstream.write<std::string_view>(json_string);
        if (!bstream.save_2_file(_filename))
            return -1;

        is_changed_ = false;

        return 0;
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

            auto mail = m.get_mailbox();
            mailboxes_[std::move(mail)] = std::move(m);
        }

        return 0;
    }
}
}