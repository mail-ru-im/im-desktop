#pragma once
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <string_view>
#include <vector>
#include <iterator>
#include <algorithm>

#include "../../common.shared/string_utils.h"

template<class _Char, class _OutIt>
static _OutIt read_lines(std::basic_istream<_Char>& in, _OutIt _out)
{
    using namespace std;

    basic_string<_Char> line;
    while (getline(in, line))
    {
        if (!line.empty())
        {
            *_out = line;
            ++_out;
        }
    }
    return _out;
}

template<class _Char, class _OutIt>
static _OutIt read_lines(const char* _filename, _OutIt _out)
{
    using namespace std;

    basic_string<_Char> line;
    basic_ifstream<_Char> ifs(_filename);
    return read_lines(ifs, _out);
}


template<class _Char, class _OutIt>
static _OutIt read_kvl(const char* _filename, _Char _delim, _OutIt _out)
{
    using namespace std;
    using string_type = basic_string<_Char>;
    using string_view_type = basic_string_view<_Char>;
    using string_vector = std::vector<basic_string<_Char>>;

    string_vector strings;
    string_type line;
    basic_ifstream<_Char> ifs(_filename);
    while(getline(ifs, line))
    {
        strings.clear();
        if (line.empty())
            continue;

        su::split((string_view_type)line, back_inserter(strings), _delim, su::token_compress::on);
        if (strings.size() == 0)
            continue;

        *_out = { strings.front(), string_vector{strings.begin() + 1, strings.end()} };
        ++_out;
    }
    return _out;
}

template<class _Char, class _Key, class _Value,  class _OutIt>
static _OutIt read_kvp(const char* _filename, _Char _delim, _OutIt _out)
{
    using namespace std;
    using string_type = basic_string<_Char>;
    using key_type = std::remove_const_t<_Key>;
    using val_type = std::remove_const_t<_Value>;

    string_type line;
    basic_istringstream<_Char> iss;
    basic_ifstream<_Char> ifs(_filename);
    while (getline(ifs, line))
    {
        if (line.empty())
            continue;

        auto pos = line.find(_delim);
        if (pos == string_type::npos)
            continue;

        auto key_str = line.substr(0, pos);
        auto val_str = line.substr(pos + 1);

        key_type key;
        iss.str(key_str);
        iss.seekg(0);
        iss >> key;

        val_type value;
        iss.str(val_str);
        iss.seekg(0);
        iss >> value;

        *_out = { key, value };
        ++_out;
    }
    return _out;
}

static inline size_t read_content(const char* _filename, std::string& _content)
{
    std::ifstream ifs(_filename, std::ios::in | std::ios::binary);
    ifs.seekg(0, std::ios::end);
    _content.resize(static_cast<size_t>(ifs.tellg()));
    ifs.seekg(0);
    ifs.read(reinterpret_cast<char*>(_content.data()), _content.size());
    size_t n = static_cast<size_t>(ifs.tellg());
    _content.resize(n);
    return n;
}

