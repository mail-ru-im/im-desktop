#include "stdafx.h"

#include "tools/device_id.h"
#include "../common.shared/common_defs.h"

std::string core::tools::impl::get_device_id_impl()
{
    return common::get_win_device_id();
}
