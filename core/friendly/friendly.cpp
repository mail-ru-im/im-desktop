#include "stdafx.h"

#include "friendly.h"

namespace core
{
    friendly_container::friendly_container() noexcept
    {
        thread_id_ = std::this_thread::get_id();
    }

    friendly_container::friendly_updates friendly_container::insert_friendly(const std::string& _uin, const std::string_view _name, const std::string_view _nick, std::optional<bool> _official, const friendly_source _type, const friendly_add_mode _mode)
    {
        assert(std::this_thread::get_id() == thread_id_);
        auto& friendly = container_[_uin];

        const auto is_different = [&friendly, &_name, &_nick, &_official]()
        {
            return friendly.name_ != _name || friendly.nick_ != _nick || (_official && friendly.official_ != _official);
        };

        if (_type == friendly_source::remote)
        {
            if (friendly.type_ != _type)
                friendly.type_ = _type;

            if (is_different())
            {
                friendly.name_ = _name;
                friendly.nick_ = _nick;
                if (_official)
                    friendly.official_ = _official;

                return { { _uin , friendly } };
            }
        }
        else if ((_mode == friendly_add_mode::insert_or_update && is_different()) || (_mode == friendly_add_mode::insert && friendly.name_.empty()))
        {
            friendly.name_ = _name;
            friendly.nick_ = _nick;
            if (_official)
                friendly.official_ = _official;

            return { { _uin , friendly } };
        }
        return {};
    }

    friendly_container::friendly_updates friendly_container::insert_friendly(const core::archive::persons_map& _map, const friendly_source _type, const friendly_add_mode _mode)
    {
        assert(std::this_thread::get_id() == thread_id_);
        friendly_container::friendly_updates result;
        for (const auto& [uin, person] : _map)
        {
            auto r = insert_friendly(uin, person.friendly_, person.nick_, person.official_, _type, _mode);
            result.insert(result.end(), std::make_move_iterator(r.begin()), std::make_move_iterator(r.end()));
        }
        return result;
    }

    std::optional<friendly> friendly_container::get_friendly(const std::string& _uin) const
    {
        assert(std::this_thread::get_id() == thread_id_);
        if (const auto it = container_.find(_uin); it != container_.end())
            return it->second;
        return std::nullopt;
    }

    std::string friendly_container::get_nick(const std::string& _uin) const
    {
        assert(std::this_thread::get_id() == thread_id_);
        if (auto res = get_friendly(_uin); res)
            return (*res).nick_;
        return {};
    }
}
