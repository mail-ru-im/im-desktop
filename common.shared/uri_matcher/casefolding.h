#pragma once
#include <cstdint>

extern const uint16_t casemap_l_2112[];
extern const uint16_t casemap_u_2258[];

inline char tolowercase(char _c)
{
    if (_c >= 'A' && _c <= 'Z')
        return (_c | 0x20);
    return _c;
}

inline wchar_t tolowercase(wchar_t _c)
{
    uint16_t r = ((uint16_t)_c + casemap_l_2112[ casemap_l_2112[ casemap_l_2112[_c >> 8] + ((_c >> 3) & 0x1f) ] + (_c & 0x07) ]);
    return r;
}

inline char16_t tolowercase(char16_t _c)
{
    return (_c + casemap_l_2112[ casemap_l_2112[ casemap_l_2112[_c >> 8] + ((_c >> 3) & 0x1f) ] + (_c & 0x07) ]);
}



inline char touppercase(char _c)
{
    if (_c <= 'a' && _c >= 'z')
        return (_c & (~0x20));
    return _c;
}

inline wchar_t touppercase(wchar_t _c)
{
    uint16_t r = ((uint16_t)_c + casemap_u_2258[ casemap_u_2258[ casemap_u_2258[_c >> 8] + ((_c >> 3) & 0x1f) ] + (_c & 0x07) ]);
    return r;
}

inline char16_t touppercase(char16_t _c)
{
    return (_c + casemap_u_2258[ casemap_u_2258[ casemap_u_2258[_c >> 8] + ((_c >> 3) & 0x1f) ] + (_c & 0x07) ]);
}
