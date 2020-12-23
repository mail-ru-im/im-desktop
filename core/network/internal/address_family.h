// Copyright (c) 2010 The Chromium Authors. All rights reserved.

#pragma once

namespace core 
{
    class ip_address;

    // Enum wrapper around the address family types supported by host resolver
    // procedures.
    enum address_family 
    {
        ADDRESS_FAMILY_UNSPECIFIED,   // AF_UNSPEC
        ADDRESS_FAMILY_IPV4,          // AF_INET
        ADDRESS_FAMILY_IPV6,          // AF_INET6
        ADDRESS_FAMILY_LAST = ADDRESS_FAMILY_IPV6
    };

    // HostResolverFlags is a bitflag enum used by host resolver procedures to
    // determine the value of addrinfo.ai_flags and work around getaddrinfo
    // peculiarities.
    enum 
    {
        HOST_RESOLVER_CANONNAME = 1 << 0,  // AI_CANONNAME
        // Hint to the resolver proc that only loopback addresses are configured.
        HOST_RESOLVER_LOOPBACK_ONLY = 1 << 1,
        // Indicate the address family was set because no IPv6 support was detected.
        HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6 = 1 << 2,
        // The resolver should only invoke getaddrinfo, not DnsClient.
        HOST_RESOLVER_SYSTEM_ONLY = 1 << 3
    };

    // Returns address_family for |address|.
    address_family get_address_family(const ip_address& _address);

    // Maps the given address_family to either AF_INET, AF_INET6 or AF_UNSPEC.
    int convert_address_family(address_family _address_family);

}  // namespace core

