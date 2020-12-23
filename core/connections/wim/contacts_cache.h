#pragma once

#include "friendly/friendly.h"
#include "persons.h"

namespace core
{
    struct icollection;

    namespace wim
    {
        class cached_contact
        {
        public:
            cached_contact() = default;
            cached_contact(std::string_view _aimid, std::optional<friendly> _friendly = {}, std::optional<int64_t> _time = {});

            const std::string& get_aimid() const noexcept { return aimid_; }
            const std::optional<friendly>& get_friendly() const noexcept { return friendly_; }
            const std::optional<int64_t>& get_time() const noexcept { return time_; }

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

        private:
            std::string	aimid_;
            std::optional<friendly> friendly_;
            std::optional<int64_t> time_;
        };

        class contacts_cache
        {
        public:
            contacts_cache(std::string_view _name = {});

            size_t size() const noexcept { return contacts_.size(); }

            bool is_changed() const noexcept { return changed_; }
            void set_changed(bool _changed) { changed_ = _changed; }

            const std::vector<cached_contact>& contacts() const { return contacts_; }
            const std::string& get_name() const { return name_; }

            void set_contacts(std::vector<cached_contact> _contacts);
            void update(cached_contact _contact);
            void remove(std::string_view _aimid);
            void clear();

            bool contains(std::string_view _aimid) const;
            std::optional<int64_t> get_time(std::string_view _aimid) const;

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

            archive::persons_map get_persons() const;

        private:
            std::vector<cached_contact> contacts_;
            std::string name_;
            bool changed_ = false;
        };


        class contacts_multicache
        {
        public:
            contacts_multicache() = default;

            bool has_cache(std::string_view _cache_name) const;
            void set_contacts(std::string_view _cache_name, std::vector<cached_contact> _contacts);
            const std::vector<cached_contact>& get_contacts(std::string_view _cache_name) const;

            void remove(std::string_view _aimid, std::string_view _cache_name);

            bool is_changed() const noexcept;
            void set_changed(bool _changed);

            void clear();

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

            archive::persons_map get_persons() const;

        private:
            std::vector<contacts_cache> caches_;
        };
    }
}
