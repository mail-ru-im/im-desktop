// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#pragma once

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "ip_address.h"
#include "../network_change_notifier.h"

namespace core
{

    // A subset of IP address attributes which are actionable by the
    // application layer. Currently unimplemented for all hosts;
    // IP_ADDRESS_ATTRIBUTE_NONE is always returned.
    enum ip_address_attributes
    {
        IP_ADDRESS_ATTRIBUTE_NONE = 0,

        // A temporary address is dynamic by nature and will not contain MAC
        // address. Presence of MAC address in IPv6 addresses can be used to
        // track an endpoint and cause privacy concern. Please refer to
        // RFC4941.
        IP_ADDRESS_ATTRIBUTE_TEMPORARY = 1 << 0,

        // A temporary address could become deprecated once the preferred
        // lifetime is reached. It is still valid but shouldn't be used to
        // create new connections.
        IP_ADDRESS_ATTRIBUTE_DEPRECATED = 1 << 1,

        // Anycast address.
        IP_ADDRESS_ATTRIBUTE_ANYCAST = 1 << 2,

        // Tentative address.
        IP_ADDRESS_ATTRIBUTE_TENTATIVE = 1 << 3,

        // DAD detected duplicate.
        IP_ADDRESS_ATTRIBUTE_DUPLICATED = 1 << 4,

        // May be detached from the link.
        IP_ADDRESS_ATTRIBUTE_DETACHED = 1 << 5,
    };

    // struct that is used by get_network_list() to represent a network
    // interface.
    struct network_interface
    {

        network_interface();
        network_interface(const std::string& _name, const std::string& _friendly_name, uint32_t _interface_index, network_change_notifier::connection_type _type, const ip_address& _address, uint32_t _prefix_length, int _ip_address_attributes);
        network_interface(const network_interface& _other);
        ~network_interface();

        std::string name_;
        std::string friendly_name_;  // Same as |name| on non-Windows.
        uint32_t interface_index_;  // Always 0 on Android.
        network_change_notifier::connection_type type_;
        ip_address address_;
        uint32_t prefix_length_;
        int ip_address_attributes_;  // Combination of |ip_address_attributes|.
    };

    typedef std::vector<network_interface> network_interface_list;

    // Policy settings to include/exclude network interfaces.
    enum host_address_selection_policy
    {
        INCLUDE_HOST_SCOPE_VIRTUAL_INTERFACES = 0x0,
        EXCLUDE_HOST_SCOPE_VIRTUAL_INTERFACES = 0x1
    };

    // Returns list of network interfaces except loopback interface. If an
    // interface has more than one address, a separate entry is added to
    // the list for each address.
    bool get_network_list(network_interface_list* _networks, int _policy);
    // Returns the hostname of the current system. Returns empty string on failure.
    std::string get_host_name();

#if !defined(_WIN32)
    // The application layer can pass |policy| defined in net_util.h to
    // request filtering out certain type of interfaces.
    bool should_ignore_interface(const std::string& _name, int _policy);

    // Check if the address is unspecified (i.e. made of zeroes) or loopback.
    bool is_loopback_or_unspecified_address(const sockaddr* _addr);
#endif

}  // namespace core

