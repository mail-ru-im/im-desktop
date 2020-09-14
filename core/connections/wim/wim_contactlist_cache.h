#pragma once

#include "persons.h"
#include "lastseen.h"
#include "status.h"

namespace core
{
    class async_executer;
    struct icollection;

    namespace wim
    {
        class im;

        class contactlist;

        struct cl_presence
        {
            std::string usertype_;
            std::string chattype_;
            std::string status_msg_;
            std::string other_number_;
            std::string sms_number_;
            std::set<std::string> capabilities_;
            std::string ab_contact_name_;
            std::string friendly_;
            std::string nick_;

            lastseen lastseen_;
            status status_;

            int32_t outgoing_msg_count_ = 0;
            bool is_chat_ = false;
            bool muted_ = false;
            bool is_live_chat_ = false;
            bool official_ = false;
            bool public_ = false;
            bool is_channel_ = false;
            bool auto_added_ = false;
            bool deleted_ = false;

            std::string icon_id_;
            std::string big_icon_id_;
            std::string large_icon_id_;

            struct search_cache
            {
                std::string aimid_;
                std::string friendly_;
                std::string nick_;
                std::string ab_;
                std::string sms_number_;

                std::vector<std::string_view> friendly_words_;
                std::vector<std::string_view> ab_words_;

                bool is_empty() const noexcept { return aimid_.empty(); }
                void clear()
                {
                    aimid_.clear();
                    friendly_.clear();
                    nick_.clear();
                    ab_.clear();
                    sms_number_.clear();
                    friendly_words_.clear();
                    ab_words_.clear();
                }

            } search_cache_;

            void serialize(icollection* _coll);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void unserialize(const rapidjson::Value& _node);
            bool are_icons_equal(const cl_presence& _other) const;

            static bool are_icons_equal(const cl_presence& _lhs, const cl_presence& _rhs);
        };

        struct cl_buddy
        {
            uint32_t id_;
            std::string aimid_;
            std::shared_ptr<cl_presence> presence_;

            cl_buddy() : id_(0), presence_(std::make_shared<cl_presence>()) {}

            void prepare_search_cache();
        };

        using cl_buddy_ptr = std::shared_ptr<cl_buddy>;
        using cl_buddies_map = std::map<std::string, cl_buddy_ptr>;

        struct cl_group
        {
            uint32_t id_ = 0;
            std::string name_;

            std::vector<cl_buddy_ptr> buddies_;

            bool added_ = false;
            bool removed_ = false;
        };

        using ignorelist_cache = std::unordered_set<std::string>;

        struct cl_search_resut
        {
            std::string aimid_;
            std::set<std::string> highlights_;
        };

        struct cl_update_result
        {
            std::vector<std::string> update_avatars_;
        };

        using cl_search_resut_v = std::vector<cl_search_resut>;


        class contactlist
        {
        public:
            enum class changed_status
            {
                full,
                presence,
                none
            };

            explicit contactlist();

        private:
            changed_status changed_status_ = changed_status::none;
            bool need_update_search_cache_ = false;
            bool need_update_avatar_ = false;
            void set_need_update_cache(bool _need_update_search_cache);

            std::list<std::shared_ptr<cl_group>> groups_;

            ignorelist_cache ignorelist_;
            std::shared_ptr<core::archive::persons_map> persons_;

            void add_to_persons(const cl_buddy_ptr&);

            cl_buddies_map contacts_index_;
            cl_buddies_map trusted_contacts_;

            std::map<std::string, int32_t> search_priority_;
            cl_buddies_map search_cache_;
            cl_buddies_map tmp_cache_;

            std::string last_search_pattern_;
            cl_search_resut_v search_results_;

            void add_to_search_results(cl_buddies_map& _res_cache, const size_t _base_word_count, const cl_buddy_ptr& _buddy, std::set<std::string> _highlights, const int _priority);
            void search(const std::vector<std::vector<std::string>>& search_patterns, int32_t fixed_patterns_count);
            void search(const std::string_view search_pattern, bool first, int32_t searh_priority, int32_t fixed_patters_count);

        public:

            cl_update_result update_cl(const contactlist& _cl);

            void update_ignorelist(const ignorelist_cache& _ignorelist);

            void update_trusted(const std::string& _aimid, const std::string& _friendly, const std::string& _nick, const bool _is_trusted);

            void set_changed_status(changed_status _status) noexcept;
            changed_status get_changed_status() const noexcept;

            bool get_need_update_search_cache() const noexcept { return need_update_search_cache_; }
            bool get_need_update_avatar(bool reset) noexcept
            {
                const auto need = need_update_avatar_;
                if (reset)
                    need_update_avatar_ = false;
                return need;
            }

            bool exist(const std::string& contact) const { return contacts_index_.find(contact) != contacts_index_.end(); }

            std::string get_contact_friendly_name(const std::string& contact_login) const;

            int32_t unserialize(const rapidjson::Value& _node);
            int32_t unserialize_from_diff(const rapidjson::Value& _node);

            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            void serialize(icollection* _coll, const std::string& type) const;
            void serialize_search(icollection* _coll) const;
            void serialize_ignorelist(icollection* _coll) const;
            void serialize_contact(const std::string& _aimid, icollection* _coll) const;

            cl_search_resut_v search(const std::vector<std::vector<std::string>>& _search_patterns, unsigned _fixed_patterns_count, const std::string_view _pattern);

            int32_t get_search_priority(const std::string& _aimid) const;
            void set_last_search_pattern(const std::string_view _pattern);

            void update_presence(const std::string& _aimid, const std::shared_ptr<cl_presence>& _presence);
            void merge_from_diff(const std::string& _type, const std::shared_ptr<contactlist>& _diff, const std::shared_ptr<std::list<std::string>>& removedContacts);

            int32_t get_contacts_count() const;
            int32_t get_phone_contacts_count() const;
            int32_t get_groupchat_contacts_count() const;

            void add_to_ignorelist(const std::string& _aimid);
            void remove_from_ignorelist(const std::string& _aimid);

            bool contains(const std::string& _aimid) const;
            std::shared_ptr<cl_presence> get_presence(const std::string& _aimid) const;
            void set_outgoing_msg_count(const std::string& _contact, int32_t _count);
            void reset_outgoung_counters();

            std::shared_ptr<cl_group> get_first_group() const;
            bool is_ignored(const std::string& _aimid) const;

            const auto& get_persons() const { return persons_; }
            bool is_empty() const;

            std::vector<std::string> get_ignored_aimids() const;
            std::vector<std::string> get_aimids() const;
        };
    }
}
