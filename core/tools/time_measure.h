#pragma once

#include <chrono>

namespace core
{
	namespace tools
	{
	    template<typename TimeT = std::chrono::milliseconds, class ClockT = std::chrono::steady_clock>
	    struct time_measure
	    {
	        template<typename F, typename ...Args>
	        static typename TimeT::rep execution(F&& func, Args&&... args)
	        {
	            auto start = ClockT::now();

	            std::forward<decltype(func)>(func)(std::forward<Args>(args)...);

	            return std::chrono::duration_cast<TimeT>(ClockT::now() - start).count();
	        }
	    };
	}
}
