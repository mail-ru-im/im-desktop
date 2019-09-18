#pragma once

#include "namespaces.h"

PLATFORM_NS_BEGIN

enum class OsxVersion
{
    UNKNOWN = 0x0000,

    MV_9    = 0x0001,
    MV_10_0 = 0x0002,
    MV_10_1 = 0x0003,
    MV_10_2 = 0x0004,
    MV_10_3 = 0x0005,
    MV_10_4 = 0x0006,
    MV_10_5 = 0x0007,
    MV_10_6 = 0x0008,
    MV_10_7 = 0x0009,
    MV_10_8 = 0x000A,
    MV_10_9 = 0x000B,
    MV_10_10 = 0x000C,
    MV_10_11 = 0x000D,
    MV_10_12 = 0x000E,
};

OsxVersion osxVersion();

PLATFORM_NS_END