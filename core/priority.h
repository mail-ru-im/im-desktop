#pragma once

namespace core
{
    using milliseconds_t = long;

    using hash_t = size_t;

    using priority_t = int; // the lower number is the higher priority

    constexpr inline priority_t increase_priority() noexcept { return -5; }
    constexpr inline priority_t decrease_priority() noexcept { return 5; }

    constexpr inline priority_t priority_fetch() noexcept { return 0; }
    constexpr inline priority_t priority_send_message() noexcept { return 1; }
    constexpr inline priority_t priority_protocol() noexcept { return 2; }

    constexpr inline priority_t top_priority() noexcept { return 3; }

    constexpr inline priority_t packets_priority_high() noexcept { return 40; }
    constexpr inline priority_t packets_priority() noexcept { return 45; }

    // non wim-send-thread priorities
    constexpr inline priority_t highest_priority() noexcept { return 50; }
    constexpr inline priority_t high_priority() noexcept { return 100; }
    constexpr inline priority_t default_priority() noexcept { return 150; }
    constexpr inline priority_t low_priority() noexcept { return 200; }
    constexpr inline priority_t lowest_priority() noexcept { return 250; }
}
