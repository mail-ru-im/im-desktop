// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#pragma once

#include <stdint.h>

#include <string>

#include "address_family.h"
#include "ip_address.h"

#if defined(_WIN32)
#include <ws2tcpip.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

struct sockaddr;

namespace core
{

    // An ip_endpoint represents the address of a transport endpoint:
    //  * IP address (either v4 or v6)
    //  * Port
    class ip_endpoint
    {
    public:
        ip_endpoint();
        ~ip_endpoint();
        ip_endpoint(const ip_address& _address, uint16_t _port);
        ip_endpoint(const ip_endpoint& _endpoint);

        const ip_address& address() const { return address_; }
        uint16_t port() const { return port_; }

        // Returns AddressFamily of the address.
        address_family get_family() const;

        // Returns the sockaddr family of the address, AF_INET or AF_INET6.
        int get_sock_addr_family() const;

        // Convert to a provided sockaddr struct.
        // |address| is the sockaddr to copy into.  Should be at least
        //    sizeof(struct sockaddr_storage) bytes.
        // |address_length| is an input/output parameter.  On input, it is the
        //    size of data in |address| available.  On output, it is the size of
        //    the address that was copied into |address|.
        // Returns true on success, false on failure.
        bool to_sock_addr(struct sockaddr* _address, socklen_t* _address_length) const;

        // Convert from a sockaddr struct.
        // |address| is the address.
        // |address_length| is the length of |address|.
        // Returns true on success, false on failure.
        bool from_sock_addr(const struct sockaddr* _address, socklen_t _address_length);


    private:
        friend bool operator<(const ip_endpoint& _lhs, const ip_endpoint& _rhs);
        friend bool operator==(const ip_endpoint& _lhs, const ip_endpoint& _rhs);
        friend bool operator!=(const ip_endpoint& _lhs, const ip_endpoint& _rhs);

        ip_address address_;
        uint16_t port_;
    };

}  // namespace net
