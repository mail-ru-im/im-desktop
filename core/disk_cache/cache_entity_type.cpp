#include "stdafx.h"

#include "cache_entity_type.h"

CORE_DISK_CACHE_NS_BEGIN

std::ostream& operator<<(std::ostream &oss, const entity_type arg)
{
    assert(arg > entity_type::min);
    assert(arg < entity_type::max);

    switch(arg)
    {
        case entity_type::file: oss << "file"; break;

        case entity_type::json: oss << "json"; break;

        case entity_type::preview: oss << "preview"; break;

        default: assert(!"unexpected entity type"); break;
    }

    return oss;
}

CORE_DISK_CACHE_NS_END