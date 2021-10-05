// Copyright 2017 The Chromium Authors. All rights reserved.

#include "stdafx.h"
#include "network_interfaces_mac.h"

#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_media.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <memory>
#include <set>

#include "../internal/ip_endpoint.h"
#include "../../core.h"


namespace core
{
    namespace internal
    {
        // MacOSX implementation of ip_attributes_getter which calls ioctl() on socket to
        // retrieve IP attributes.
        class ip_attributes_getter_mac : public internal::ip_attributes_getter
        {
        public:
            ip_attributes_getter_mac();
            ~ip_attributes_getter_mac() override;
            bool is_initialized() const override;
            bool get_address_attributes(const ifaddrs* _if_addr, int* _attributes) override;
            network_change_notifier::connection_type get_network_interface_type(const ifaddrs* _if_addr) override;

        private:
            int ioctl_socket_;
        };

        ip_attributes_getter_mac::ip_attributes_getter_mac()
            : ioctl_socket_(socket(AF_INET6, SOCK_DGRAM, 0))
        {
            im_assert(ioctl_socket_ >= 0);
        }

        ip_attributes_getter_mac::~ip_attributes_getter_mac()
        {
            if (is_initialized()) {
                close(ioctl_socket_);
            }
        }

        bool ip_attributes_getter_mac::is_initialized() const
        {
            return ioctl_socket_ >= 0;
        }

        int address_flags_to_net_address_attributes(int _flags)
        {
            int result = 0;
            if (_flags & IN6_IFF_TEMPORARY)
                result |= IP_ADDRESS_ATTRIBUTE_TEMPORARY;

            if (_flags & IN6_IFF_DEPRECATED)
                result |= IP_ADDRESS_ATTRIBUTE_DEPRECATED;

            if (_flags & IN6_IFF_ANYCAST)
                result |= IP_ADDRESS_ATTRIBUTE_ANYCAST;

            if (_flags & IN6_IFF_TENTATIVE)
                result |= IP_ADDRESS_ATTRIBUTE_TENTATIVE;

            if (_flags & IN6_IFF_DUPLICATED)
                result |= IP_ADDRESS_ATTRIBUTE_DUPLICATED;

            if (_flags & IN6_IFF_DETACHED)
                result |= IP_ADDRESS_ATTRIBUTE_DETACHED;

            return result;
        }

        bool ip_attributes_getter_mac::get_address_attributes(const ifaddrs* _if_addr, int* _attributes)
        {
            struct in6_ifreq ifr = {};
            strncpy(ifr.ifr_name, _if_addr->ifa_name, sizeof(ifr.ifr_name) - 1);
            memcpy(&ifr.ifr_ifru.ifru_addr, _if_addr->ifa_addr, _if_addr->ifa_addr->sa_len);
            int rv = ioctl(ioctl_socket_, SIOCGIFAFLAG_IN6, &ifr);
            if (rv >= 0)
                *_attributes = address_flags_to_net_address_attributes(ifr.ifr_ifru.ifru_flags);

            return (rv >= 0);
        }

        network_change_notifier::connection_type ip_attributes_getter_mac::get_network_interface_type(const ifaddrs* _if_addr)
        {
            if (!is_initialized())
                return network_change_notifier::CONNECTION_UNKNOWN;

            struct ifmediareq ifmr = {};
            strncpy(ifmr.ifm_name, _if_addr->ifa_name, sizeof(ifmr.ifm_name) - 1);

            if (ioctl(ioctl_socket_, SIOCGIFMEDIA, &ifmr) != -1)
            {
                if (ifmr.ifm_current & IFM_IEEE80211)
                    return network_change_notifier::CONNECTION_WIFI;

                if (ifmr.ifm_current & IFM_ETHER)
                    return network_change_notifier::CONNECTION_ETHERNET;
            }

            return network_change_notifier::CONNECTION_UNKNOWN;
        }


        bool ifaddrs_to_network_interface_list(int _policy, const ifaddrs* _interfaces, core::internal::ip_attributes_getter* _ip_attributes_getter, network_interface_list* _networks)
        {
            // Enumerate the addresses assigned to network interfaces which are up.
            for (const ifaddrs* interface = _interfaces; interface != NULL;
                interface = interface->ifa_next) {
                // Skip loopback interfaces, and ones which are down.
                if (!(IFF_RUNNING & interface->ifa_flags))
                    continue;
                if (IFF_LOOPBACK & interface->ifa_flags)
                    continue;
                // Skip interfaces with no address configured.
                struct sockaddr* addr = interface->ifa_addr;
                if (!addr)
                    continue;

                // Skip unspecified addresses (i.e. made of zeroes) and loopback addresses
                // configured on non-loopback interfaces.
                if (is_loopback_or_unspecified_address(addr))
                    continue;

                std::string name = interface->ifa_name;
                // Filter out VMware interfaces, typically named vmnet1 and vmnet8.
                if (should_ignore_interface(name, _policy))
                    continue;


                network_change_notifier::connection_type connection_type = network_change_notifier::CONNECTION_UNKNOWN;

                int ip_attributes = IP_ADDRESS_ATTRIBUTE_NONE;

                // Retrieve native ip attributes and convert to net version if a getter is
                // given.
                if (_ip_attributes_getter && _ip_attributes_getter->is_initialized())
                {
                    if (addr->sa_family == AF_INET6 && _ip_attributes_getter->get_address_attributes(interface, &ip_attributes))
                    {
                        // Disallow addresses with attributes ANYCASE, DUPLICATED, TENTATIVE,
                        // and DETACHED as these are still progressing through duplicated
                        // address detection (DAD) or are not suitable to be used in an
                        // one-to-one communication and shouldn't be used by the application
                        // layer.
                        if (ip_attributes & (IP_ADDRESS_ATTRIBUTE_ANYCAST | IP_ADDRESS_ATTRIBUTE_DUPLICATED | IP_ADDRESS_ATTRIBUTE_TENTATIVE | IP_ADDRESS_ATTRIBUTE_DETACHED))
                            continue;

                    }

                    connection_type = _ip_attributes_getter->get_network_interface_type(interface);
                }

                ip_endpoint address;

                int addr_size = 0;
                if (addr->sa_family == AF_INET6)
                    addr_size = sizeof(sockaddr_in6);
                else if (addr->sa_family == AF_INET)
                    addr_size = sizeof(sockaddr_in);


                if (address.from_sock_addr(addr, addr_size)) {

                    uint8_t prefix_length = 0;
                    if (interface->ifa_netmask)
                    {
                        // If not otherwise set, assume the same sa_family as ifa_addr.
                        if (interface->ifa_netmask->sa_family == 0)
                            interface->ifa_netmask->sa_family = addr->sa_family;

                        ip_endpoint netmask;
                        if (netmask.from_sock_addr(interface->ifa_netmask, addr_size))
                            prefix_length = mask_prefix_length(netmask.address());

                    }
                    _networks->push_back(network_interface(name, name, if_nametoindex(name.c_str()), connection_type, address.address(), prefix_length, ip_attributes));
                }
            }

            return true;
        }

    }  // namespace internal


    bool get_network_list(network_interface_list* _networks, int _policy)
    {
        if (_networks == NULL)
            return false;

        ifaddrs* interfaces;
        if (getifaddrs(&interfaces) < 0)
        {
            network_change_notifier::log_text("Error in getifaddrs");
            return false;
        }

        std::unique_ptr<internal::ip_attributes_getter> ip_attributes_getter;

        ip_attributes_getter = std::make_unique<internal::ip_attributes_getter_mac>();


        bool result = internal::ifaddrs_to_network_interface_list(_policy, interfaces, ip_attributes_getter.get(), _networks);
        freeifaddrs(interfaces);
        return result;
    }

}  // namespace net
