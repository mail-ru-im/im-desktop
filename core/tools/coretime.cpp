#include "stdafx.h"

#include "coretime.h"

namespace core
{
    namespace tools
    {
        namespace time
        {
            using namespace std::chrono;

            int64_t now_ms() noexcept
            {
                const auto now = time_point_cast<milliseconds>(high_resolution_clock::now());
                return now.time_since_epoch().count();
            }
        }
    }
}