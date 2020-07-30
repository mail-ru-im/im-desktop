#include "stdafx.h"
#include "active_dialogs.h"

#include "../../../corelib/collection_helper.h"
#include "../../tools/json_helper.h"

using namespace core;
using namespace wim;

void active_dialog::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("aimId", get_aimid(), _a);
}

int32_t active_dialog::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "aimId", aimid_);
    return 0;
}

void active_dialog::serialize(icollection* _coll) const
{
    coll_helper cl(_coll, false);
    cl.set_value_as_string("aimId", aimid_);
}





active_dialogs::active_dialogs()
    : changed_(false)
{
}


active_dialogs::~active_dialogs()
{
}

void active_dialogs::update(active_dialog _dialog)
{
    changed_ = true;

    for (auto iter = dialogs_.begin(); iter != dialogs_.end(); ++iter)
    {
        if (iter->get_aimid() == _dialog.get_aimid())
        {
            dialogs_.erase(iter);

            break;
        }
    }

    dialogs_.push_front(std::move(_dialog));
}

void active_dialogs::remove(const std::string& _aimid)
{
    for (auto iter = dialogs_.begin(); iter != dialogs_.end(); ++iter)
    {
        if (iter->get_aimid() == _aimid)
        {
            dialogs_.erase(iter);

            changed_ = true;

            break;
        }
    }
}

bool active_dialogs::contains(const std::string& _aimId) const
{
    return std::any_of(dialogs_.cbegin(), dialogs_.cend(), [&_aimId](const auto& x) { return x.get_aimid() == _aimId; });
}

void active_dialogs::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    rapidjson::Value node_dialogs(rapidjson::Type::kArrayType);
    node_dialogs.Reserve(dialogs_.size(), _a);
    for (const auto& dialog : dialogs_)
    {
        rapidjson::Value node_dialog(rapidjson::Type::kObjectType);

        dialog.serialize(node_dialog, _a);

        node_dialogs.PushBack(std::move(node_dialog), _a);
    }

    _node.AddMember("dialogs", std::move(node_dialogs), _a);
}


int32_t active_dialogs::unserialize(const rapidjson::Value& _node)
{
    auto iter_dialogs = _node.FindMember("dialogs");
    if (iter_dialogs == _node.MemberEnd() || !iter_dialogs->value.IsArray())
        return -1;

    for (const auto& dialog : iter_dialogs->value.GetArray())
    {
        active_dialog dlg;
        if (dlg.unserialize(dialog) != 0)
            return -1;

        dialogs_.push_back(std::move(dlg));
    }

    return 0;
}

void active_dialogs::serialize(icollection* _coll) const
{
    coll_helper cl(_coll, false);

    ifptr<iarray> dialogs_array(_coll->create_array());
    dialogs_array->reserve((uint32_t)dialogs_.size());

    for (const auto& iter : dialogs_)
    {
        coll_helper dlg_coll(_coll->create_collection(), true);
        iter.serialize(dlg_coll.get());

        ifptr<ivalue> val_dlg(_coll->create_value());
        val_dlg->set_as_collection(dlg_coll.get());
        dialogs_array->push_back(val_dlg.get());
    }

    cl.set_value_as_array("dialogs", dialogs_array.get());
}

size_t active_dialogs::size() const noexcept
{
    return dialogs_.size();
}

void active_dialogs::enumerate(const std::function<void(const active_dialog&)>& _callback)
{
    for (const auto& _dialog : boost::adaptors::reverse(dialogs_))
    {
        _callback(_dialog);
    }
}
