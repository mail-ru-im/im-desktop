#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include "namespaces.h"

CORE_MEMORY_STATS_NS_BEGIN

enum class stat_type
{
    voip_initialization,
    cached_avatars,
    cached_themes,
    cached_previews,
    cached_emojis,
    cached_stickers,
    video_player_initialization,
    invalid
};

using memory_size_t = int64_t;
using name_string_t = std::string;
using request_id = int64_t;
using requested_types = std::unordered_set<stat_type>;
constexpr request_id INVALID_REQUEST_ID = -1;
constexpr memory_size_t INVALID_SIZE = -1LL;

CORE_MEMORY_STATS_NS_END
