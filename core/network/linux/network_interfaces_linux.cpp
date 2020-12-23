// Copyright (c) 2014 The Chromium Authors. All rights reserved.

#include "network_interfaces_linux.h"

#include <memory>

#include <linux/ethtool.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <set>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "address_tracker_linux.h"
#include "../internal/ip_endpoint.h"

#define SIOCGIWNAME	0x8B01		// get name == wireless protocol
// SIOCGIWNAME is used to verify the presence of Wireless Extensions.
// Common values : "IEEE 802.11-DS", "IEEE 802.11-FH", "IEEE 802.11b"...
// Don't put the name of your driver there, it's useless.

namespace core 
{

    namespace 
    {
        // When returning true, the platform native IPv6 address attributes were
        // successfully converted to net IP address attributes. Otherwise, returning
        // false and the caller should drop the IP address which can't be used by the
        // application layer.
        bool try_convert_native_to_net_ip_attributes(int _native_attributes, int* _net_attributes)
        {
            // For Linux/ChromeOS/Android, we disallow addresses with attributes
            // IFA_F_OPTIMISTIC, IFA_F_DADFAILED, and IFA_F_TENTATIVE as these
            // are still progressing through duplicated address detection (DAD)
            // and shouldn't be used by the application layer until DAD process
            // is completed.
            if (_native_attributes & (IFA_F_OPTIMISTIC | IFA_F_DADFAILED | IFA_F_TENTATIVE)) 
                return false;
            
            if (_native_attributes & IFA_F_TEMPORARY) 
                *_net_attributes |= IP_ADDRESS_ATTRIBUTE_TEMPORARY;            

            if (_native_attributes & IFA_F_DEPRECATED) 
                *_net_attributes |= IP_ADDRESS_ATTRIBUTE_DEPRECATED;            

            return true;
        }

    }  // namespace

    namespace internal 
    {

        // Gets the connection type for interface |ifname| by checking for wireless
        // or ethtool extensions.
        network_change_notifier::connection_type get_interface_connection_type(const std::string& _ifname)
        {
            int s = socket_for_ioctl();
            if (s == -1)
                return network_change_notifier::CONNECTION_UNKNOWN;

            // Test wireless extensions for CONNECTION_WIFI
            struct ifreq pwrq = {};
            strncpy(pwrq.ifr_name, _ifname.c_str(), IFNAMSIZ - 1);
            if (ioctl(s, SIOCGIWNAME, &pwrq) != -1)
            {
                close(s);
                return network_change_notifier::CONNECTION_WIFI;
            }

            // Test ethtool for CONNECTION_ETHERNET
            struct ethtool_cmd ecmd = {};
            ecmd.cmd = ETHTOOL_GSET;
            struct ifreq ifr = {};
            ifr.ifr_data = reinterpret_cast<char*>(&ecmd);
            strncpy(ifr.ifr_name, _ifname.c_str(), IFNAMSIZ - 1);
            if (ioctl(s, SIOCETHTOOL, &ifr) != -1) 
            {
                close(s);
                return network_change_notifier::CONNECTION_ETHERNET;
            }

            close(s);
            return network_change_notifier::CONNECTION_UNKNOWN;
        }

        bool get_network_list_impl(network_interface_list* _networks, int _policy, const std::unordered_set<int>& _online_links, const internal::address_tracker_linux::address_map& _address_map)
        {
            std::unordered_map<int, std::string> ifnames;

            for (auto it = _address_map.begin(); it != _address_map.end(); ++it)
            {
                // Ignore addresses whose links are not online.
                if (_online_links.find(it->second.ifa_index) == _online_links.end())
                    continue;

                sockaddr_storage sock_addr;
                socklen_t sock_len = sizeof(sockaddr_storage);

                // Convert to sockaddr for next check.
                if (!ip_endpoint(it->first, 0).to_sock_addr(reinterpret_cast<sockaddr*>(&sock_addr), &sock_len))
                    continue;


                // Skip unspecified addresses (i.e. made of zeroes) and loopback addresses
                if (is_loopback_or_unspecified_address(reinterpret_cast<sockaddr*>(&sock_addr)))
                    continue;

                int ip_attributes = IP_ADDRESS_ATTRIBUTE_NONE;

                if (it->second.ifa_family == AF_INET6)
                {
                    // Ignore addresses whose attributes are not actionable by
                    // the application layer.
                    if (!try_convert_native_to_net_ip_attributes(it->second.ifa_flags, &ip_attributes))
                        continue;
                }

                // Find the name of this link.
                std::unordered_map<int, std::string>::const_iterator itname = ifnames.find(it->second.ifa_index);
                std::string ifname;
                if (itname == ifnames.end())
                {
                    char buffer[IFNAMSIZ] = { 0 };
                    ifname = address_tracker_linux::get_interface_name(it->second.ifa_index, buffer);
                    // Ignore addresses whose interface name can't be retrieved.
                    if (ifname.empty())
                        continue;
                    ifnames[it->second.ifa_index] = ifname;
                }
                else
                {
                    ifname = itname->second;
                }

                // Based on the interface name and policy, determine whether we
                // should ignore it.
                if (should_ignore_interface(ifname, _policy))
                    continue;

                network_change_notifier::connection_type type = get_interface_connection_type(ifname);

                _networks->push_back(network_interface(ifname, ifname, it->second.ifa_index, type, it->first, it->second.ifa_prefixlen, ip_attributes));
            }

            return true;
        }


        int socket_for_ioctl()
        {
            int ioctl_socket = socket(AF_INET6, SOCK_DGRAM, 0);
            if (ioctl_socket > 0)
                return ioctl_socket;
            return socket(AF_INET, SOCK_DGRAM, 0);
        }


    }  // namespace internal

}  // namespace core
