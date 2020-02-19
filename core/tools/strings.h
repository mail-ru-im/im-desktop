#ifndef __STRINGS_H_
#define __STRINGS_H_

#pragma once

namespace core
{
    namespace tools
    {
        std::string from_utf16(std::wstring_view _source_16);
        std::wstring from_utf8(std::string_view _source_8);
        inline std::string from_int64(const int64_t _val) { return std::to_string(_val); }
        std::string wstring_to_string(const std::wstring& wstr);

        bool is_digit(char _c);
        bool is_latin(char _c);

        template<class t_, class u_>
        t_ trim_right(const t_& _str, u_ _sym)
        {
            const auto endpos = _str.find_last_not_of(_sym);
            if (t_::npos != endpos)
                return _str.substr(0, endpos + 1);

            return _str;
        }

        template<class t_, class u_>
        t_ trim_left(const t_& _str, u_ _sym)
        {
            const auto startpos = _str.find_first_not_of(_sym);
            if (t_::npos != startpos)
                return _str.substr(startpos);

            return _str;
        }

        bool is_number(const std::string_view _value);
        bool is_uin(const std::string_view _value);
        bool is_email(const std::string_view _value);
        bool is_phone(const std::string_view _value);

        std::vector<std::string> to_array(const std::string &text, const std::string &separator);

        std::string adler32(const std::string& _input);

        template <class T>
        uint64_t to_uint64(const T& value, uint64_t _default_value = 0)
        {
            try
            {
                return stoull(value);
            }
            catch (...)
            {
                return _default_value;
            }
        }

        std::vector<std::string_view> get_words(std::string_view _word);
        std::pair<bool, std::string> contains(const std::vector<std::vector<std::string>>& _patterns, const std::string_view _word, unsigned fixed_patterns_count, int32_t& priority, bool starts_with = false);

        inline int32_t utf8_char_size(char _s)
        {
            const unsigned char b = *reinterpret_cast<unsigned char*>(&_s);

            if ((b & 0xE0) == 0xC0) return 2;
            if ((b & 0xF0) == 0xE0) return 3;
            if ((b & 0xF8) == 0xF0) return 4;
            if ((b & 0xFC) == 0xF8) return 5;
            if ((b & 0xFE) == 0xFC) return 6;

            return 1;
        }

        template<typename T>
        std::vector<T> build_prefix(const std::vector<T>& _term)
        {
            std::vector<T> prefix(_term.size());
            for (size_t i = 1; i < _term.size(); ++i)
            {
                auto j = prefix[i - 1];
                while (j > 0 && _term[j] != _term[i])
                    j = prefix[j - 1];
                if (_term[j] == _term[i])
                    j += 1;
                prefix[i] = j;
            }
            return prefix;
        }

        std::vector<int32_t> convert_string_to_vector(const std::string& _str, std::shared_ptr<int32_t> _last_symb_id
            , std::string& _symbols, std::vector<int32_t>& _indexes, std::vector<std::pair<std::string, int32_t>>& _table);
    }
}

#endif //__STRINGS_H_
