#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include "namespaces.h"

MEMSTATS_NS_BEGIN

enum class StatType
{
    VoipInitialization,
    CachedAvatars,
    CachedThemes,
    CachedPreviews,
    CachedEmojis,
    CachedStickers,
    VideoPlayerInitialization,
    Invalid
};

using MemorySize_t = int64_t;
using NameString_t = std::string;
using RequestId = int64_t;
using RequestedTypes = std::unordered_set<StatType>;
const static RequestId INVALID_REQUEST_ID = -1;
constexpr MemorySize_t INVALID_SIZE = -1LL;

MEMSTATS_NS_END
