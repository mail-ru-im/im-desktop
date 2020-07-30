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

            bool localtime(const time_t* timer, struct tm* buf)
            {
#if defined(_WIN32)
                return localtime_s(buf, timer) == 0;
#else
                return localtime_r(timer, buf) != nullptr;
#endif
            }

            bool gmtime(const time_t* timer, struct tm* buf)
            {
#if defined(_WIN32)
                return gmtime_s(buf, timer) == 0;
#else
                return gmtime_r(timer, buf) != nullptr;
#endif
            }
        }
    }
}