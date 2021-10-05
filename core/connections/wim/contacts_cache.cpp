#include "stdafx.h"

#include "contacts_cache.h"

#include "../../../corelib/collection_helper.h"
#include "../../../common.shared/json_helper.h"

using namespace core;
using namespace wim;

cached_contact::cached_contact(std::string_view _aimid, std::optional<friendly> _friendly, std::optional<int64_t> _time)
    : aimid_(_aimid)
    , friendly_(std::move(_friendly))
    , time_(std::move(_time))
{
}

void cached_contact::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("aimId",  get_aimid(), _a);
    if (time_)
        _node.AddMember("time", *time_, _a);

    if (friendly_ && !friendly_->name_.empty())
    {
        _node.AddMember("friendly", friendly_->name_, _a);
        if (auto official = friendly_->official_.value_or(false))
            _node.AddMember("official", official, _a);
    }
}

int32_t cached_contact::unserialize(const rapidjson::Value& _node)
{
    if (!tools::unserialize_value(_node, "aimId", aimid_))
        return -1;

    time_ = common::json::get_value<int64_t>(_node, "time");

    if (std::string_view name; tools::unserialize_value(_node, "friendly", name) && !name.empty())
    {
        friendly f;
        f.name_ = std::move(name);
        if (bool official = false; tools::unserialize_value(_node, "official", official) && official)
            f.official_ = official;

        friendly_ = std::make_optional(std::move(f));
    }
    return 0;
}


contacts_cache::contacts_cache(std::string_view _name)
    : name_(_name)
{
}

void contacts_cache::set_contacts(std::vector<cached_contact> _contacts)
{
    contacts_ = std::move(_contacts);
    changed_ = true;
}

void contacts_cache::update(cached_contact _contact)
{
    const auto it = std::find_if(contacts_.begin(), contacts_.end(), [&_contact](const auto& cc){ return cc.get_aimid() == _contact.get_aimid(); });
    if (it != contacts_.end())
        *it = std::move(_contact);
    else
        contacts_.push_back(std::move(_contact));

    changed_ = true;
}

void contacts_cache::remove(std::string_view _aimid)
{
    const auto it = std::find_if(contacts_.begin(), contacts_.end(), [_aimid](const auto& cc){ return cc.get_aimid() == _aimid; });
    if (it != contacts_.end())
    {
        contacts_.erase(it);
        changed_ = true;
    }
}

void contacts_cache::clear()
{
    set_contacts({});
}

bool contacts_cache::contains(std::string_view _aimid) const
{
    return std::any_of(contacts_.begin(), contacts_.end(), [_aimid](const auto& cc) { return cc.get_aimid() == _aimid; });
}

std::optional<int64_t> contacts_cache::get_time(std::string_view _aimid) const
{
    const auto it = std::find_if(contacts_.begin(), contacts_.end(), [_aimid](const auto& cc) { return cc.get_aimid() == _aimid; });
    if (it != contacts_.end())
        return it->get_time();

    return {};
}

void contacts_cache::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    if (name_.empty())
        return;

    rapidjson::Value node_contacts(rapidjson::Type::kArrayType);
    node_contacts.Reserve(contacts_.size(), _a);
    for (const auto& contact : contacts_)
    {
        rapidjson::Value node_contact(rapidjson::Type::kObjectType);
        contact.serialize(node_contact, _a);
        node_contacts.PushBack(std::move(node_contact), _a);
    }

    _node.AddMember(tools::make_string_ref(name_), std::move(node_contacts), _a);
}

archive::persons_map contacts_cache::get_persons() const
{
    archive::persons_map persons;
    for (const auto& contact : contacts_)
    {
        if (const auto& friendly = contact.get_friendly())
        {
            core::archive::person p;
            p.friendly_ = friendly->name_;
            p.nick_ = friendly->nick_;
            p.official_ = friendly->official_;
            persons.emplace(contact.get_aimid(), std::move(p));
        }
    }
    return persons;
}


int32_t contacts_cache::unserialize(const rapidjson::Value& _node)
{
    if (name_.empty())
        return -1;

    const auto iter_contacts = _node.FindMember(name_);
    if (iter_contacts == _node.MemberEnd() || !iter_contacts->value.IsArray())
        return -1;

    contacts_.reserve(iter_contacts->value.Size());
    for (const auto& contact : iter_contacts->value.GetArray())
    {
        cached_contact c;
        if (c.unserialize(contact) != 0)
            return -1;

        contacts_.push_back(std::move(c));
    }

    return 0;
}

bool contacts_multicache::has_cache(std::string_view _cache_name) const
{
    return std::any_of(caches_.begin(), caches_.end(), [_cache_name](const auto& c) { return c.get_name() == _cache_name; });
}

void contacts_multicache::set_contacts(std::string_view _cache_name, std::vector<cached_contact> _contacts)
{
    const auto it = std::find_if(caches_.begin(), caches_.end(), [_cache_name](const auto& c) { return c.get_name() == _cache_name; });
    if (it != caches_.end())
    {
        it->set_contacts(std::move(_contacts));
    }
    else
    {
        contacts_cache cc(_cache_name);
        cc.set_contacts(std::move(_contacts));
        caches_.emplace_back(std::move(cc));
    }
}

const std::vector<cached_contact>& core::wim::contacts_multicache::get_contacts(std::string_view _cache_name) const
{
    im_assert(has_cache(_cache_name));
    const auto it = std::find_if(caches_.begin(), caches_.end(), [_cache_name](const auto& c) { return c.get_name() == _cache_name; });
    if (it != caches_.end())
        return it->contacts();

    static const std::vector<cached_contact> empty;
    return empty;
}

void contacts_multicache::remove(std::string_view _aimid, std::string_view _cache_name)
{
    const auto it = std::find_if(caches_.begin(), caches_.end(), [_cache_name](const auto& c) { return c.get_name() == _cache_name; });
    if (it != caches_.end())
        it->remove(_aimid);
}

bool contacts_multicache::is_changed() const noexcept
{
    return std::any_of(caches_.begin(), caches_.end(), [](const auto& c) { return c.is_changed(); });
}

void contacts_multicache::set_changed(bool _changed)
{
    for (auto& c : caches_)
        c.set_changed(_changed);
}

void contacts_multicache::clear()
{
    caches_.clear();
}

int32_t contacts_multicache::unserialize(const rapidjson::Value& _node)
{
    const auto iter_caches = _node.FindMember("caches");
    if (iter_caches == _node.MemberEnd() || !iter_caches->value.IsArray())
        return -1;

    caches_.reserve(iter_caches->value.Size());
    for (const auto& cache_node : iter_caches->value.GetArray())
    {
        if (!cache_node.IsObject())
            continue;

        contacts_cache c(rapidjson_get_string_view(cache_node.MemberBegin()->name));
        if (c.unserialize(cache_node) != 0)
            return -1;

        caches_.push_back(std::move(c));
    }
    return 0;
}

void core::wim::contacts_multicache::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    rapidjson::Value node_caches(rapidjson::Type::kArrayType);
    node_caches.Reserve(caches_.size(), _a);

    for (const auto& c : caches_)
    {
        rapidjson::Value obj(rapidjson::Type::kObjectType);
        c.serialize(obj, _a);
        node_caches.PushBack(std::move(obj), _a);
    }

    _node.AddMember("caches", std::move(node_caches), _a);
}

archive::persons_map contacts_multicache::get_persons() const
{
    archive::persons_map persons;
    for (const auto& c : caches_)
        persons.merge(c.get_persons());
    return persons;
}
