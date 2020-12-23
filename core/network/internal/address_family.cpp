// Copyright (c) 2015 The Chromium Authors. All rights reserved.

#include "stdafx.h"
#include "address_family.h"

#include "ip_address.h"

#if defined(_WIN32)
#include <ws2tcpip.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace core 
{
    address_family get_address_family(const ip_address& _address) 
    {
        if (_address.is_ip_v4())
            return ADDRESS_FAMILY_IPV4;
        else if (_address.is_ip_v6())
            return ADDRESS_FAMILY_IPV6;
        else
            return ADDRESS_FAMILY_UNSPECIFIED;
    }

    int convert_address_family(address_family _addr_family)
    {
        switch (_addr_family)
        {
        case ADDRESS_FAMILY_UNSPECIFIED:
            return AF_UNSPEC;
        case ADDRESS_FAMILY_IPV4:
            return AF_INET;
        case ADDRESS_FAMILY_IPV6:
            return AF_INET6;
        }
        return AF_UNSPEC;
    }

}  // namespace core
