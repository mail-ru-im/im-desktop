#include "stdafx.h"

#include "../quarantine.h"

namespace core::wim::quarantine
{
    quarantine_result quarantine_file(const quarantine_params& /*_params*/)
    {
        return quarantine_result::ok;
    }
}