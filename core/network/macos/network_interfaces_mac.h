// Copyright 2017 The Chromium Authors. All rights reserved.

#pragma once

// network_interfaces_getaddrs.cc implements GetNetworkList() using getifaddrs()
// API. It is a non-standard API, so not all POSIX systems implement it (e.g.
// it doesn't exist on Android). It is used on MacOS, iOS and Fuchsia. On Linux
// interface is used to implement GetNetworkList(), see
// network_interfaces_linux.cc.
// This file defines IfaddrsToNetworkInterfaceList() so it can be called in
// unittests.

#include "../internal/network_interfaces.h"

#include <string>

struct ifaddrs;

namespace core
{
    namespace internal
    {
        class ip_attributes_getter
        {
        public:
            ip_attributes_getter() {}
            virtual ~ip_attributes_getter() {}
            virtual bool is_initialized() const = 0;

            // Returns false if the interface must be skipped. Otherwise sets |attributes|
            // and returns true.
            virtual bool get_address_attributes(const ifaddrs* _if_addr, int* _attributes) = 0;

            // Returns interface type for the given interface.
            virtual network_change_notifier::connection_type get_network_interface_type(const ifaddrs* _if_addr) = 0;

        };

        // Converts ifaddrs list returned by getifaddrs() to NetworkInterfaceList. Also
        // filters the list interfaces according to |policy| (see
        // HostAddressSelectionPolicy).
        bool ifaddrs_to_network_interface_list(int _policy, const ifaddrs* _interfaces, ip_attributes_getter* _ip_attributes_getter, network_interface_list* _networks);

    }  // namespace internal
}  // namespace core

