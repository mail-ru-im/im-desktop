#include "stdafx.h"
#include "strings.h"
#include "system.h"

#include <codecvt>
#include <locale>
#include <string>

namespace core
{
    namespace tools
    {
        std::string from_utf16(std::wstring_view _source_16)
        {
#ifdef __linux__
            return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(_source_16.data(), _source_16.data() + _source_16.size());
#else
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(_source_16.data(), _source_16.data() + _source_16.size());
#endif //__linux__
        }

        std::wstring from_utf8(std::string_view _source_8)
        {
#ifdef __linux__
            return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(_source_8.data(), _source_8.data() + _source_8.size());
#else
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().from_bytes(_source_8.data(), _source_8.data() + _source_8.size());
#endif //__linux__
        }

        bool is_digit(char _c)
        {
            return (_c >= '0' && _c <= '9');
        }

        bool is_latin(char _c)
        {
            return (_c >= 'a' && _c <= 'z') || (_c >= 'A' && _c <= 'Z');
        }

        bool is_phone(std::string_view _value)
        {
            if (_value.empty())
                return false;

            const auto iter = _value.front() == '+' ? std::next(_value.begin()) : _value.begin();

            return std::all_of(iter, _value.end(), [](auto c) { return is_digit(c); });
        }

        bool is_number(std::string_view _value)
        {
            return std::all_of(_value.begin(), _value.end(), [](auto c) { return is_digit(c); });
        }

        bool is_uin(std::string_view _value)
        {
            return is_number(_value);
        }

        bool is_email_sym(char _c)
        {
            if (_c < -1 || (!isalpha(_c) && !isdigit(_c) && _c != '_' && _c != '.' && _c != '-'))
                return false;

            return true;
        }

        int32_t is_email_domain(char _c)
        {
            return is_email_sym(_c);
        }


        bool is_email(std::string_view _value)
        {
            bool alpha = false;
            int32_t name = 0, domain = 0, dots = 0;

            for (auto c : _value)
            {
                if (c == '@')
                {
                    if (alpha)
                        return false;

                    alpha = true;
                }
                else if (!alpha)
                {
                    if (!is_email_sym(c))
                        return false;

                    ++name;
                }
                else
                {
                    if (!is_email_domain(c))
                        return false;

                    if (c == '.')
                        ++dots;

                    ++domain;
                }
            }

            if (alpha && name > 0 && domain - dots > 0 && dots > 0)
                return true;

            return false;
        }

#ifdef _WIN32
        std::wstring short_path_for(const std::wstring& filename)
        {
            std::array< wchar_t, MAX_PATH >  buffer;

            GetShortPathName(filename.c_str(), buffer.data(), buffer.size());
            return buffer.data();
        }
#endif // _WIN32

        std::string wstring_to_string(const std::wstring& _wstr)
        {
#if defined(__linux__)
            return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(_wstr);
#elif defined(__APPLE__)
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(_wstr);
#else
            return from_utf16(short_path_for(_wstr));
#endif //__linux__
        }

        std::string adler32(std::string_view _input)
        {
            int32_t d = 1;
            long long b = 0;
            int32_t e = 0;
            int32_t c = 0;
            int32_t j = 0;
            for (j = (int32_t)_input.size(); 0 <= j ? e < j : e > j; c = 0 <= j ? ++e : --e)
                c = _input[c], d += c, b += d;
            d %= 65521;
            b %= 65521;
            auto result = b * 65536 + d;
            return std::to_string(result);
        }

        bool contains_on_pos(const std::vector<std::vector<std::string>>& _patterns, const std::string_view _word, int32_t _pos, std::string& pattern, unsigned _offset)
        {
            auto current_symbol_i = 1u;
            if (current_symbol_i == _patterns.size())
                return true;

            while (current_symbol_i < _patterns.size())
            {
                auto found = false;
                for (auto patter_i = _offset; patter_i < _patterns[current_symbol_i].size(); ++patter_i)
                {
                    auto tt = _patterns[current_symbol_i][patter_i];
                    auto tt_size = tt.size();
                    auto b = _word.compare(_pos, tt_size, tt);

                    if (b == 0)
                    {
                        found = true;
                        pattern.append(tt);
                        _pos += tt_size;
                        ++current_symbol_i;
                        break;
                    }
                }

                if (current_symbol_i == _patterns.size())
                    return true;

                if (!found)
                    return false;
            }
            return false;
        }

        std::vector<std::string_view> get_words(std::string_view _word)
        {
            std::vector<std::string_view> words;

            constexpr auto spaces = std::string_view(" \n\t\r");
            _word.remove_prefix(std::min(_word.find_first_not_of(spaces), _word.size()));
            _word.remove_suffix(std::min(_word.size() - _word.find_last_not_of(spaces) - 1, _word.size()));
            if (!_word.empty())
            {
                size_t prev_pos = 0;
                auto pos = _word.find_first_of(spaces);
                while (pos != _word.npos)
                {
                    words.emplace_back(_word.substr(prev_pos, pos - prev_pos));
                    prev_pos = pos + 1;
                    if (prev_pos < _word.length() - 1)
                        prev_pos = _word.find_first_not_of(spaces, prev_pos);

                    pos = _word.find_first_of(spaces, prev_pos);
                }

                if (pos == _word.npos && _word.length() > prev_pos)
                    words.emplace_back(_word.substr(prev_pos, _word.length() - prev_pos));
            }

            return words;
        }

        std::pair<bool, std::string> contains(const std::vector<std::vector<std::string>>& _patterns, const std::string_view _word, unsigned fixed_patterns_count, int32_t& priority, bool starts_with)
        {
            priority = -1;

            if (_patterns.empty())
                return { true, std::string() };

            for (auto symb_i = fixed_patterns_count; symb_i < _patterns[0].size(); ++symb_i)
            {
                auto s = _patterns[0][symb_i];
                auto pos = _word.find(s, 0);
                if (starts_with && pos != 0)
                    continue;

                while (pos != std::string::npos && !s.empty())
                {
                    auto prev_pos = pos;
                    std::string pattern = s;
                    pos += s.size();
                    if (pos > _word.length() - 1)
                        break;

                    if (contains_on_pos(_patterns, _word, (int)pos, pattern, fixed_patterns_count))
                    {
                        priority = (_word.length() == pattern.length() || prev_pos == 0 || _word[prev_pos - 1] == ' ') ? 0 : 1;
                        return { true, pattern };
                    }

                    pos = _word.find(_patterns[0][symb_i], pos);
                }
            }

            return { false, std::string() };
        }

        int32_t get_char_index(const char* _str, int32_t& _length, const std::shared_ptr<int32_t>& _last_symb_id, std::string& _symbols
            , std::vector<int32_t>& _indexes, std::vector<std::pair<std::string, int32_t>>& _table)
        {
            _length = tools::utf8_char_size(*_str);

            std::string_view str(_str, _length);

            auto lower = system::to_lower(str);
            auto upper = system::to_upper(str);

            if (lower.empty() || (lower.size() == 1 && lower[0] == '\0'))
                lower = std::string(str);

            auto id = 0u;
            for (const auto& symb : _table)
            {
                if (symb.first == lower)
                    break;
                ++id;
            }

            _indexes.push_back((int)_symbols.size());
            _symbols.append(lower);

            _indexes.push_back((int)_symbols.size());
            _symbols.append(std::move(upper));

            auto result = -1;
            if (id < _table.size())
            {
                result = _table[id].second;
            }
            else
            {
                ++(*_last_symb_id);
                result = *_last_symb_id;
                _table.push_back(std::make_pair(std::move(lower), *_last_symb_id));
            }

            return result;
        }

        std::vector<int32_t> convert_string_to_vector(const std::string& _str, const std::shared_ptr<int32_t>& _last_symb_id
            , std::string& _symbols, std::vector<int32_t>& _indexes, std::vector<std::pair<std::string, int32_t>>& _table)
        {
            std::vector<int32_t> result;

            for (std::string::size_type i = 0; i < _str.size(); )
            {
                int32_t len = 0;
                auto id = get_char_index(_str.c_str() + i, len, _last_symb_id, _symbols, _indexes, _table);
                result.push_back(id);
                i += len;
            }

            return result;
        }
    }
}
