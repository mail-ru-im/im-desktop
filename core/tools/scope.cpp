#include "stdafx.h"

#include "scope.h"
#include "../core.h"

namespace core
{
    namespace tools
    {
        void run_on_core_thread(std::function<void()> _f)
        {
            if (g_core->is_core_thread())
                _f();
            else
                g_core->execute_core_context(std::move(_f));
        }
    }
}