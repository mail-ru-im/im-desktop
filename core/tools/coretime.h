#pragma once

namespace core
{
    namespace tools
    {
        namespace time
        {

            int64_t now_ms() noexcept;

            bool localtime(const time_t* timer, struct tm* buf);
            bool gmtime(const time_t* timer, struct tm* buf);
        }
    }
}