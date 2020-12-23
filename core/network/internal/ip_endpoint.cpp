// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#include "stdafx.h"

#include "ip_endpoint.h"


#if defined(_WIN32)
#include <winsock2.h>
#include <ws2bth.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <netinet/in.h>
#endif

#include <string.h>

#include <tuple>

#include "ip_address.h"

#if defined(_WIN32)
#include "../win32/winsock_util.h"
#endif

namespace core 
{
    namespace
    {

#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#endif

        uint64_t byte_swap(uint16_t x) 
        {
#if defined(COMPILER_MSVC)
            return _byteswap_ushort(x);
#else
            return __builtin_bswap16(x);
#endif
        }

        // By definition, socklen_t is large enough to hold both sizes.
        constexpr socklen_t sockaddr_in_size() noexcept { return sizeof(struct sockaddr_in); }
        constexpr socklen_t sockaddr_in6_size() noexcept { return sizeof(struct sockaddr_in6); }

        // Extracts the address and port portions of a sockaddr.
        bool get_ip_address_from_sock_addr(const struct sockaddr* _sock_addr, socklen_t _sock_addr_len, const uint8_t** _address, size_t* _address_len, uint16_t* _port)
        {
            if (_sock_addr->sa_family == AF_INET) 
            {
                if (_sock_addr_len < static_cast<socklen_t>(sizeof(struct sockaddr_in)))
                    return false;
                const struct sockaddr_in* addr = reinterpret_cast<const struct sockaddr_in*>(_sock_addr);
                *_address = reinterpret_cast<const uint8_t*>(&addr->sin_addr);
                *_address_len = ip_address::ip_v4_address_size;
                if (_port)
                    *_port = byte_swap(addr->sin_port);
                return true;
            }

            if (_sock_addr->sa_family == AF_INET6) 
            {
                if (_sock_addr_len < static_cast<socklen_t>(sizeof(struct sockaddr_in6)))
                    return false;
                const struct sockaddr_in6* addr = reinterpret_cast<const struct sockaddr_in6*>(_sock_addr);
                *_address = reinterpret_cast<const uint8_t*>(&addr->sin6_addr);
                *_address_len = ip_address::ip_v6_address_size;
                if (_port)
                    *_port = byte_swap(addr->sin6_port);
                return true;
            }
            // sock_addr->sa_family == AF_BTH not supported

            return false;  // Unrecognized |sa_family|.
        }

    }  // namespace

    ip_endpoint::ip_endpoint() : port_(0) {}

    ip_endpoint::~ip_endpoint() = default;

    ip_endpoint::ip_endpoint(const ip_address& _address, uint16_t _port)
        : address_(_address), port_(_port) {}

    ip_endpoint::ip_endpoint(const ip_endpoint& _endpoint) 
    {
        address_ = _endpoint.address_;
        port_ = _endpoint.port_;
    }

    address_family ip_endpoint::get_family() const 
    {
        return get_address_family(address_);
    }

    int ip_endpoint::get_sock_addr_family() const
    {
        switch (address_.size()) 
        {
        case ip_address::ip_v4_address_size:
            return AF_INET;
        case ip_address::ip_v6_address_size:
            return AF_INET6;
        default:
            return AF_UNSPEC;
        }
    }

    bool ip_endpoint::to_sock_addr(struct sockaddr* _address, socklen_t* _address_length) const 
    {
        assert(_address);
        assert(_address_length);
        switch (address_.size()) 
        {
        case ip_address::ip_v4_address_size: 
        {
            if (*_address_length < sockaddr_in_size())
                return false;
            *_address_length = sockaddr_in_size();
            struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(_address);
            memset(addr, 0, sizeof(struct sockaddr_in));
            addr->sin_family = AF_INET;
            addr->sin_port = byte_swap(port_);
            memcpy(&addr->sin_addr, address_.data(),
                ip_address::ip_v4_address_size);
            break;
        }
        case ip_address::ip_v6_address_size: 
        {
            if (*_address_length < sockaddr_in6_size())
                return false;
            *_address_length = sockaddr_in6_size();
            struct sockaddr_in6* addr6 = reinterpret_cast<struct sockaddr_in6*>(_address);
            memset(addr6, 0, sizeof(struct sockaddr_in6));
            addr6->sin6_family = AF_INET6;
            addr6->sin6_port = byte_swap(port_);
            memcpy(&addr6->sin6_addr, address_.data(), ip_address::ip_v6_address_size);
            break;
        }
        default:
            return false;
        }
        return true;
    }

    bool ip_endpoint::from_sock_addr(const struct sockaddr* _sock_addr, socklen_t _sock_addr_len) 
    {
        assert(_sock_addr);

        const uint8_t* address;
        size_t address_len;
        uint16_t port;
        if (!get_ip_address_from_sock_addr(_sock_addr, _sock_addr_len, &address, &address_len, &port)) 
            return false;

        address_ = ip_address(address, address_len);
        port_ = port;
        return true;
    }

    
    bool operator<(const ip_endpoint& _lhs, const ip_endpoint& _rhs)
    {
        // Sort IPv4 before IPv6.
        if (_lhs.address_.size() != _rhs.address_.size()) {
            return _lhs.address_.size() < _rhs.address_.size();
        }
        return std::tie(_lhs.address_, _lhs.port_) < std::tie(_rhs.address_, _rhs.port_);        
    }

    bool operator==(const ip_endpoint& _lhs, const ip_endpoint& _rhs)
    {
        return _lhs.address_ == _rhs.address_ && _lhs.port_ == _rhs.port_;        
    }

    bool operator!=(const ip_endpoint& _lhs, const ip_endpoint& _rhs)
    {
        return !(_lhs == _rhs);        
    }

}  // namespace core
