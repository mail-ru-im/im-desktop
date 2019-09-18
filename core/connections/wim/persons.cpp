#include "stdafx.h"

#include "../../log/log.h"

#include "persons.h"

#include "../../../corelib/core_face.h"
#include "../../../corelib/collection_helper.h"
#include "../../tools/json_helper.h"

using namespace core;

archive::persons_map core::wim::parse_persons(const rapidjson::Value& _node)
{
    archive::persons_map persons;
    if (const auto iter_persons = _node.FindMember("persons"); iter_persons != _node.MemberEnd() && iter_persons->value.IsArray())
    {
        for (const auto& person : iter_persons->value.GetArray())
        {
            archive::person p;
            std::string sn;

            if (!tools::unserialize_value(person, "sn", sn) || !tools::unserialize_value(person, "friendly", p.friendly_))
                continue;

            tools::unserialize_value(person, "nick", p.nick_);

            p.official_ = is_official(person);

            boost::replace_last(sn, "@uin.icq", std::string());

            persons.emplace(std::move(sn), std::move(p));
        }
    }
    return persons;
}

bool core::wim::is_official(const rapidjson::Value& _node)
{
    if (bool official = false; tools::unserialize_value(_node, "official", official))
        return official;

    if (const auto iter_honours = _node.FindMember("honours"); iter_honours != _node.MemberEnd() && iter_honours->value.IsArray())
    {
        for (const auto& x : iter_honours->value.GetArray())
        {
            if (x.IsString() && (rapidjson_get_string_view(x) == "official"))
                return true;
        }
    }
    return false;
}

void core::wim::serialize_persons(const archive::persons_map& _persons, icollection* _collection)
{
    coll_helper coll(_collection, false);

    for (const auto& [sn, person] : _persons)
        coll.set_value_as_string(sn, person.friendly_);
}
