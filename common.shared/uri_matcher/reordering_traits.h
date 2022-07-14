#pragma once
#include <iterator>
#include <algorithm>

template<class _It>
inline constexpr bool is_random_access_v = std::is_same_v<typename std::iterator_traits<_It>::iterator_category, std::random_access_iterator_tag>;

template<class _It>
inline constexpr bool is_bidirectional_v = std::is_same_v<typename std::iterator_traits<_It>::iterator_category, std::bidirectional_iterator_tag>;


enum class reordering_policy
{
    none, // do not move anything anywhere
    mtf, // move-to-front rearrange optimization
    transpose, // transpose rearrange optimization
};


template<class _Impl>
struct basic_reorderer
{
    template<class _BiIt>
    _BiIt operator()(_BiIt _cur, _BiIt _first, _BiIt _last) const noexcept
    {
        return _Impl::reorder(_cur, _first, _last);
    }
};

template<reordering_policy _Policy = reordering_policy::none>
struct reorderer :
    basic_reorderer<reorderer<reordering_policy::none>>
{
    template<class _BiIt>
    static _BiIt reorder(_BiIt _curr, _BiIt, _BiIt) noexcept
    {
        return _curr; // no-op
    }
};

template<>
struct reorderer<reordering_policy::mtf> :
    basic_reorderer<reorderer<reordering_policy::mtf>>
{
    template<class _BiIt>
    static _BiIt reorder(_BiIt _curr, _BiIt _first, _BiIt _last) noexcept
    {
        static_assert (is_random_access_v<_BiIt> || is_bidirectional_v<_BiIt>,
            "unable to reorder this category of iterators");
        if (_curr == _first || _curr == _last)
            return _curr;

        _curr = std::rotate(_first, _curr, _curr + 1);
        return (--_curr);
    }
};

template<>
struct reorderer<reordering_policy::transpose> :
    basic_reorderer<reorderer<reordering_policy::transpose>>
{
    template<class _BiIt>
    static _BiIt reorder(_BiIt _curr, _BiIt _first, _BiIt _last) noexcept
    {
        static_assert (is_random_access_v<_BiIt> || is_bidirectional_v<_BiIt>,
            "unable to reorder this category of iterators");

        if (_curr == _first || _curr == _last)
            return _curr;
        else
            return swap_iters(_curr, std::prev(_curr));
    }

    template<class _BiIt>
    static _BiIt swap_iters(_BiIt _current, _BiIt _target)
    {
        std::iter_swap(_current, _target);
        return _target;
    }
};


