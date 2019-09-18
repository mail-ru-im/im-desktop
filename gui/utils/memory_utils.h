#pragma once

#include <QtGlobal>

namespace Utils
{

template <typename Type>
qint64 getMemoryFootprint(Type _object);

// implementation duplicates method in core
uint64_t getCurrentProcessRamUsage(); // phys_footprint on macos, "Mem" in Activity Monitor
uint64_t getCurrentProcessRealMemUsage(); // resident size on macos, "Real Mem" in Activity Monitor

}
