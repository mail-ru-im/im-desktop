// Copyright (c) 2014 The Chromium Authors. All rights reserved.

#pragma once

#include <string>
#include <unordered_set>

#include "address_tracker_linux.h"
#include "../internal/network_interfaces.h"

namespace core
{
    namespace internal
    {

        int socket_for_ioctl();
        bool get_network_list_impl(network_interface_list* _networks, int _policy, const std::unordered_set<int>& _online_links, const internal::address_tracker_linux::address_map& _address_map);

    }  // namespace internal
}  // namespace core

