#pragma once
#include <string_view>

template<class _String>
struct is_mutable : std::true_type {};

template<class _Ch, class _Tr>
struct is_mutable<std::basic_string_view<_Ch, _Tr>> : std::false_type {};

template<class _String>
inline constexpr bool is_mutable_v = is_mutable<_String>::value;


template<class T>
struct is_string_type : std::false_type {};

template<class _Char, class _Traits>
struct is_string_type<std::basic_string_view<_Char, _Traits>> : std::true_type {};

template<class _Char, class _Traits, class _Alloc>
struct is_string_type<std::basic_string<_Char, _Traits, _Alloc>> : std::true_type {};

template<class T>
inline constexpr bool is_string_type_v = is_string_type<T>::value;
