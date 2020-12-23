// Copyright (c) 2015 The Chromium Authors. All rights reserved.

#include "stdafx.h"
#include "ip_address.h"

#include <algorithm>

namespace core 
{
    namespace
    {
        // The prefix for IPv6 mapped IPv4 addresses.
        // https://tools.ietf.org/html/rfc4291#section-2.5.5.2
        constexpr uint8_t ip_v4_mapped_prefix[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};
    }

    ip_address::ip_address() = default;

    ip_address::ip_address(const uint8_t* _data, size_t _data_len)
    {
        size_ = _data_len;
        std::copy_n(_data, _data_len, bytes_.data());
    }

    size_t ip_address::size() const
    {
        return size_;
    }

    bool ip_address::is_ip_v4() const 
    {
        return size() == ip_v4_address_size;
    }

    bool ip_address::is_ip_v6() const 
    {
        return size() == ip_v6_address_size;
    }

    bool ip_address::is_valid() const 
    {
        return is_ip_v4() || is_ip_v6();
    }

    bool ip_address::is_ip_v4_mapped_ip_v6() const
    {
        return is_ip_v6() && starts_with(ip_v4_mapped_prefix);
    }

    bool ip_address::is_link_local() const
    {
        // 169.254.0.0/16
        if (is_ip_v4())
            return (bytes_[0] == 169) && (bytes_[1] == 254);
        // [::ffff:169.254.0.0]/112
        if (is_ip_v4_mapped_ip_v6())
            return (bytes_[12] == 169) && (bytes_[13] == 254);
        // [fe80::]/10
        if (is_ip_v6())
            return (bytes_[0] == 0xFE) && ((bytes_[1] & 0xC0) == 0x80);
        return false;
    }

    size_t mask_prefix_length(const ip_address &mask)
    {
        std::vector<uint8_t> all_ones;
        all_ones.resize(mask.size(), 0xFF);
        return common_prefix_length(mask, ip_address(all_ones.data(), all_ones.size()));
    }

    bool operator<(const ip_address& _lhs, const ip_address& _rhs)
    {
        if (_lhs.size() != _rhs.size())
            return _lhs.size() < _rhs.size();

        return std::lexicographical_compare(_lhs.bytes_.begin(), _lhs.bytes_.end(), _rhs.bytes_.begin(), _rhs.bytes_.end());
    }

    bool operator==(const ip_address& _lhs, const ip_address& _rhs)
    {
        return std::equal(_lhs.bytes_.begin(), _lhs.bytes_.end(), _rhs.bytes_.begin(), _rhs.bytes_.end());
    }

    size_t common_prefix_length(const ip_address &a1, const ip_address &a2)
    {
        assert(a1.size() == a2.size());
          for (size_t i = 0; i < a1.size(); ++i)
          {
            unsigned diff = a1.data()[i] ^ a2.data()[i];
            if (!diff)
              continue;
            for (unsigned j = 0; j < CHAR_BIT; ++j)
            {
              if (diff & (1 << (CHAR_BIT - 1)))
                return i * CHAR_BIT + j;
              diff <<= 1;
            }
          }
          return a1.size() * CHAR_BIT;
    }

} // namespace core
