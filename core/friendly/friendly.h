#pragma once

#include "../connections/wim/persons.h"

namespace core
{
    enum class friendly_source
    {
        local,
        remote
    };

    enum class friendly_add_mode
    {
        insert_or_update,
        insert
    };

    class friendly
    {
    public:
        friendly_source type_ = friendly_source::local;
        std::string name_;
        std::string nick_;
        std::optional<bool> official_;
    };

    class friendly_container
    {
    public:
        friendly_container() noexcept;

        using container = std::unordered_map<std::string, friendly>;

        struct uin_friendly
        {
            std::string uin_;
            friendly friendly_;
        };

        using friendly_updates = std::vector<uin_friendly>;
        friendly_updates insert_friendly(const std::string& _uin, const std::string_view _name, const std::string_view _nick, std::optional<bool> _official, const friendly_source _type, const friendly_add_mode _mode);
        friendly_updates insert_friendly(const std::string& _uin, const friendly& _friendly, const friendly_add_mode _mode)
        {
            return insert_friendly(_uin, _friendly.name_, _friendly.nick_, _friendly.official_, _friendly.type_, _mode);
        }
        friendly_updates insert_friendly(const core::archive::persons_map& _map, const friendly_source _type, const friendly_add_mode _mode);

        std::optional<friendly> get_friendly(const std::string& _uin) const;
        std::string get_nick(const std::string& _uin) const;

    private:
        container container_;
        std::thread::id thread_id_;
    };
}