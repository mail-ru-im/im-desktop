#include "stdafx.h"

#include "history_patch.h"

CORE_ARCHIVE_NS_BEGIN

    history_patch::history_patch(const type _type, const int64_t _archive_id)
    : type_(_type)
    , archive_id_(_archive_id)
{
    assert(is_valid_type(type_));
    assert(archive_id_ > 0);
}

int64_t history_patch::get_message_id() const
{
    assert(archive_id_ > 0);

    return archive_id_;
}

history_patch::type history_patch::get_type() const
{
    assert(is_valid_type(type_));

    return type_;
}

bool history_patch::is_valid_type(const type _type)
{
    switch (_type)
    {
    case type::deleted:
    case type::modified:
    case type::pinned:
    case type::unpinned:
    case type::updated:
    case type::clear:
        return true;

    default:
        break;
    }

    return false;
}

std::string history_patch::type_to_string(const type _type)
{
    switch (_type)
    {
    case type::deleted:
        return "deleted";
    case type::modified:
        return "modified";
    case type::pinned:
        return "pinned";
    case type::unpinned:
        return "unpinned";
    case type::updated:
        return "updated";

    default:
        break;
    }

    return "unknown type";
}

history_patch_uptr history_patch::make(const type _type, const int64_t _archive_id)
{
    assert(is_valid_type(_type));
    assert(_archive_id > 0);

    return history_patch_uptr(new history_patch(_type, _archive_id));
}

CORE_ARCHIVE_NS_END