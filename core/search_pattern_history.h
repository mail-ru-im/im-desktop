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
            contact_last_patterns(const std::wstring& _path);

            void save();
            void load();

            void add_pattern(std::string_view _pattern);
            void replace_last_pattern(std::string_view _pattern);
            void remove_pattern(std::string_view _pattern);
            const search_patterns& get_patterns() const;

        private:
            void resize_if_needed();

            bool save_needed_;
            std::wstring path_;

            search_patterns patterns_;
        };


        class search_pattern_history : public std::enable_shared_from_this<search_pattern_history>
        {
        public:
            search_pattern_history(const std::wstring& _path);

            void add_pattern(const std::string_view _pattern, const std::string_view _contact);
            void remove_pattern(const std::string_view _pattern, const std::string_view _contact);
            std::shared_ptr<get_patterns_handler> get_patterns(const std::string_view _contact);

            void save_all();

        private:
            std::wstring get_contact_path(const std::string_view _contact);
            contact_last_patterns& get_contact(const std::string_view _contact);
            std::shared_ptr<core::async_executer> get_thread();

            std::shared_ptr<core::async_executer> thread_;
            std::map<std::string, contact_last_patterns, tools::string_comparator> contact_patterns_;
            std::wstring root_path_;
        };
    }
}
