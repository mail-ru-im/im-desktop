// Copyright (c) 2014 The Chromium Authors. All rights reserved.

#pragma once

#include <winsock2.h>
#include <iphlpapi.h>
#include <wlanapi.h>

#include "../internal/network_interfaces.h"

namespace core 
{
    namespace internal
    {
        bool get_network_list_impl(network_interface_list* _networks, int _policy, const IP_ADAPTER_ADDRESSES* _ip_adapter_addresses);
    }  // namespace internal
}  // namespace core

