#pragma once

#include "../../../corelib/collection_helper.h"
#include "../../archive/storage.h"

namespace core
{
    namespace wim
    {
        struct mailbox_change
        {
            enum class type
            {
                status = 0,
                new_mail = 1,
                mail_read = 2,
            } type_;
            std::string email_;

            mailbox_change(type _type)
                : type_(_type)
            {
            }

            virtual ~mailbox_change() = default;

            virtual unsigned int get_unreads_count() const { return 0; }
        };

        struct mailbox_status_change : public mailbox_change
        {
            mailbox_status_change()
                : mailbox_change(mailbox_change::type::status)
            {
            }

            unsigned int unreads_ = 0;
            unsigned int get_unreads_count() const override { return unreads_; }
        };

        struct mailbox_read_change : public mailbox_change
        {
            mailbox_read_change()
                : mailbox_change(mailbox_change::type::mail_read)
            {
            }

            std::string uidl_;
        };

        struct mailbox_new_mail_change : public mailbox_change
        {
            mailbox_new_mail_change()
                : mailbox_change(mailbox_change::type::new_mail)
            {
            }

            unsigned int unreads_ = 0;
            std::string uidl_;
            std::string from_;
            std::string subject_;

            unsigned int get_unreads_count() const override { return unreads_; }
        };

        using mailbox_changes = std::vector<std::shared_ptr<mailbox_change>>;

        class mailbox
        {
            std::string email_;
            unsigned int unread_count_ = 0;

        public:
            mailbox() = default;
            mailbox(const std::string& _email, unsigned int _unreads);

            void set_unreads(unsigned _unreads);
            unsigned get_unreads() const;
            const std::string& get_mailbox() const;

            void unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _collection) const;
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
        };

        class mailbox_storage
        {
            bool is_changed_ = false;
            std::map<std::string, mailbox> mailboxes_;

        public:
            mailbox_storage() = default;

            void process(const mailbox_changes& _changes, std::function<void(core::coll_helper, mailbox_change::type)> _notify_callback);

            static bool unserialize(const rapidjson::Value& _node_event_data, mailbox_changes& _changes);

            void serialize(core::coll_helper _collection) const;
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

            bool is_changed() const noexcept { return is_changed_; }
            void set_changed(bool _changed) { is_changed_ = _changed; }

            int32_t load(const std::wstring& _filename);
        };
    }
}