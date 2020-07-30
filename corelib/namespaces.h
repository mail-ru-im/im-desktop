#pragma once

#ifndef CORE_NS
    #define CORE_NS core
    #define CORE_NS_BEGIN namespace CORE_NS {
    #define CORE_NS_END }
#endif

#ifndef COMMON_NS
    #define COMMON_NS common
    #define COMMON_NS_BEGIN namespace COMMON_NS {
    #define COMMON_NS_END }
#endif

#define COMMON_MEMORY_STATS_NS_BEGIN COMMON_NS_BEGIN namespace memory_stats {
#define COMMON_MEMORY_STATS_NS_END } COMMON_NS_END
