// Copyright (c) 2015 The Chromium Authors. All rights reserved.

#pragma once

#include <cstdint>
#include <array>

namespace core
{
    class ip_address
    {
    public:
        ip_address();
        ip_address(const uint8_t* _data, size_t _data_len);
        static constexpr size_t ip_v4_address_size = 4;
        static constexpr size_t ip_v6_address_size = 16;

        size_t size() const;

        // Returns true if the IP has |ip_v4_address_size| elements.
        bool is_ip_v4() const;

        // Returns true if the IP has |ip_v6_address_size| elements.
        bool is_ip_v6() const;

        // Returns true if the IP is either an IPv4 or IPv6 address. This function
        // only checks the address length.
        bool is_valid() const;

        // Returns true if |ip_address_| is an IPv4-mapped IPv6 address.
        bool is_ip_v4_mapped_ip_v6() const;

        // Returns true if |ip_address_| is 169.254.0.0/16 or fe80::/10, or
        // ::ffff:169.254.0.0/112 (IPv4 mapped IPv6 link-local).
        bool is_link_local() const;

        const uint8_t* data() const { return bytes_.data(); }
        uint8_t* data() { return bytes_.data(); }

    private:
        friend bool operator<(const ip_address& _lhs, const ip_address& _rhs);
        friend bool operator==(const ip_address& _lhs, const ip_address& _rhs);

        // Checks whether address starts with |prefix|. This provides similar
        // functionality as IPAddressMatchesPrefix() but doesn't perform automatic IPv4
        // to IPv4MappedIPv6 conversions and only checks against full bytes.
        template <size_t N>
        bool starts_with(const uint8_t (&prefix)[N]) const
        {
            return std::equal(prefix, prefix + N, bytes_.begin(), bytes_.end());
        }

        std::array<uint8_t, 16> bytes_;
        uint8_t size_;
    };

    // Returns number of matching initial bits between the addresses |a1| and |a2|.
    size_t common_prefix_length(const ip_address& a1, const ip_address& a2);
    // Computes the number of leading 1-bits in |mask|.
    size_t mask_prefix_length(const ip_address& mask);




} // namespace core


