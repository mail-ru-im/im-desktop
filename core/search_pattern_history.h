#pragma once

#include "async_task.h"
#include "tools/string_comparator.h"

namespace core
{
    namespace search
    {
        using search_patterns = std::list<std::string>;

        struct get_patterns_handler
        {
            using on_result_type = std::function<void(const search_patterns& _patterns)>;

            on_result_type on_result = [](const search_patterns& _patterns) {};
        };

        class contact_last_patterns
        {
        public:
            contact_last_patterns(std::wstring _path);

            void save();
            void load();

            void add_pattern(std::string_view _pattern);
            void remove_pattern(std::string_view _pattern);
            const search_patterns& get_patterns() const noexcept { return patterns_; }

            bool save_needed() const noexcept { return save_needed_; }

        private:
            void resize_if_needed();

            bool save_needed_ = false;
            std::wstring path_;

            search_patterns patterns_;
        };


        class search_pattern_history : public std::enable_shared_from_this<search_pattern_history>
        {
        public:
            search_pattern_history(std::wstring _path);

            void add_pattern(std::string_view _pattern, std::string_view _contact, const std::shared_ptr<core::async_executer>& _executer);
            void remove_pattern(std::string_view _pattern, std::string_view _contact, const std::shared_ptr<core::async_executer>& _executer);
            std::shared_ptr<get_patterns_handler> get_patterns(std::string_view _contact, const std::shared_ptr<core::async_executer>& _executer);

            void free_dialog(std::string_view _contact, const std::shared_ptr<core::async_executer>& _executer);

            void save_all(const std::shared_ptr<core::async_executer>& _executer);

        private:
            std::wstring get_contact_path(std::string_view _contact) const;
            contact_last_patterns& get_contact(std::string_view _contact);

        private:
            std::map<std::string, contact_last_patterns, tools::string_comparator> contact_patterns_;
            std::wstring root_path_;
        };
    }
}
