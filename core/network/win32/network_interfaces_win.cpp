// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#include "stdafx.h"
#include "network_interfaces_win.h"

#include <algorithm>
#include <memory>

#include <iphlpapi.h>

#include "../network_change_notifier.h"
#include "../internal/ip_endpoint.h"

#include "../../core.h"
#include "../../../common.shared/string_utils.h"

namespace core
{
    namespace
    {
        // Converts Windows defined types to NetworkInterfaceType.
        network_change_notifier::connection_type get_network_interface_type(DWORD _ifType)
        {
            network_change_notifier::connection_type type = network_change_notifier::CONNECTION_UNKNOWN;
            if (_ifType == IF_TYPE_ETHERNET_CSMACD)
                type = network_change_notifier::CONNECTION_ETHERNET;
            else if (_ifType == IF_TYPE_IEEE80211)
                type = network_change_notifier::CONNECTION_WIFI;
            return type;
        }

        std::string sys_wide_to_multi_byte(const std::wstring& _wide)
        {
            uint32_t code_page = CP_ACP;
            int wide_length = static_cast<int>(_wide.length());
            if (wide_length == 0)
                return std::string();

            // Compute the length of the buffer we'll need.
            int charcount = WideCharToMultiByte(code_page, 0, _wide.data(), wide_length, NULL, 0, NULL, NULL);
            if (charcount == 0)
                return std::string();

            std::string mb;
            mb.resize(charcount);
            WideCharToMultiByte(code_page, 0, _wide.data(), wide_length, &mb[0], charcount, NULL, NULL);

            return mb;
        }

    }  // namespace

    namespace internal
    {
        bool get_network_list_impl(network_interface_list* _networks, int policy, const IP_ADAPTER_ADDRESSES* _adapters)
        {
            for (const IP_ADAPTER_ADDRESSES* adapter = _adapters; adapter != nullptr; adapter = adapter->Next)
            {
                // Ignore the loopback device.
                if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
                    continue;

                if (adapter->OperStatus != IfOperStatusUp)
                    continue;

                // Ignore any HOST side vmware adapters with a description like:
                // VMware Virtual Ethernet Adapter for VMnet1
                // but don't ignore any GUEST side adapters with a description like:
                // VMware Accelerated AMD PCNet Adapter #2
                if ((policy & EXCLUDE_HOST_SCOPE_VIRTUAL_INTERFACES) && strstr(adapter->AdapterName, "VMnet") != nullptr)
                    continue;

                for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address; address = address->Next)
                {
                    int family = address->Address.lpSockaddr->sa_family;
                    if (family == AF_INET || family == AF_INET6) 
                    {
                        ip_endpoint endpoint;
                        if (endpoint.from_sock_addr(address->Address.lpSockaddr, address->Address.iSockaddrLength)) 
                        {
                            size_t prefix_length = address->OnLinkPrefixLength;

                            // If the duplicate address detection (DAD) state is not changed to
                            // Preferred, skip this address.
                            if (address->DadState != IpDadStatePreferred) 
                                continue;

                            uint32_t index = (family == AF_INET) ? adapter->IfIndex : adapter->Ipv6IfIndex;

                            // From http://technet.microsoft.com/en-us/ff568768(v=vs.60).aspx, the
                            // way to identify a temporary IPv6 Address is to check if
                            // PrefixOrigin is equal to IpPrefixOriginRouterAdvertisement and
                            // SuffixOrigin equal to IpSuffixOriginRandom.
                            int ip_address_attributes = IP_ADDRESS_ATTRIBUTE_NONE;
                            if (family == AF_INET6) 
                            {
                                if (address->PrefixOrigin == IpPrefixOriginRouterAdvertisement && address->SuffixOrigin == IpSuffixOriginRandom) 
                                    ip_address_attributes |= IP_ADDRESS_ATTRIBUTE_TEMPORARY;
                                if (address->PreferredLifetime == 0) 
                                    ip_address_attributes |= IP_ADDRESS_ATTRIBUTE_DEPRECATED;
                            }
                            _networks->push_back(network_interface( adapter->AdapterName, sys_wide_to_multi_byte(adapter->FriendlyName), index, get_network_interface_type(adapter->IfType), endpoint.address(), prefix_length, ip_address_attributes));
                        }
                    }
                }
            }
            return true;
        }

    }  // namespace internal

    bool get_network_list(network_interface_list* _networks, int _policy) 
    {
        // Max number of times to retry GetAdaptersAddresses due to
        // ERROR_BUFFER_OVERFLOW. If GetAdaptersAddresses returns this indefinitely
        // due to an unforseen reason, we don't want to be stuck in an endless loop.
        static constexpr int MAX_GETADAPTERSADDRESSES_TRIES = 10;
        // Use an initial buffer size of 15KB, as recommended by MSDN. See:
        // https://msdn.microsoft.com/en-us/library/windows/desktop/aa365915(v=vs.85).aspx
        static constexpr int INITIAL_BUFFER_SIZE = 15000;

        ULONG len = INITIAL_BUFFER_SIZE;
        ULONG flags = 0;
        // Initial buffer allocated on stack.
        char initial_buf[INITIAL_BUFFER_SIZE];
        // Dynamic buffer in case initial buffer isn't large enough.
        std::unique_ptr<char[]> buf;

        IP_ADAPTER_ADDRESSES* adapters = nullptr;
        {
            adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(&initial_buf);
            ULONG result = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapters, &len);

            // If we get ERROR_BUFFER_OVERFLOW, call GetAdaptersAddresses in a loop,
            // because the required size may increase between successive calls,
            // resulting in ERROR_BUFFER_OVERFLOW multiple times.
            for (int tries = 1; result == ERROR_BUFFER_OVERFLOW && tries < MAX_GETADAPTERSADDRESSES_TRIES; ++tries) 
            {
                buf = std::make_unique<char[]>(len);
                adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.get());
                result = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapters, &len);
            }

            if (result == ERROR_NO_DATA) 
            {
                // There are 0 networks.
                return true;
            }
            else if (result != NO_ERROR) 
            {
                network_change_notifier::log_text(su::concat("GetAdaptersAddresses failed: ", std::to_string(result)));
                return false;
            }
        }

        return internal::get_network_list_impl(_networks, _policy, adapters);
    }

}  // namespace core
