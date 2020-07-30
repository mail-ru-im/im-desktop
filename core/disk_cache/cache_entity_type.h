#pragma once

#include "../namespaces.h"

CORE_DISK_CACHE_NS_BEGIN

enum class entity_type
{
    min,

    file,
    preview,
    json,

    max
};

std::ostream& operator<<(std::ostream &oss, const entity_type arg);

CORE_DISK_CACHE_NS_END