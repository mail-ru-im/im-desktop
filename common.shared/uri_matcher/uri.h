#pragma once

#include <cstdint>

#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>

#include "string_traits.h"
#include "scheme_traits.h"
#include "casefolding.h"

enum component : uint8_t
{
    scheme_component = 0x01,
    username_component = 0x02,
    password_component = 0x04,
    userinfo_component = username_component | password_component,
    host_component = 0x08,
    port_component = 0x10,
    authority_component = userinfo_component | host_component | port_component,
    path_component = 0x20,
    query_component = 0x40,
    fragment_component = 0x80,
    fullurl = 0xff
};

enum class category_type
{
    undefined,
    image,
    video,
    filesharing,
    site,
    email,
    ftp,
    icqprotocol,
    profile
};

enum class mmedia_type
{
    unknown = -1,
    // video
    _3gp = 0,
    avi,
    mkv,
    mov,
    mpeg4,
    flv,
    webm,
    wmv,
    // images
    bmp,
    gif,
    jpeg,
    jpg,
    svg,
    png,
    tiff,

    max_media_type
};

enum error_code
{
    no_error,
    failed
};


template<class, class, class, class>
struct uri_traits;

template<class, class>
struct formatter;

template<class, class>
struct parser;


template<
    class _Char,
    class _Traits = uri_traits<_Char, std::char_traits<_Char>, formatter<_Char, std::char_traits<_Char>>, parser<_Char, std::char_traits<_Char>>>
>
class basic_uri_view : _Traits
{
    template<class _Int>
    using enable_if_signed_int_t = std::enable_if_t<std::is_integral_v<_Int>&& std::is_signed_v<_Int>, _Int>;
public:
    using string_view_type = std::basic_string_view<_Char, std::char_traits<_Char>>;
    using traits_type = _Traits;
    using parser_type = typename traits_type::parser_type;
    using formatter_type = typename traits_type::formatter_type;
    using scheme_traits = uri_scheme_traits<_Char>;

    friend _Traits;

    basic_uri_view()
        : scheme_(scheme_type::undefined)
        , port_(-1)
    { }

    explicit basic_uri_view(string_view_type _sv)
        : scheme_(scheme_type::undefined)
        , port_(-1)
    {
        assign(_sv);
    }

    basic_uri_view(const _Char* _url, size_t _length)
        : basic_uri_view(string_view_type{ _url, _length })
    {
    }

    basic_uri_view(scheme_type _scheme, const _Char* _url, size_t _length)
        : basic_uri_view(_scheme, string_view_type{ _url, _length })
    {
    }

    basic_uri_view(scheme_type _scheme, string_view_type sv)
        : scheme_(scheme_type::undefined)
        , port_(-1)
    {
        assign(_scheme, sv);
    }

    void assign(string_view_type sv)
    {
        assign(sv.begin(), sv.end());
    }

    void assign(scheme_type _scheme, string_view_type _uri)
    {
        static auto icasecomp = [](_Char c1, _Char c2) {
            return (tolowercase(c1) == tolowercase(c2));
        };

        clear();
        string_view_type sv = scheme_traits::signature(_scheme);
        if (sv.size() > _uri.size())
            return;

        if (!std::equal(sv.begin(), sv.end(), _uri.begin(), icasecomp))
            return;

        assign(_scheme, _uri.begin() + sv.size(), _uri.end());
        str_ = _uri.substr(sv.size());
    }

protected:
    template<class _RandIt>
    void assign(_RandIt _first, _RandIt _last)
    {
        traits_type::parse_from(_first, _last, *this);
    }

    template<class _RandIt>
    void assign(scheme_type _scheme, _RandIt _first, _RandIt _last)
    {
        traits_type::parse_from(_scheme, _first, _last, *this);
    }

public:
    string_view_type data() const { return str_; }

    size_t length(component c) const {
        return traits_type::size(*this, c);
    }

    bool empty() const
    {
        return (str_.empty() || (scheme_ == scheme_type::undefined && (port_ == -1) &&
            username_.empty() && password_.empty() &&
            host_.empty() && path_.empty() &&
            query_.empty() && fragment_.empty()));
    }

    template<class _Query>
    _Query query() const {
        _Query q;
        parser_type::parse_query(query_.begin(), query_.end(), q, _Char('='), _Char('&'));
        return query_;
    }

    scheme_type scheme() const { return scheme_; }
    string_view_type named_scheme() const { return scheme_traits::name(scheme_); }
    string_view_type username() const { return username_; }
    string_view_type password() const { return password_; }
    string_view_type host() const { return host_; }
    int32_t port() const { return port_; }

    string_view_type path() const { return path_; }
    string_view_type query() const { return query_; }
    string_view_type fragment() const { return fragment_; }

    string_view_type path_suffix() const
    {
        if (path_.empty()) return string_view_type{};
        const auto pos = path_.rfind(_Char('.'));
        return (pos == string_view_type::npos ? string_view_type{} : path_.substr(pos+1));
    }

    bool has_query() const { return !query_.empty(); }
    bool has_fragment() const { return !fragment_.empty(); }

    category_type category() const { return traits_type::get_category(*this); }
    mmedia_type media_type() const { return traits_type::get_media_type(*this); }

    void clear()
    {
        str_ = {};
        scheme_ = scheme_type::undefined;
        port_ = -1;
        query_ = {};
        username_ = {};
        password_ = {};
        host_ = {};
        path_ = {};
        fragment_ = {};
    }

    template<class _String>
    _String to_string(_Char delim = _Char('='), _Char sep = _Char('&')) const
    {
        _String result;
        result.reserve(traits_type::size(*this, fullurl));
        format(std::back_inserter(result), fullurl, delim, sep);
        return result;
    }

    template<class _OutIt>
    _OutIt format(_OutIt out, component c, _Char delim = _Char('='), _Char sep = _Char('&')) const
    {
        return formatter_type::format_component(out, *this, c, delim, sep);
    }

    friend std::basic_ostream<_Char, _Traits>& operator<<(std::basic_ostream<_Char, _Traits>& out, const basic_uri_view& u)
    {
        using iter_type = std::ostreambuf_iterator<_Char, _Traits>;
        u.format(iter_type{ out });
        return out;
    }

private:
    string_view_type str_;
    string_view_type username_;
    string_view_type password_;
    string_view_type host_;
    string_view_type path_;
    string_view_type query_;
    string_view_type fragment_;
    scheme_type scheme_;
    int32_t port_;
};

template<class _Char>
basic_uri_view(const std::basic_string<_Char>&) -> basic_uri_view<_Char>;

template<
    class _Char,
    class _Traits = std::char_traits<_Char>
>
struct formatter
{
    using string_view_type = std::basic_string_view<_Char, _Traits>;
    using scheme_traits = uri_scheme_traits<_Char>;

    template<class _OutIt, class _Uri>
    static _OutIt format_component(_OutIt out, const _Uri& u, uint8_t c, _Char delim, _Char sep)
    {
        switch (c)
        {
        case scheme_component:
            if (u.scheme() == scheme_type::undefined)
                out = do_format(out, scheme_traits::signature(scheme_type::http));
            else
                out = do_format(out, scheme_traits::signature(u.scheme()));
            break;
        case username_component:
            out = do_format(out, u.username());
            break;
        case password_component:
            out = do_format(out, u.password());
            break;
        case userinfo_component:
            if (!u.username().empty())
            {
                out = format_component(out, u, username_component, delim, sep);
                if (!u.password().empty())
                {
                    out = do_format(out, _Char(':'));
                    out = format_component(out, u, password_component, delim, sep);
                }
                out = do_format(out, _Char('@')); // output '@' only if we have a username
            }
            break;
        case host_component:
            out = do_format(out, u.host());
            break;
        case port_component:
            out = do_format(out, u.port());
            break;
        case authority_component:
            out = format_component(out, u, userinfo_component, delim, sep);
            if (!u.host().empty())
            {
                out = format_component(out, u, host_component, delim, sep);
            }
            if (u.port() != -1)
            {
                out = do_format(out, _Char(':'));
                out = format_component(out, u, port_component, delim, sep);
            }
            break;
        case path_component:
            out = do_format(out, u.path());
            break;
        case query_component:
            out = do_format(out, u.query(), delim, sep);
            break;
        case fragment_component:
            out = do_format(out, u.fragment());
            break;
        case fullurl:
            out = format_component(out, u, scheme_component, delim, sep);
            out = format_component(out, u, authority_component, delim, sep);
            out = format_component(out, u, path_component, delim, sep);
            if (u.has_query())
            {
                out = do_format(out, _Char('?'));
                out = format_component(out, u, query_component, delim, sep);
            }
            if (u.has_fragment())
            {
                out = do_format(out, _Char('#'));
                out = format_component(out, u, fragment_component, delim, sep);
            }
            break;
        default:
            break;
        }
        return out;
    }

    template<class _OutIt>
    static _OutIt do_format(_OutIt out, _Char c)
    {
        *out = c;
        return ++out;
    }

    template<class _OutIt, class _String>
    static _OutIt do_format(_OutIt out, const _String& s)
    {
        return std::copy(s.begin(), s.end(), out);
    }

    template<class _OutIt>
    static _OutIt do_format(_OutIt out, int32_t port)
    {
        if (port < 0)
            return out;

        std::string s = std::to_string(port);
        return std::copy(s.begin(), s.end(), out);
    }

    template<class _OutIt, class _Query>
    static _OutIt do_format(_OutIt out, const _Query& q, _Char delim, _Char sep)
    {
        if (q.empty())
            return out;

        if constexpr (is_string_type_v<_Query>) {
            (void)delim;
            (void)sep;
            return std::copy(q.begin(), q.end(), out);
        }
        else
        {
            for (auto it = q.begin(), last = q.end() - 1; it != q.end(); ++it)
            {
                if (it->empty())
                    continue;

                out = do_format(out, *it, delim);
                if (it != last)
                    out = do_format(out, sep);
            }
            return out;
        }
    }

    template<class _OutIt, class _Item>
    static _OutIt do_format(_OutIt out, const _Item& i, _Char delim)
    {
        if (i.empty())
            return out;

        out = do_format(out, i.key());
        out = do_format(out, delim);
        out = do_format(out, i.value());
        return out;
    }
};

template<class _Char, class _Traits>
struct parser
{
    using string_view_type = std::basic_string_view<_Char, _Traits>;

    template<_Char lo, _Char hi>
    static bool in_range(_Char c) { return (c >= lo && c <= hi); }

    template<class _RandIt>
    static void assign(_RandIt first, _RandIt last, string_view_type& s)
    {
        size_t n = std::distance(first, last);
        if (n != 0)
            s = { std::addressof(*first), n };
        else
            s = {};
    }

    template<class _RandIt, class _String>
    static void assign(_RandIt first, _RandIt last, _String& s)
    {
        s = { first, last };
    }

    static void clear(int32_t& port)
    {
        port = -1;
    }

    static void clear(string_view_type& s)
    {
        s = { };
    }

    template<class _String>
    static void clear(_String& s)
    {
        s.clear();
    }

    template<class _RandIt>
    static bool is_numeric(_RandIt first, _RandIt last)
    {
        return std::all_of(first, last, [](auto c) { return in_range<_Char('0'), _Char('9')>(c); });
    }

    template<class _RandIt>
    static error_code parse_scheme(_RandIt first, _RandIt last, scheme_type& scheme)
    {
        error_code ec = failed;

        scheme = undefined;
        if (first == last)
            return ec;

        ec = no_error;

        // validate and parse
        _RandIt lcIt = first;
        for (auto i = first; i != last; ++i)
        {
            _Char c = *i;
            if (in_range<_Char('a'), _Char('z')>(c))
                continue;
            if (in_range<_Char('A'), _Char('Z')>(c))
            {
                lcIt = i;
                continue;
            }

            if (i != first)
            {
                if (in_range<_Char('0'), _Char('9')>(c))
                    continue;
                if (c == '+' || c == '-' || c == '.')
                    continue;
            }
            // found something else: try to recover it later
            return ec;
        }

        std::basic_string<_Char> buf(first, last);
        auto n = std::distance(first, lcIt);
        std::transform(buf.begin(), buf.begin() + n, buf.begin(), [](auto c) { return tolowercase(c); });
        scheme = uri_scheme_traits<_Char>::value((string_view_type)buf);
        return ec;
    }

    template<class _RandIt, class _String>
    static error_code parse_username(_RandIt first, _RandIt last, _String& username)
    {
        assign(first, last, username);
        return no_error;
    }

    template<class _RandIt, class _String>
    static error_code parse_password(_RandIt first, _RandIt last, _String& password)
    {
        assign(first, last, password);
        return no_error;
    }

    template<class _RandIt, class _String>
    static error_code parse_host(_RandIt first, _RandIt last, _String& host)
    {
        assign(first, last, host);
        return no_error;
    }

    template<class _RandIt>
    static error_code parse_port(_RandIt first, _RandIt last, int32_t& port)
    {
        const auto n = std::distance(first, last);
        if (n > 5 || n < 1) // maximum 5 chars allowed
            return failed;

        if (!is_numeric(first, last))
            return failed;

        std::basic_string<_Char, _Traits> buf(first, last);
        port = std::stoi(buf, nullptr, 10);
        return no_error;
    }


    template<class _RandIt, class _String>
    static error_code parse_path(_RandIt first, _RandIt last, _String& path)
    {
        assign(first, last, path);
        return no_error;
    }

    template<class _RandIt, class _Query>
    static error_code parse_query(_RandIt first, _RandIt last, _Query& q, _Char delim, _Char sep)
    {
        if constexpr (is_string_type_v<_Query>)
        {
            (void)delim;
            (void)sep;
            assign(first, last, q);
        }
        else
        {
            clear(q);

            _RandIt pos = first;
            for (; first != last; )
            {
                _RandIt begin = pos;
                _RandIt delimiter = last;
                while (pos != last)
                {
                    // scan for the component parts of this pair
                    if (delimiter == last && *pos == delim)
                        delimiter = pos;
                    if (*pos == sep)
                        break;
                    ++pos;
                }
                if (delimiter == last)
                    delimiter = pos;

                if (begin == last)
                    break;

                if (delimiter == pos) {
                    q.emplace(string_view_type{ std::addressof(*begin), size_t(delimiter - begin) }, string_view_type{});
                }
                else if (delimiter + 1 == pos) {
                    q.emplace(string_view_type{ std::addressof(*begin), size_t(delimiter - begin) }, string_view_type{});
                }
                else {
                    q.emplace(string_view_type{ std::addressof(*begin), size_t(delimiter - begin) },
                        string_view_type{ std::addressof(*(delimiter + 1)), size_t(pos - delimiter - 1) });
                }

                if (pos != last)
                    ++pos;
            }
        }
        return no_error;
    }

    template<class _RandIt, class _String>
    static error_code parse_userinfo(_RandIt first, _RandIt last, _String& username, _String& password)
    {
        auto pos = std::find(first, last, _Char(':'));
        if (pos != last)
        {
            parse_username(first, pos, username);
            parse_password(pos + 1, last, password);
        }
        else
        {
            parse_username(first, last, username);
        }
        return no_error;
    }

    template<class _RandIt, class _String, class _PortRep>
    static error_code parse_authority(_RandIt first, _RandIt last, _String& username, _String& password, _String& host, _PortRep& port)
    {
        auto pos = std::find(first, last, _Char('@'));
        if (pos != last)
        {
            parse_userinfo(first, pos, username, password);
            first = pos + 1;
        }

        pos = std::find(first, last, _Char(':'));
        if (pos != last)
        {
            parse_host(first, pos, host);
            parse_port(pos + 1, last, port);
        }
        else
        {
            parse_host(first, last, host);
        }

        return no_error;
    }

    template<class _RandIt, class _String>
    static error_code parse_fragment(_RandIt first, _RandIt last, _String& fragment)
    {
        assign(first, last, fragment);
        return no_error;
    }
};

template<
    class _Char,
    class _Traits = std::char_traits<_Char>,
    class _Formatter = formatter<_Char, _Traits>,
    class _Parser = parser<_Char, _Traits>
>
struct uri_traits : _Traits
{
    using formatter_type = _Formatter;
    using parser_type = _Parser;
    using string_view_type = std::basic_string_view<_Char, _Traits>;

    template<class _OutIt, class _Uri>
    static _OutIt format_to(_OutIt out, const _Uri& u, _Char delim, _Char sep)
    {
        return formatter_type::format_component(out, u, fullurl, delim, sep);
    }

    template<class _RandIt, class _Uri>
    static void parse_from(scheme_type scheme, _RandIt first, _RandIt last, _Uri& u, _Char delim = _Char('='), _Char sep = _Char('&'))
    {
        static auto is_delimiter = [](_Char c) { return (c == _Char('/') || c == _Char('#') || c == _Char('?')); };
        u.clear();

        u.scheme_ = scheme;

        if (first == last)
            return;

        _RandIt pos = std::find_if(first, last, is_delimiter);
        _Parser::parse_authority(first, pos, u.username_, u.password_, u.host_, u.port_);
        first = pos;
        if (first == last)
            return;

        pos = std::find_if(first + 1, last, [](_Char c) { return c == _Char('#') || c == _Char('?'); });

        if (*first == _Char('/'))
        {
            _Parser::parse_path(first, pos, u.path_);
            first = pos;
        }

        if (first == last)
            return;

        if (*first == _Char('#'))
        {
            _Parser::parse_fragment(++first, last, u.fragment_);
            return;
        }

        if (*first == _Char('?'))
        {
            pos = std::find(++first, last, _Char('#'));
            _Parser::parse_query(first, pos, u.query_, delim, sep);
        }

        first = pos;
        if (first == last)
            return;

        _Parser::parse_fragment(++first, last, u.fragment_);
    }

    template<class _RandIt, class _Uri>
    static void parse_from(_RandIt first, _RandIt last, _Uri& u, _Char delim = _Char('='), _Char sep = _Char('&'))
    {
        using uint = unsigned int;
        u.clear();

        if (first == last)
            return;

        _RandIt colon = last;
        _RandIt question = last;
        _RandIt hash = last;

        for (auto i = first; i != last; ++i)
        {
            uint uc = *i;
            if (uc == '#' && hash == last)
            {
                hash = i;
                break; // nothing more to be found
            }
            if (question == last)
            {
                if (uc == ':' && colon == last)
                    colon = i;
                else if (uc == '?')
                    question = i;
            }
        }

        auto len = std::distance(first, last);
        auto qpos = std::distance(first, question);
        auto hpos = std::distance(first, hash);

        _RandIt hierStart = last;
        if (colon != last && _Parser::parse_scheme(first, colon, u.scheme_) == no_error)
        {
            hierStart = colon + 1;
        }
        else
        {
            // recover from failed scheme: it might not have been a schemen at all
            u.scheme_ = undefined;
            hierStart = first;
        }
        _RandIt pathStart;
        _RandIt hierEnd = first + std::min(std::min(qpos, hpos), len);
        if (std::distance(hierStart, hierEnd) >= 2 && *hierStart == _Char('/') && (*(hierStart + 1) == _Char('/')))
        {
            // we have an authority, it ends at the first slash after these
            _RandIt authorityEnd = hierEnd;
            for (auto i = hierStart + 2; i < authorityEnd; ++i)
            {
                if (*i == _Char('/'))
                {
                    authorityEnd = i;
                    break;
                }
            }

            _Parser::parse_authority(hierStart + 2, authorityEnd, u.username_, u.password_, u.host_, u.port_);
            // even if we failed to set the authority properly, let's try to recover
            pathStart = authorityEnd;
            _Parser::parse_path(pathStart, hierEnd, u.path_);
        }
        else
        {
            _Parser::clear(u.username_);
            _Parser::clear(u.password_);
            _Parser::clear(u.host_);
            _Parser::clear(u.port_);
            pathStart = hierStart;
            if (hierStart < hierEnd)
                return parse_from(u.scheme_, u.scheme_ != scheme_type::undefined ? pathStart : first, last, u, delim, sep);
            else
                _Parser::clear(u.path_);
        }

        if (question < hash)
            _Parser::parse_query(question + 1, first + std::min(hpos, len), u.query_, delim, sep);

        if (hash != last)
            _Parser::parse_fragment(hash + 1, last, u.fragment_);
    }

    template<class _Uri>
    static size_t size(const _Uri& u, component c)
    {
        size_t len = 0;
        switch (c)
        {
        case scheme_component:
            len = u.named_scheme().size();
            break;
        case username_component:
            len = u.username().size();
            break;
        case password_component:
            len = u.password().size();
            break;
        case userinfo_component:
            len = (size(u, username_component) + size(u, password_component) + 1);
            break;
        case host_component:
            len = u.host().size();
            break;
        case port_component:
            len = u.port() < 0 ? 0 : 32;
            break;
        case authority_component:
            len = (size(u, userinfo_component) + size(u, host_component) + size(u, port_component) + 1);
            break;
        case path_component:
            len = u.path().size();
            break;
        case query_component:
            len = u.query().length();
            break;
        case fragment_component:
            len = u.fragment().size();
            break;
        case fullurl:
            len = size(u, scheme_component) + size(u, authority_component) + 1 + size(u, path_component);
            if (u.has_query())
                len += size(u, query_component) + 1;
            if (u.has_fragment())
                len += size(u, fragment_component) + 1;
            break;
        default:
            break;
        }
        return len;
    }
};

template<class T>
struct is_uri_type : std::false_type {};

template<class _Ch, class _Tr>
struct is_uri_type<basic_uri_view<_Ch, _Tr>> : std::true_type {};

template<class T>
inline constexpr bool is_uri_type_v = is_uri_type<T>::value;

