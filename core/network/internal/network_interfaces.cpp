// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#include "stdafx.h"

#include "network_interfaces.h"

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

#if defined(_WIN32)
#include <winsock2.h>
#include "../win32/winsock_util.h"
#endif

#include "../../core.h"
#include "../../../common.shared/string_utils.h"

namespace core
{

    network_interface::network_interface()
        : type_(network_change_notifier::CONNECTION_UNKNOWN), 
          prefix_length_(0) 
    {
    }

    network_interface::network_interface(const std::string& _name, const std::string& _friendly_name, uint32_t _interface_index, network_change_notifier::connection_type _type, const ip_address& _address, uint32_t _prefix_length, int _ip_address_attributes)
        : name_(_name),
          friendly_name_(_friendly_name),
          interface_index_(_interface_index),
          type_(_type),
          address_(_address),
          prefix_length_(_prefix_length),
          ip_address_attributes_(_ip_address_attributes) 
    {
    }

    network_interface::network_interface(const network_interface& _other) = default;

    network_interface::~network_interface() = default;

    std::string get_host_name()
    {
#if defined(_WIN32)
        internal::ensure_winsock_init();
#endif

        // Host names are limited to 255 bytes.
        char buffer[256];
        int result = gethostname(buffer, sizeof(buffer));
        if (result != 0)
        {
            network_change_notifier::log_text(su::concat("gethostname() failed with ", std::to_string(result)));
            buffer[0] = '\0';
        }
        return std::string(buffer);
    }

#if !defined(_WIN32)
    // The application layer can pass |policy| defined in net_util.h to
    // request filtering out certain type of interfaces.
    bool should_ignore_interface(const std::string& _name, int _policy)
    {
        // Filter out VMware interfaces, typically named vmnet1 and vmnet8,
        // which might not be useful for use cases like WebRTC.
        if ((_policy & EXCLUDE_HOST_SCOPE_VIRTUAL_INTERFACES) && ((_name.find("vmnet") != std::string::npos) || (_name.find("vnic") != std::string::npos)))
            return true;

        return false;
    }

    // Check if the address is unspecified (i.e. made of zeroes) or loopback.
    bool is_loopback_or_unspecified_address(const sockaddr* _addr)
    {
        if (_addr->sa_family == AF_INET6)
        {
            const struct sockaddr_in6* addr_in6 = reinterpret_cast<const struct sockaddr_in6*>(_addr);
            const struct in6_addr* sin6_addr = &addr_in6->sin6_addr;
            if (IN6_IS_ADDR_LOOPBACK(sin6_addr) || IN6_IS_ADDR_UNSPECIFIED(sin6_addr))
                return true;
        }
        else if (_addr->sa_family == AF_INET)
        {
            const struct sockaddr_in* addr_in = reinterpret_cast<const struct sockaddr_in*>(_addr);
            if (addr_in->sin_addr.s_addr == INADDR_LOOPBACK || addr_in->sin_addr.s_addr == 0)
                return true;
        }
        else
        {
            // Skip non-IP addresses.
            return true;
        }
        return false;
    }
#endif

}  // namespace core
