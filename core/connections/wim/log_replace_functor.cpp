#include "stdafx.h"
#include "log_replace_functor.h"
#include "wim_packet.h"
#include "../../../common.shared/string_utils.h"

void core::log_replace_functor::add_marker(std::string_view _marker, ranges_evaluator _re)
{
    im_assert(!_marker.empty());
    if (!_marker.empty())
        markers_.push_back(std::make_pair(su::concat(_marker, '='), std::move(_re)));
}

void core::log_replace_functor::add_url_marker(std::string_view _marker, ranges_evaluator _re)
{
    im_assert(!_marker.empty());
    if (!_marker.empty())
        markers_.push_back(std::make_pair(std::string(_marker), std::move(_re)));
}

void core::log_replace_functor::add_json_marker(std::string_view _marker, ranges_evaluator _re)
{
    static const constexpr std::string_view json_delimeter = "\":\"";
    static const constexpr std::string_view json_delimeter_with_space = "\": \"";
    static const auto json_delimeter_encoded = wim::wim_packet::escape_symbols("\":\"");
    static const auto json_qoute_encoded = wim::wim_packet::escape_symbols("\"");

    markers_json_.push_back(std::make_pair(su::concat('"', _marker, json_delimeter), _re));
    markers_json_.push_back(std::make_pair(su::concat('"', _marker, json_delimeter_with_space), _re));
    markers_json_encoded_.push_back(std::make_pair(su::concat(json_qoute_encoded, _marker, json_delimeter_encoded), std::move(_re)));
}

void core::log_replace_functor::add_json_array_marker(std::string_view _marker, ranges_evaluator _re)
{
    static const constexpr std::string_view json_delimeter = "\":[";
    static const constexpr std::string_view json_delimeter_with_space = "\": [";
    static const auto json_delimeter_encoded = wim::wim_packet::escape_symbols(json_delimeter);
    static const auto json_delimeter_encoded_with_space = wim::wim_packet::escape_symbols(json_delimeter_with_space);
    static const auto json_qoute_encoded = wim::wim_packet::escape_symbols("\"");

    markers_json_.push_back(std::make_pair(su::concat('"', _marker, json_delimeter), _re));
    markers_json_.push_back(std::make_pair(su::concat('"', _marker, json_delimeter_with_space), _re));

    markers_json_encoded_.push_back(std::make_pair(su::concat(json_qoute_encoded, _marker, json_delimeter_encoded), _re));
    markers_json_encoded_.push_back(std::make_pair(su::concat(json_qoute_encoded, _marker, json_delimeter_encoded_with_space), std::move(_re)));
}

void core::log_replace_functor::add_message_markers()
{
    add_json_array_marker("snippets");
    add_json_marker("text");
    add_json_marker("message");
    add_json_marker("original_url", tail_from_last_range_evaluator('/'));
    add_json_marker("id");
    add_json_marker("preview_url");
    add_json_marker("url");
    add_json_marker("snippet");
    add_json_marker("title");
    add_json_marker("caption");
}

void core::log_replace_functor::operator()(tools::binary_stream& _bs) const
{
    const auto sz = _bs.available();
    auto data = _bs.get_data();
    if (!sz || !data)
        return;

    std::string json_value_end;
    std::string json_value_end_excluded;

    std::remove_const_t<decltype(sz)> i = 0;
    while (i < sz - 1) // -1 because there is no need to check last character
    {
        bool found = false;
        bool found_json = false;

        ranges_evaluator range_eval = nullptr;

        static const auto json_array_end_not_encoded = std::string("]");
        static const auto json_array_end_encoded = wim::wim_packet::escape_symbols(json_array_end_not_encoded);
        static const auto json_array_begin_encoded = wim::wim_packet::escape_symbols("[");

        for (const auto& [m, re] : markers_)
        {
            if (sz - i > m.size() && strncmp(data + i, m.c_str(), m.size()) == 0)
            {
                i += m.size();
                found = true;
                range_eval = re;
                break;
            }
        }
        if (!found)
        {
            for (const auto& [m, re] : markers_json_) // we have to check both escaped and not escaped json markers, because everything is possible
            {
                if (sz - i > m.size() && strncmp(data + i, m.c_str(), m.size()) == 0)
                {
                    i += m.size();
                    found_json = true;
                    range_eval = re;
                    if (m.back() == '[') // found array marker "...: ["
                    {
                        json_value_end = json_array_end_not_encoded;
                        json_value_end_excluded.clear();
                    }
                    else
                    {
                        static const auto json_value_end_not_encoded = std::string("\"");
                        static const auto json_value_end_not_encoded_excluded = std::string("\\\"");
                        json_value_end = json_value_end_not_encoded;
                        json_value_end_excluded = json_value_end_not_encoded_excluded;
                    }
                    break;
                }
            }
            if (!found_json)
            {
                for (const auto& [m, re] : markers_json_encoded_)
                {
                    if (sz - i > m.size() && strncmp(data + i, m.c_str(), m.size()) == 0)
                    {
                        i += m.size();
                        found_json = true;
                        range_eval = re;
                        static const auto json_value_end_encoded = wim::wim_packet::escape_symbols("\"");
                        static const auto json_value_end_encoded_excluded = wim::wim_packet::escape_symbols("\\\"");

                        if (m.back() == json_array_begin_encoded.back())
                        {
                            json_value_end = json_array_end_encoded;
                            json_value_end_excluded.clear();
                        }
                        else
                        {
                            json_value_end = json_value_end_encoded;
                            json_value_end_excluded = json_value_end_encoded_excluded;
                        }
                        break;
                    }
                }
            }
        }
        if (found || found_json)
        {
            if (range_eval)
            {
                const auto ranges = range_eval(std::string_view(data + i, sz - i));
                auto max_ii = i;
                for (auto [start, end] : ranges)
                {
                    auto ii = i + start;
                    end += i;
                    while (ii < sz && ii <= size_t(end))
                    {
                        mask_char(data + ii);
                        ++ii;
                    }
                    max_ii = std::max(ii, max_ii);
                }
            }
            else if (!found_json)
            {
                while (i < sz && *(data + i) != '&' && *(data + i) != ' ')
                {
                    mask_char(data + i);
                    ++i;
                }
            }
            else
            {
                bool skip_value_end = false;
                int count_to_skip = 0;

                const bool array_marker = json_value_end == json_array_end_not_encoded;
                int brace_counter = array_marker ? 1 : 0;
                bool ignore_next_quote = false;
                bool in_quotes = false;

                while (i <= (sz - json_value_end.size()) && !(!skip_value_end && strncmp(data + i, json_value_end.data(), json_value_end.size()) == 0))
                {
                    // check if there is excluded value end, for example we need to skip \" characters and don't treat " character as value end
                    if (count_to_skip > 0)
                    {
                        skip_value_end = --count_to_skip > 0;
                    }
                    else if (i <= (sz - json_value_end_excluded.size()) && strncmp(data + i, json_value_end_excluded.data(), json_value_end_excluded.size()) == 0)
                    {
                        count_to_skip = json_value_end_excluded.size() - 1;
                        skip_value_end = true;
                    }

                    if (array_marker)
                    {
                        if (*(data + i) == '\\' && *(data + i + 1) == '\"')
                            ignore_next_quote = true;
                        else if (*(data + i) == '\"')
                        {
                            if (!ignore_next_quote)
                            {
                                in_quotes = !in_quotes;
                                ++i;
                            }

                            ignore_next_quote = false;
                        }

                        if (!in_quotes)
                        {
                            if (*(data + i) == '[')
                                brace_counter++;
                            else if (*(data + i) == ']')
                                brace_counter--;

                            if (brace_counter == 0)
                            {
                                i += json_value_end.size();
                                break;
                            }
                        }
                        else
                        {
                            mask_char(data + i);
                        }
                    }
                    else
                    {
                        mask_char(data + i);
                    }

                    ++i;
                }
            }
        }
        else
        {
            ++i;
        }
    }
}

bool core::log_replace_functor::is_null() const
{
    return markers_.empty() && markers_json_.empty() && markers_json_encoded_.empty();
}

void core::log_replace_functor::mask_char(char* c)
{
    if (*c != ' ') // do not replace space character
        *c = '*';
}

core::log_replace_functor::ranges core::aimsid_range_evaluator::operator()(std::string_view s) const
{
    // 025.0643767406.2836549238:123456 -> 025.0643767406.**********:123456

    size_t point_count = 0;
    std::ptrdiff_t start = 0;
    std::ptrdiff_t end = 0;

    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '.')
        {
            ++point_count;
            if (point_count == 2)
                start = i + 1;
        }
        else if (point_count == 2 && (s[i] == ':' || s[i] == '%'))
        {
            end = i - 1;
            break;
        }
    }

    return { { start, end } };
}

std::pair<std::ptrdiff_t, std::ptrdiff_t> core::aimsid_single_range_evaluator::operator()(std::string_view s) const
{
    const auto ranges = aimsid_range_evaluator::operator()(s);
    return ranges.empty() ? std::make_pair(std::ptrdiff_t(0), std::ptrdiff_t(0)) : ranges.front();
}

core::log_replace_functor::ranges core::poll_responses_ranges_evaluator::operator()(std::string_view s) const
{
    // %22123%22%2C%22%D1%84%D1%83%22%2C%22bar%22%5D -> %22***%22%2C%22************%22%2C%22***%22%5D

    static const auto quote = wim::wim_packet::escape_symbols("\"");
    static const auto backslash = wim::wim_packet::escape_symbols("\\");
    static const auto braket_close = wim::wim_packet::escape_symbols("]");

    static const auto invalid_offset = std::ptrdiff_t(0);
    log_replace_functor::ranges result;

    enum state_enum {
        look_for_array_close_bracket,
        look_for_string_close_quote,
        look_for_what_is_escaped,
    } state = look_for_array_close_bracket;

    auto read_symbol = [end = s.cend()](auto& it)->std::string_view{
        im_assert(it != end);
        const auto step = *it == '%' ? 3 : 1;
        return std::string_view(&(*it), step);
    };

    const auto begin = s.cbegin();
    const auto end = s.cend();
    auto it = begin;
    while (it < end)
    {
        const auto symbol = read_symbol(it);
        it += symbol.size();

        if (look_for_array_close_bracket == state && symbol == braket_close)
        {
            break;
        }
        else if (look_for_array_close_bracket == state && symbol == quote)
        {
            result.emplace_back(std::distance(begin, it), invalid_offset);
            state = look_for_string_close_quote;
        }
        else if (look_for_string_close_quote == state && symbol == quote)
        {
            im_assert(!result.empty());
            if (result.empty())
                break;
            result.back().second = std::distance(begin, it - symbol.size()) - 1;
            state = look_for_array_close_bracket;
        }
        else if (look_for_string_close_quote == state && symbol == backslash)
        {
            state = look_for_what_is_escaped;
        }
        else if (look_for_what_is_escaped == state)
        {
            state = look_for_string_close_quote;
        }
    }

    return result;
}

core::log_replace_functor::ranges core::speechtotext_fileid_range_evaluator::operator()(std::string_view s) const
{
    // Z0201YB4Tty47Prq1f6hBN5f1ee20d1ac?aimsid=... -> *********************************?aimsid=...
    const auto questionIt = std::find(s.cbegin(), s.cend(), '?');
    return { { 0, std::distance(s.cbegin(), questionIt - 1) } };
}

core::tail_from_last_range_evaluator::tail_from_last_range_evaluator(const char _chr)
    : chr_(_chr)
{
}

core::log_replace_functor::ranges core::tail_from_last_range_evaluator::operator()(std::string_view _str) const
{
    std::ptrdiff_t start = 0;
    std::ptrdiff_t end = _str.size() - 1;

    for (size_t i = 0u; i < _str.size(); ++i)
    {
        if (_str[i] == chr_)
        {
            start = i + 1;
        }
        else if (std::isspace(static_cast<int>(_str[i])) || _str[i] == '"')
        {
            end = i - 1;
            break;
        }
    }

    return { { start, end } };
}

core::distance_range_evaluator::distance_range_evaluator(std::ptrdiff_t _distance)
    : distance_(_distance)
{
}

core::log_replace_functor::ranges core::distance_range_evaluator::operator()(std::string_view) const
{
    if (distance_ > 0)
        return { { 0, distance_ } };
    return { { distance_, 0 } };
}
