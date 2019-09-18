#include "stdafx.h"
#include "favorites.h"

#include "../../../corelib/collection_helper.h"
#include "../../tools/json_helper.h"

using namespace core;
using namespace wim;

favorite::favorite() = default;

favorite::favorite(const std::string& _aimid, friendly _friendly, const int64_t _time)
    : aimid_(_aimid)
    , time_(_time)
    , friendly_(std::move(_friendly))
{
}


void favorite::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("aimId",  get_aimid(), _a);
    _node.AddMember("time", time_, _a);
    if (!friendly_.name_.empty())
    {
        _node.AddMember("friendly", friendly_.name_, _a);
        if (friendly_.official_)
            _node.AddMember("official", *(friendly_.official_), _a);
    }

}

int32_t favorite::unserialize(const rapidjson::Value& _node)
{
    if (!tools::unserialize_value(_node, "aimId", aimid_))
        return -1;

    if (!tools::unserialize_value(_node, "time", time_))
        time_ = 0;

    tools::unserialize_value(_node, "friendly", friendly_.name_);
    if (bool official = false; tools::unserialize_value(_node, "official", official))
        friendly_.official_ = official;

    return 0;
}


favorites::favorites() = default;

favorites::~favorites() = default;

void favorites::update(favorite& _contact)
{
    changed_ = true;

    for (auto iter = contacts_.begin(); iter != contacts_.end(); ++iter)
    {
        if (iter->get_aimid() == _contact.get_aimid())
        {
            contacts_.erase(iter);

            break;
        }
    }

    contacts_.push_back(_contact);
    index_[_contact.get_aimid()] = _contact.get_time();
}

void favorites::remove(const std::string_view _aimid)
{
    for (auto iter = contacts_.begin(); iter != contacts_.end(); ++iter)
    {
        if (iter->get_aimid() == _aimid)
        {
            contacts_.erase(iter);

            changed_ = true;

            break;
        }
    }

    if (auto iter = index_.find(std::string(_aimid)); iter != index_.end())
        index_.erase(iter);
}

bool favorites::contains(const std::string_view _aimid) const
{
    return index_.find(std::string(_aimid)) != index_.end();
}

int64_t favorites::get_time(const std::string_view _aimid) const
{
    auto iter = index_.find(std::string(_aimid));
    if (iter != index_.end())
        return iter->second;

    return -1;
}

void favorites::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
    rapidjson::Value node_contacts(rapidjson::Type::kArrayType);
    node_contacts.Reserve(contacts_.size(), _a);
    for (const auto& iter : contacts_)
    {
        rapidjson::Value node_contact(rapidjson::Type::kObjectType);

        iter.serialize(node_contact, _a);

        node_contacts.PushBack(std::move(node_contact), _a);
    }

    _node.AddMember("favorites", std::move(node_contacts), _a);
}


int32_t favorites::unserialize(const rapidjson::Value& _node)
{
    const auto iter_contacts = _node.FindMember("favorites");
    if (iter_contacts == _node.MemberEnd() || !iter_contacts->value.IsArray())
        return -1;

    for (const auto& contact : iter_contacts->value.GetArray())
    {
        favorite dlg;
        if (dlg.unserialize(contact) != 0)
            return -1;

        index_.insert(std::make_pair(dlg.get_aimid(), dlg.get_time()));
        contacts_.push_back(std::move(dlg));
    }

    return 0;
}
