#include "stdafx.h"

#ifdef __APPLE__
    #include "../../core/tools/strings.h"
#else
    #include "../core/tools/strings.h"
#endif

#include "url_parser.h"
#include "../config/config.h"

#include <cctype>

namespace
{
    #include "domain_parser.in"

    constexpr std::string_view ICQ_COM = "icq.com/files/";
    static constexpr int ICQ_COM_SAFE_POS = std::string_view("icq.com").size() - 1;

    constexpr std::string_view CHAT_MY_COM = "chat.my.com/files/";
    static constexpr int CHAT_MY_COM_SAFE_POS = std::string_view("chat.my.com").size() - 1;

    constexpr const char* LEFT_DOUBLE_QUOTE = "«";
    constexpr const char* RIGHT_DOUBLE_QUOTE = "»";

    constexpr const char NBSP_FIRST = static_cast<char>(0xc2);
    constexpr const char NBSP_SECOND = static_cast<char>(0xa0);

    constexpr const char* LEFT_QUOTE = "‘";
    constexpr const char* RIGHT_QUOTE = "’";
}

const char* to_string(common::tools::url::type _value)
{
    switch (_value)
    {
    case common::tools::url::type::undefined:   return "undefined";
    case common::tools::url::type::image:       return "image";
    case common::tools::url::type::video:       return "video";
    case common::tools::url::type::filesharing: return "filesharing";
    case common::tools::url::type::site:        return "site";
    case common::tools::url::type::email:       return "email";
    case common::tools::url::type::ftp:         return "ftp";
    default:                                    return "#unknown";
    }
}

const char* to_string(common::tools::url::protocol _value)
{
    switch (_value)
    {
    case common::tools::url::protocol::undefined:   return "undefined";
    case common::tools::url::protocol::http:        return "http";
    case common::tools::url::protocol::ftp:         return "ftp";
    case common::tools::url::protocol::https:       return "https";
    case common::tools::url::protocol::ftps:        return "ftps";
    case common::tools::url::protocol::sftp:        return "sftp";
    case common::tools::url::protocol::rdp:         return "rdp";
    case common::tools::url::protocol::vnc:         return "vnc";
    case common::tools::url::protocol::ssh:         return "ssh";
    case common::tools::url::protocol::icq:         return "icq";
    case common::tools::url::protocol::agent:       return "magent";
    case common::tools::url::protocol::biz:         return "myteam-messenger";
    case common::tools::url::protocol::dit:         return "itd-messenger";
    default:                                        return "#unknown";
    }
}

const char* to_string(common::tools::url::extension _value)
{
    switch (_value)
    {
    case common::tools::url::extension::undefined:  return "undefined";
    case common::tools::url::extension::avi:        return "avi";
    case common::tools::url::extension::bmp:        return "bmp";
    case common::tools::url::extension::flv:        return "flv";
    case common::tools::url::extension::gif:        return "gif";
    case common::tools::url::extension::jpeg:       return "jpeg";
    case common::tools::url::extension::jpg:        return "jpg";
    case common::tools::url::extension::mkv:        return "mkv";
    case common::tools::url::extension::mov:        return "mov";
    case common::tools::url::extension::mpeg4:      return "mpeg4";
    case common::tools::url::extension::png:        return "png";
    case common::tools::url::extension::tiff:       return "tiff";
    case common::tools::url::extension::webm:       return "webm";
    case common::tools::url::extension::wmv:        return "wmv";
    case common::tools::url::extension::_3gp:       return "3gp";
    case common::tools::url::extension::svg:        return "svg";
    default:                                        return "#unknown";
    }
}

common::tools::url::url() = default;

common::tools::url::url(std::string _url, type _type, protocol _protocol, extension _extension)
    : type_(_type)
    , protocol_(_protocol)
    , extension_(_extension)
    , url_(_url)
    , original_(std::move(_url))
{
}

common::tools::url::url(std::string _original, std::string _url, type _type, protocol _protocol, extension _extension)
    : type_(_type)
    , protocol_(_protocol)
    , extension_(_extension)
    , url_(std::move(_url))
    , original_(std::move(_original))
{
}

bool common::tools::url::is_filesharing() const
{
    return type_ == type::filesharing;
}

bool common::tools::url::is_image() const
{
    return type_ == type::image;
}

bool common::tools::url::is_video() const
{
    return type_ == type::video;
}

bool common::tools::url::is_site() const
{
    return type_ == type::site;
}

bool common::tools::url::is_email() const
{
    return type_ == type::email;
}

bool common::tools::url::is_ftp() const
{
    return type_ == type::ftp;
}

bool common::tools::url::has_prtocol() const
{
    constexpr auto http = std::string_view("http");
    if (original_.size() < http.size())
        return false;

    const auto prefix = std::string_view(original_.data(), http.size());

    return std::equal(prefix.begin(), prefix.end(), http.begin(), http.end(), [](unsigned char a, unsigned char b)
    {
        return std::tolower(a) == std::tolower(b);
    });
}

bool common::tools::url::operator==(const url& _right) const
{
    return type_ == _right.type_
        && protocol_ == _right.protocol_
        && extension_ == _right.extension_
        && url_ == _right.url_;
}

bool common::tools::url::operator!=(const url& _right) const
{
    return !(*this == _right);
}

std::ostream& operator<<(std::ostream& _out, const common::tools::url& _url)
{
    _out << "['"
        << _url.url_
        << "', "
        << to_string(_url.type_)
        << ", "
        << to_string(_url.protocol_)
        << ", "
        << to_string(_url.extension_)
        << ']';
    return _out;
}

static auto make_vector(std::string_view _files_url, std::vector<std::string>&& _files_urls)
{
    std::vector<std::string> res(std::move(_files_urls));
    res.push_back(std::string(_files_url));
    return res;
}

common::tools::url_parser::url_parser(std::string_view _files_url)
    : url_parser(std::vector<std::string>{ std::string(_files_url) })
{
}

common::tools::url_parser::url_parser(std::string_view _files_url, std::vector<std::string> _files_urls)
    : url_parser(make_vector(_files_url, std::move(_files_urls)))
{
}

common::tools::url_parser::url_parser(std::vector<std::string> _files_urls)
    : files_urls_(std::move(_files_urls))
{
    reset();
    init_fixed_urls_compare();
}

void common::tools::url_parser::process(char c)
{
    char_buf_[char_pos_] = c;

    if (char_pos_ > 0)
    {
        ++char_pos_;

        if (char_pos_ < char_size_)
            return;

        char_pos_ = 0;
        process();
        return;
    }

    char_size_ = core::tools::utf8_char_size(c);

    is_utf8_ = char_size_ > 1;

    if (!is_utf8_)
        process();
    else
        ++char_pos_;
}

bool common::tools::url_parser::has_url() const
{
    return url_.type_ != url::type::undefined;
}

const common::tools::url& common::tools::url_parser::get_url() const
{
    return url_;
}

bool common::tools::url_parser::skipping_chars() const
{
    return state_ == states::lookup;
}

int32_t common::tools::url_parser::raw_url_length() const
{
    return buf_.size();
}

int32_t common::tools::url_parser::tail_size() const
{
    return tail_size_;
}

void common::tools::url_parser::finish()
{
    save_url();
}

common::tools::url_vector_t common::tools::url_parser::parse_urls(const std::string& _source, const std::string& _files_url)
{
    url_vector_t urls;

    url_parser parser(_files_url);

    for (char c : _source)
    {
        parser.process(c);
        if (parser.has_url())
        {
            urls.push_back(parser.get_url());
            parser.reset();
        }
    }

    parser.finish();
    if (parser.has_url())
        urls.push_back(parser.get_url());

    return urls;
}

void common::tools::url_parser::add_fixed_urls(std::vector<compare_item>&& _items)
{
    for (auto&& item : _items)
    {
        if (item.str.empty())
            continue;

        compare_set_.insert(item.str[0]);
        fixed_urls_.push_back(std::move(item));
    }
}

static bool is_black_list_email_host(std::string_view _s)
{
    return std::any_of(std::begin(_s), std::end(_s), [](auto c) { return c == ']' || c == '[' || c == '}' || c == '{'; });
}

#ifdef URL_PARSER_RESET_PARSER
#error Rename the macros
#endif
#define URL_PARSER_RESET_PARSER { reset(); return; }

#ifdef URL_PARSER_URI_FOUND
#error Rename the macros
#endif
#define URL_PARSER_URI_FOUND { if (!save_url()) reset(); return; }

#ifdef URL_PARSER_SET_STATE_AND_REPROCESS
#error Rename the macros
#endif
#define URL_PARSER_SET_STATE_AND_REPROCESS(X) { assert(state_ != (X)); if (state_ != (X)) { state_ = (X); process(); return; } }

static bool is_supported_scheme(std::string_view scheme)
{
    const static auto schemes = []() {
        const auto scheme_csv = config::get().string(config::values::register_url_scheme_csv); //url1,descr1,url2,desc2,..
        auto splitted = common::utils::splitSV(scheme_csv, ',');
        size_t i = 0;
        splitted.erase(std::remove_if(splitted.begin(), splitted.end(), [&i](const auto&) { return i % 2 != 0; }), splitted.end());
        return splitted;
    }();
    return std::any_of(schemes.begin(), schemes.end(), [scheme](auto x) { return x == scheme; });
}

void common::tools::url_parser::process()
{
    const char c = char_buf_[0];
    switch (state_)
    {
    case states::lookup:
        if (compare_set_.count(c)) { start_fixed_urls_compare(states::lookup_without_fixed_urls); return; }
        [[fallthrough]];
    case states::lookup_without_fixed_urls:
        if (c == 'w' || c == 'W') { state_ = states::www_2; break; }
        else if (c == 'h' || c == 'H') { protocol_ = url::protocol::http; state_ = states::protocol_t1; break; }
        else if (c == 'f' || c == 'F') { protocol_ = url::protocol::ftp; state_ = states::protocol_t1; break; }
        else if (c == 's' || c == 'S') { protocol_ = url::protocol::sftp; state_ = states::protocol_t1; break; }
        else if (c == 'v' || c == 'V') { protocol_ = url::protocol::vnc; state_ = states::protocol_t1; break; }
        else if (c == 'r' || c == 'R') { protocol_ = url::protocol::rdp; state_ = states::protocol_t1; break; }
        else if (c == 'i' || c == 'I') { protocol_ = url::protocol::icq; state_ = states::protocol_t1; break; }
        else if (c == 'm' || c == 'M') { protocol_ = url::protocol::agent; state_ = states::protocol_t1; break; }
        else if (c == LEFT_DOUBLE_QUOTE[0] && char_size_ == 2 && char_buf_[1] == LEFT_DOUBLE_QUOTE[1]) return;
        else if (c == LEFT_QUOTE[0] && char_size_ == 3 && char_buf_[1] == LEFT_QUOTE[1] && char_buf_[2] == LEFT_QUOTE[2]) return;
        else if (is_letter_or_digit(c, is_utf8_)) { save_char_buf(domain_); state_ = states::host; break; }
        return;
    case states::protocol_t1:
        if (c == 't' || c == 'T') state_ = protocol_ == url::protocol::ftp ? states::protocol_p : states::protocol_t2;
        else if (c == 'd' || c == 'D') state_ = states::protocol_d;
        else if (c == 'f' || c == 'F') state_ = states::protocol_t2;
        else if (c == 'n' || c == 'N') state_ = states::protocol_n;
        else if (c == 's' || c == 'S') { protocol_ = url::protocol::ssh; state_ = states::protocol_h; }
        else if ((c == 'c' || c == 'C') && protocol_ == url::protocol::icq && is_supported_scheme("icq")) state_ = states::protocol_c;
        else if ((c == 'a' || c == 'A') && protocol_ == url::protocol::agent && is_supported_scheme("magent")) state_ = states::protocol_a;
        else if ((c == 'y' || c == 'Y') && is_supported_scheme("myteam-messenger")) { protocol_ = url::protocol::biz; state_ = states::protocol_y; }
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_t2:
        if (c == 't' || c == 'T') state_ = states::protocol_p;
        else if (c == 'd' || c == 'D') state_ = states::protocol_d;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_p:
        if (c == 'p' || c == 'P') state_ = protocol_ == url::protocol::sftp ? states::delimeter_colon : states::protocol_s;
        else if (c == 'd' || c == 'D') state_ = states::protocol_d;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_c:
        if (c == 'q' || c == 'Q') state_ = states::protocol_q;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_h:
        if (c == 'h' || c == 'H') state_ = states::delimeter_colon;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_t:
    case states::protocol_q:
        if (c == ':') state_ = states::delimeter_slash1;
        else if (c == 'e' || c == 'E') state_ = states::protocol_e;
        else if (is_allowable_char(c, is_utf8_)) { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        else URL_PARSER_RESET_PARSER
            break;
    case states::protocol_s:
        if (c == 's' || c == 'S') { if (protocol_ == url::protocol::http) protocol_ = url::protocol::https; else protocol_ = url::protocol::ftps; state_ = states::delimeter_colon; }
        else if (c == '.') { protocol_ = url::protocol::undefined; state_ = states::host_dot; }
        else if (c == ':') state_ = states::delimeter_slash1;
        else if (is_allowable_char(c, is_utf8_)) { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        else URL_PARSER_RESET_PARSER
            break;
    case states::protocol_a:
        if (c == 'g' || c == 'G') state_ = states::protocol_g;
        else if (c == 'm' || c == 'M') state_ = states::protocol_m;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_g:
        if (c == 'e' || c == 'E') state_ = states::protocol_e;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_e:
        if (c == 'n' || c == 'N') state_ = states::protocol_n;
        else if (c == 'a' || c == 'A') state_ = states::protocol_a;
        else if (c == 's' || c == 'S') state_ = states::protocol_s1;
        else if (c == 'g' || c == 'G') state_ = states::protocol_g;
        else if (c == 'r' || c == 'R') state_ = states::protocol_r;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_n:
        if (c == 't' || c == 'T') state_ = states::protocol_t;
        if ((c == 'c' || c == 'C') && protocol_ == url::protocol::vnc) state_ = states::delimeter_colon;
        else if (c == 'g' || c == 'G') state_ = states::protocol_g;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_y:
        if (c == 't' || c == 'T') state_ = states::protocol_t;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_m:
        if (c == '-') state_ = states::delimeter_dash;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_m1:
        if (c == 'e' || c == 'E') state_ = states::protocol_e;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_s1:
        if (c == 's' || c == 'S') state_ = states::protocol_s1;
        else if (c == 'e' || c == 'E') state_ = states::protocol_e;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_r:
        if (c == ':') state_ = states::delimeter_slash1;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::protocol_d:
        if ((c == 'p' || c == 'P') && protocol_ == url::protocol::rdp) { state_ = states::delimeter_colon; }
        else if (c == '-' && is_supported_scheme("itd-messenger")) { protocol_ = url::protocol::dit; state_ = states::delimeter_dash; }
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::delimeter_dash:
        if (c == 'm' || c == 'M') state_ = states::protocol_m1;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; URL_PARSER_SET_STATE_AND_REPROCESS(states::host) }
        break;
    case states::check_www:
        if (c == 'w' || c == 'W') state_ = states::www_2;
        else if (compare_set_.count(c)) { start_fixed_urls_compare(); return; }
        else if (is_allowable_char(c, is_utf8_)) { save_char_buf(domain_); state_ = states::host; }
        else URL_PARSER_RESET_PARSER
            break;
    case states::www_2:
        if (c == 'w' || c == 'W') state_ = states::www_3;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::host;
        else URL_PARSER_RESET_PARSER
            break;
    case states::www_3:
        if (c == 'w' || c == 'W') state_ = states::www_4;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::host;
        else URL_PARSER_RESET_PARSER
            break;
    case states::www_4:
        if (c == '.') state_ = states::check_filesharing;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::host;
        else URL_PARSER_RESET_PARSER
            break;
    case states::compare:
        if (safe_position_ && compare_pos_ >= safe_position_) state_ = states::compare_safe;
        [[fallthrough]];
    case states::compare_safe:
    {
        auto compare_result = compare_fixed_urls(compare_pos_);
        if (compare_result == fixed_urls_compare_state::no_match) { fallback(char_buf_.front()); }
        else if (compare_result == fixed_urls_compare_state::match) { state_ = ok_state_; }
        return;
    }
    case states::delimeter_colon:
        if (c == ':') state_ = states::delimeter_slash1;
        else if (c == '.') { protocol_ = url::protocol::undefined; state_ = states::host_dot; }
        else if (is_allowable_char(c, is_utf8_)) { protocol_ = url::protocol::undefined; state_ = states::host; }
        else URL_PARSER_SET_STATE_AND_REPROCESS(states::host)
            break;
    case states::delimeter_slash1:
        if (c == '/') state_ = states::delimeter_slash2;
        else URL_PARSER_RESET_PARSER
            break;
    case states::delimeter_slash2:
        if (c == '/') { has_protocol_prefix_ = true; state_ = states::check_www; }
        else URL_PARSER_RESET_PARSER
            break;
    case states::filesharing_id:
    case states::profile_id:
        ++id_length_;
        if (is_space(c, is_utf8_)) { if (id_length_ >= min_profile_id_length) URL_PARSER_URI_FOUND else { state_ = states::host; need_to_check_domain_ = false; URL_PARSER_URI_FOUND } }
        else if (!is_allowed_profile_id_char(c, is_utf8_)) URL_PARSER_RESET_PARSER
            break;
    case states::check_filesharing:
        if (compare_set_.count(c)) { start_fixed_urls_compare(); return; }
        else if (is_allowable_char(c, is_utf8_)) { save_char_buf(domain_); state_ = states::host; }
        else URL_PARSER_RESET_PARSER
            break;
    case states::password:
        if (c == '@') state_ = states::host;
        else if (!is_allowable_char(c, is_utf8_)) URL_PARSER_RESET_PARSER
            break;
    case states::host:
        if (c == '.') state_ = states::host_dot;
        else if (c == ':') state_ = states::host_colon;
        else if (c == '@') state_ = states::host_at;
        else if (c == '/') { if (protocol_ == url::protocol::undefined) URL_PARSER_RESET_PARSER }
        else if (c == '?') { if (protocol_ != url::protocol::icq && protocol_ != url::protocol::agent && protocol_ != url::protocol::biz && protocol_ != url::protocol::dit) URL_PARSER_RESET_PARSER }
        else if (!is_allowable_char(c, is_utf8_)) { if (protocol_ != url::protocol::undefined) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else save_char_buf(domain_);
        break;
    case states::host_n:
        if (c == '.') state_ = states::host_dot;
        else if (c == ':') { if (!is_email_ && is_valid_top_level_domain(domain_)) state_ = states::host_colon; else URL_PARSER_RESET_PARSER }
        else if (c == '@') { if (is_email_ || is_not_email_) URL_PARSER_RESET_PARSER else if (is_black_list_email_host(domain_)) { break; } else { state_ = states::host; is_email_ = true; } }
        else if (c == '/') { if (!is_email_ && is_valid_top_level_domain(domain_)) state_ = states::host_slash; else URL_PARSER_RESET_PARSER }
        else if (c == '?') { if (is_email_) URL_PARSER_URI_FOUND else if (is_valid_top_level_domain(domain_)) state_ = states::query; else URL_PARSER_RESET_PARSER }
        else if (c == '#') { if (!is_email_ && is_valid_top_level_domain(domain_)) state_ = states::query; else URL_PARSER_RESET_PARSER }
        else if (is_space(c, is_utf8_)) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER; }
        else if (is_ending_char(c)) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else if (c == RIGHT_DOUBLE_QUOTE[0] && char_size_ == 2 && char_buf_[1] == RIGHT_DOUBLE_QUOTE[1]) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else if (c == RIGHT_QUOTE[0] && char_size_ == 3 && char_buf_[1] == RIGHT_QUOTE[1] && char_buf_[2] == RIGHT_QUOTE[2]) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else if (!is_letter_or_digit(c, is_utf8_) && c != '-' && c != '_') URL_PARSER_RESET_PARSER
        else save_char_buf(domain_);
        break;
    case states::host_at:
        if (!is_allowable_char(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else if (protocol_ != url::protocol::undefined) state_ = states::host;
        else if (is_black_list_email_host(domain_)) break;
        else if (is_email_ || is_not_email_) URL_PARSER_RESET_PARSER
        else { state_ = states::host; is_email_ = true; }
        break;
    case states::host_dot:
        ++domain_segments_;
        if (is_ipv4_segment(domain_))
            ++ipv4_segnents_;
        if (is_letter_or_digit(c, is_utf8_)) { domain_.clear(); save_char_buf(domain_); state_ = states::host_n; }
        else if (is_space(c, is_utf8_)) { --domain_segments_; URL_PARSER_URI_FOUND }
        else URL_PARSER_RESET_PARSER
            break;
    case states::host_colon:
        if (is_digit(c, is_utf8_)) state_ = states::port;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::password;
        else URL_PARSER_RESET_PARSER
            break;
    case states::host_slash:
        if (c == '/') break;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else URL_PARSER_RESET_PARSER
            break;
    case states::port:
        if (c == '/') { if (is_valid_top_level_domain(domain_)) state_ = states::port_slash; else URL_PARSER_RESET_PARSER }
        else if (is_digit(c, is_utf8_)) break;
        else if (c == '@') state_ = states::host;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::password;
        else URL_PARSER_RESET_PARSER
            break;
    case states::port_dot:
        if (is_digit(c, is_utf8_)) state_ = states::port;
        else URL_PARSER_RESET_PARSER
            break;
    case states::port_slash:
        if (is_letter_or_digit(c, is_utf8_)) state_ = states::path;
        else if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else URL_PARSER_RESET_PARSER
            break;
    case states::path:
        if (c == '.') state_ = states::path_dot;
        else if (c == '/') state_ = states::path_slash;
        else if (c == '?') state_ = states::query;
        else if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else if (c == RIGHT_DOUBLE_QUOTE[0] && char_size_ == 2 && char_buf_[1] == RIGHT_DOUBLE_QUOTE[1]) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else if (c == RIGHT_QUOTE[0] && char_size_ == 3 && char_buf_[1] == RIGHT_QUOTE[1] && char_buf_[2] == RIGHT_QUOTE[2]) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else if (is_allowable_char(c, is_utf8_)) break;
        else if (is_ending_char(c)) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        break;
    case states::path_dot:
        if (c == 'j' || c == 'J') state_ = states::jpg_p;
        else if (c == 'p' || c == 'P') state_ = states::png_n;
        else if (c == 'g' || c == 'G') state_ = states::gif_i;
        else if (c == 'b' || c == 'B') state_ = states::bmp_m;
        else if (c == 't' || c == 'T') state_ = states::tiff_i;
        else if (c == 'a' || c == 'A') state_ = states::avi_v;
        else if (c == 'm' || c == 'M') state_ = states::mkv_k;
        else if (c == 'f' || c == 'F') state_ = states::flv_l;
        else if (c == 's' || c == 'S') state_ = states::svg_v;
        else if (c == '3') state_ = states::_3gp_g;
        else if (c == 'w' || c == 'W') state_ = states::webm_e;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else URL_PARSER_RESET_PARSER
            break;
    case states::path_slash:
        if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else if (c == '?') state_ = states::query;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::jpg_p:
        if (c == 'p' || c == 'P') state_ = states::jpg_g;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::jpg_g:
        if (c == 'g' || c == 'G') { extension_ = url::extension::jpg; state_ = states::query_or_end; }
        else if (c == 'e' || c == 'E') state_ = states::jpg_e;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::jpg_e:
        if (c == 'g' || c == 'G') { extension_ = url::extension::jpeg; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::png_n:
        if (c == 'n' || c == 'N') state_ = states::png_g;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::png_g:
        if (c == 'g' || c == 'G') { extension_ = url::extension::png; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::gif_i:
        if (c == 'i' || c == 'I') state_ = states::gif_f;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::gif_f:
        if (c == 'f' || c == 'F') { extension_ = url::extension::gif; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::bmp_m:
        if (c == 'm' || c == 'M') state_ = states::bmp_p;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::bmp_p:
        if (c == 'p' || c == 'P') { extension_ = url::extension::bmp; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::tiff_i:
        if (c == 'i' || c == 'I') state_ = states::tiff_f1;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::tiff_f1:
        if (c == 'f' || c == 'F') state_ = states::tiff_f2;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::tiff_f2:
        if (c == 'f' || c == 'F') { extension_ = url::extension::tiff; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::avi_v:
        if (c == 'v' || c == 'V') state_ = states::avi_i;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::avi_i:
        if (c == 'i' || c == 'I') { extension_ = url::extension::avi; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::mkv_k:
        if (c == 'k' || c == 'K') state_ = states::mkv_v;
        else if (c == 'o' || c == 'O') state_ = states::mov_v;
        else if (c == 'p' || c == 'P') state_ = states::mpeg4_e;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::mkv_v:
        if (c == 'v' || c == 'V') { extension_ = url::extension::mkv; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::mov_v:
        if (c == 'v' || c == 'V') { extension_ = url::extension::mov; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::mpeg4_e:
        if (c == 'e' || c == 'E') state_ = states::mpeg4_g;
        else if (c == '4') { extension_ = url::extension::mpeg4; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::mpeg4_g:
        if (c == 'g' || c == 'G') state_ = states::mpeg4_4;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::mpeg4_4:
        if (c == '4') { extension_ = url::extension::mpeg4; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::svg_v:
        if (c == 'v' || c == 'V') state_ = states::svg_g;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::svg_g:
        if (c == 'g' || c == 'G') { extension_ = url::extension::svg; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::flv_l:
        if (c == 'l' || c == 'L') state_ = states::flv_v;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::flv_v:
        if (c == 'v' || c == 'V') { extension_ = url::extension::flv; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::_3gp_g:
        if (c == 'g' || c == 'G') state_ = states::_3gp_p;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::_3gp_p:
        if (c == 'p' || c == 'P') { extension_ = url::extension::_3gp; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::webm_e:
        if (c == 'e' || c == 'E') state_ = states::webm_b;
        else if (c == 'm' || c == 'M') state_ = states::wmv_v;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::webm_b:
        if (c == 'b' || c == 'B') state_ = states::webm_m;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::webm_m:
        if (c == 'm' || c == 'M') { extension_ = url::extension::webm; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::wmv_v:
        if (c == 'v' || c == 'V') { extension_ = url::extension::wmv; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
            break;
    case states::query_or_end:
        if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else if (c == '>') URL_PARSER_URI_FOUND
        else if (c == '?' || c == '!' || c == ',' || c == '#' || c == '/' || c == ':' || c == '.') state_ = states::query;
        else if (c == '_') state_ = states::path;
        else if (is_letter_or_digit(c, is_utf8_)) { extension_ = url::extension::undefined; state_ = states::query; }
        else URL_PARSER_RESET_PARSER
            break;
    case states::query:
        if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else if (!is_allowable_query_char(c, is_utf8_)) URL_PARSER_RESET_PARSER
            break;
    default:
        assert(!"invalid state_!");
    };

    save_char_buf(buf_);
}

#undef URL_PARSER_RESET_PARSER

#undef URL_PARSER_URI_FOUND

void common::tools::url_parser::reset()
{
    state_ = states::lookup;

    protocol_ = url::protocol::undefined;

    buf_.clear();
    domain_.clear();

    to_compare_ = nullptr;
    compare_pos_ = 0;

    extension_ = url::extension::undefined;

    ok_state_ = states::lookup;
    fallback_state_ = states::lookup;
    safe_position_ = 0;

    is_not_email_ = false;
    is_email_ = false;

    has_protocol_prefix_ = false;

    domain_segments_ = 1;
    ipv4_segnents_ = 1;

    id_length_ = 0;

    char_buf_.fill(0);
    char_pos_ = 0;
    char_size_ = 0;

    is_utf8_ = false;

    url_.type_ = url::type::undefined;

    need_to_check_domain_ = true;

    tail_size_ = 0;
}

bool common::tools::url_parser::save_url()
{
    switch (state_)
    {
    case states::lookup:
    case states::protocol_t1:
    case states::protocol_t2:
    case states::protocol_p:
    case states::protocol_s:
    case states::protocol_c:
    case states::protocol_d:
    case states::protocol_q:
    case states::protocol_a:
    case states::protocol_g:
    case states::protocol_e:
    case states::protocol_m:
    case states::protocol_m1:
    case states::protocol_r:
    case states::protocol_y:
    case states::protocol_s1:
    case states::protocol_n:
    case states::protocol_t:
    case states::protocol_f:
    case states::protocol_h:
    case states::protocol_v:
    case states::check_www:
    case states::www_2:
    case states::www_3:
    case states::www_4:
    case states::compare:
        return false;
    case states::compare_safe:
    {
        const auto is_like_ftp = protocol_ == url::protocol::ftp || protocol_ == url::protocol::ftps || protocol_ == url::protocol::sftp || protocol_ == url::protocol::ssh || protocol_ == url::protocol::vnc || protocol_ == url::protocol::rdp;
        url_ = make_url(is_like_ftp ? common::tools::url::type::ftp : common::tools::url::type::site);
        return true;
    }
    case states::delimeter_colon:
    case states::delimeter_dash:
        return false;
    case states::delimeter_slash1:
        return false;
    case states::delimeter_slash2:
        return false;
    case states::filesharing_id:
        break;
    case states::profile_id:
        url_ = make_url(common::tools::url::type::profile);
        return true;
    case states::check_filesharing:
        return false;
    case states::password:
        return false;
    case states::host:
        if (protocol_ != url::protocol::undefined)
            break;
        return false;
    case states::host_n:
        break;
    case states::host_at:
        break;
    case states::host_dot:
        break;
    case states::host_colon:
        return false;
    case states::host_slash:
        break;
    case states::port:
        break;
    case states::port_dot:
        break;
    case states::port_slash:
        break;
    case states::path:
        break;
    case states::path_dot:
        break;
    case states::path_slash:
        break;
    case states::jpg_p:
        break;
    case states::jpg_g:
        break;
    case states::jpg_e:
        break;
    case states::png_n:
        break;
    case states::png_g:
        break;
    case states::gif_i:
        break;
    case states::gif_f:
        break;
    case states::bmp_m:
        break;
    case states::bmp_p:
        break;
    case states::tiff_i:
        break;
    case states::tiff_f1:
        break;
    case states::tiff_f2:
        break;
    case states::avi_v:
        break;
    case states::avi_i:
        break;
    case states::mkv_k:
        break;
    case states::mkv_v:
        break;
    case states::mov_v:
        break;
    case states::mpeg4_e:
        break;
    case states::mpeg4_g:
        break;
    case states::mpeg4_4:
        break;
    case states::flv_l:
        break;
    case states::flv_v:
        break;
    case states::svg_v:
        break;
    case states::svg_g:
        break;
    case states::_3gp_g:
        break;
    case states::_3gp_p:
        break;
    case states::webm_e:
        break;
    case states::webm_b:
        break;
    case states::webm_m:
        break;
    case states::wmv_v:
        break;
    case states::query_or_end:
        break;
    case states::query:
        break;
    default:
        assert(!"you must handle all enumerations");
    };

    if (domain_segments_ == 1 && state_ != states::filesharing_id && protocol_ == url::protocol::undefined)
        return false;

    while (is_ending_char(buf_.back()))
    {
        buf_.pop_back();
        ++tail_size_;
    }

    if (buf_.back() == ')')
    {
        buf_.pop_back();
        if (buf_.back() != '/')
            buf_.push_back(')');
        else
            ++tail_size_;
    }

    if (char_size_ == 2 && char_buf_[0] == NBSP_FIRST && char_buf_[1] == NBSP_SECOND)
        ++tail_size_;

    if (is_email_ && protocol_ == url::protocol::undefined && is_valid_top_level_domain(domain_))
    {
        url_ = make_url(common::tools::url::type::email);
    }
    else if (extension_ == url::extension::bmp
        || extension_ == url::extension::gif
        || extension_ == url::extension::jpeg
        || extension_ == url::extension::jpg
        || extension_ == url::extension::png
        || extension_ == url::extension::svg
        || extension_ == url::extension::tiff)
    {
        url_ = make_url(common::tools::url::type::image);
    }
    else if (extension_ == url::extension::_3gp
        || extension_ == url::extension::avi
        || extension_ == url::extension::flv
        || extension_ == url::extension::mkv
        || extension_ == url::extension::mov
        || extension_ == url::extension::mpeg4
        || extension_ == url::extension::webm
        || extension_ == url::extension::wmv)
    {
        url_ = make_url(common::tools::url::type::video);
    }
    else
    {
        const auto is_filesharing_link =
            (state_ == states::filesharing_id) && (id_length_ >= min_filesharing_id_length);
        if (is_filesharing_link)
        {
            url_ = make_url(common::tools::url::type::filesharing);
        }
        else
        {
            if (need_to_check_domain_ && !is_valid_top_level_domain(domain_))
                return false;
            url_ = make_url(protocol_ == url::protocol::ftp || protocol_ == url::protocol::ftps ? common::tools::url::type::ftp : common::tools::url::type::site);
        }
    }

    return true;
}

common::tools::url common::tools::url_parser::make_url(url::type _type) const
{
    if (!has_protocol_prefix_ && _type != url::type::email)
    {
        switch (protocol_)
        {
        case url::protocol::undefined:
            return url(buf_, "http://" + buf_, _type, url::protocol::http, extension_);
        case url::protocol::http:
            return url(buf_, "http://" + buf_, _type, protocol_, extension_);
        case url::protocol::ftp:
            return url(buf_, "ftp://" + buf_, _type, protocol_, extension_);
        case url::protocol::https:
            return url(buf_, "https://" + buf_, _type, protocol_, extension_);
        case url::protocol::ftps:
            return url(buf_, "ftps://" + buf_, _type, protocol_, extension_);
        case url::protocol::sftp:
            return url(buf_, "sftp://" + buf_, _type, protocol_, extension_);
        case url::protocol::vnc:
            return url(buf_, "vnc://" + buf_, _type, protocol_, extension_);
        case url::protocol::rdp:
            return url(buf_, "rdp://" + buf_, _type, protocol_, extension_);
        case url::protocol::ssh:
            return url(buf_, "ssh://" + buf_, _type, protocol_, extension_);
        case url::protocol::icq:
            return url(buf_, "icq://" + buf_, _type, protocol_, extension_);
        case url::protocol::agent:
            return url(buf_, "magent://" + buf_, _type, protocol_, extension_);
        case url::protocol::biz:
            return url(buf_, "myteam-messenger://" + buf_, _type, protocol_, extension_);
        case url::protocol::dit:
            return url(buf_, "itd-messenger://" + buf_, _type, protocol_, extension_);
        }
    }

    return url(buf_, buf_, _type, protocol_, extension_);
}

void common::tools::url_parser::fallback(char _last_char)
{
    state_ = fallback_state_;

    buf_.resize(buf_.size() - compare_pos_);

    for (char c : compare_buf_)
        process(c);

    process(_last_char);
}

bool common::tools::url_parser::is_equal(const char* _text) const
{
    if (!is_utf8_)
        return char_buf_[0] == *_text || tolower(char_buf_[0]) == *_text;

    for (int i = 0; i < char_size_; ++i)
        if (char_buf_[i] != _text[i])
            return false;

    return true;
}

void common::tools::url_parser::save_char_buf(std::string& _to)
{
    std::copy(char_buf_.data(), char_buf_.data() + char_size_, std::back_inserter(_to));
}

void common::tools::url_parser::save_to_buf(char c)
{
    buf_.push_back(c);
}

bool common::tools::url_parser::is_space(char _c, bool _is_utf8) const
{
    if (char_size_ == 2)
    {
        if (_c == NBSP_FIRST && char_buf_[1] == NBSP_SECOND)
            return true;
    }
    return !_is_utf8 && std::isspace(static_cast<unsigned char>(_c));
}

bool common::tools::url_parser::is_digit(char _c) const
{
    return _c >= '0' && _c <= '9';
}

bool common::tools::url_parser::is_digit(char _c, bool _is_utf8) const
{
    return !_is_utf8 && is_digit(_c);
}

bool common::tools::url_parser::is_letter(char _c, bool _is_utf8) const
{
    return _is_utf8 || (_c >= 0x41 && _c <= 0x5A) || (_c >= 0x61 && _c <= 0x7A);
}

bool common::tools::url_parser::is_letter_or_digit(char _c, bool _is_utf8) const
{
    return is_letter(_c, _is_utf8) || is_digit(_c);
}

bool common::tools::url_parser::is_allowable_char(char _c, bool _is_utf8) const
{
    if (is_letter_or_digit(_c, _is_utf8))
        return true;

    return _c == '-' || _c == '.' || _c == '_' || _c == '~' || _c == '!' || _c == '$' || _c == '&'
        || _c == '\'' || _c == '(' || _c == ')'|| _c == '*' || _c == '+' || _c == ',' || _c == ';'
        || _c == '=' || _c == ':' || _c == '%' || _c == '?' || _c == '#' || _c == '@'
        || _c == '{' || _c == '}' || _c == '/' || _c == '[' || _c == ']' || _c == '"';

    /*
        https://www.w3.org/Addressing/URL/uri-spec.html

        national
            { | } | vline | [ | ] | \ | ^ | ~

        The "national" and "punctuation" characters do not appear in any productions and therefore may not appear in URIs.

        But we must support the links with "national" characters therefore we process { }
    */
}

bool common::tools::url_parser::is_allowable_query_char(char _c, bool _is_utf8) const
{
    if (is_allowable_char(_c, _is_utf8))
        return true;

    return _c == '/' || _c == '?';
}

bool common::tools::url_parser::is_ending_char(char _c) const
{
    switch (_c)
    {
    case '.': return true;
    case ',': return true;
    case '!': return true;
    case '?': return true;
    case '"': return true;
    case '\'': return true;
    case '`': return true;
    case '>': return true;
    case ']': return true;
    case ')': return true;
    case '}': return true;
    case ';': return true;
    }
    return false;
}

bool common::tools::url_parser::is_allowed_profile_id_char(char _c, bool _is_utf8) const
{
    return is_letter_or_digit(_c, _is_utf8) || _c == '.' || _c == '_' || _c == '@' || _c == '-';
}

bool common::tools::url_parser::is_valid_top_level_domain(const std::string& _name) const
{
    if (_name.empty())
        return false;

    if (protocol_ != url::protocol::undefined)
        return true;

    if (domain_segments_ == 4 && ipv4_segnents_ == 4)
        return true;

    return is_valid_domain(_name);
}

bool common::tools::url_parser::is_ipv4_segment(const std::string& _name) const
{
    if (_name.size() <= 3)
    {
        switch (_name.size()) // ok let this is ip4 address
        {
        case 1:
            if (is_digit(_name[0])) return true;
            break;
        case 2:
            if (is_digit(_name[0]) && is_digit(_name[1])) return true;
            break;
        case 3:
            if (is_digit(_name[0]) && is_digit(_name[1]) && is_digit(_name[2])) return true;
        }
    }

    return false;
}

void common::tools::url_parser::init_fixed_urls_compare()
{
    constexpr std::string_view get = "/get";
    for (const auto& url : files_urls_)
    {
        compare_item item;
        item.str = url + '/';
        item.safe_pos = url.size() - get.size() - 1;
        item.ok_state = states::filesharing_id;
        fixed_urls_.push_back(std::move(item));
    }

    compare_item compare_icq_com;
    compare_icq_com.str = ICQ_COM;
    compare_icq_com.safe_pos = ICQ_COM_SAFE_POS;
    compare_icq_com.ok_state = states::filesharing_id;

    compare_item compare_my_com;
    compare_my_com.str = CHAT_MY_COM;
    compare_my_com.safe_pos = CHAT_MY_COM_SAFE_POS;
    compare_my_com.ok_state = states::filesharing_id;

    fixed_urls_.push_back(std::move(compare_icq_com));
    fixed_urls_.push_back(std::move(compare_my_com));

    for (const auto& item : fixed_urls_)
    {
        if (!item.str.empty())
            compare_set_.insert(item.str[0]);
    }
}

common::tools::url_parser::fixed_urls_compare_state common::tools::url_parser::compare_fixed_urls(int _pos)
{
    auto has_partial_match = false;
    auto end_reached = true;

    auto min_safe_pos = -1;

    for (auto& item : fixed_urls_)
    {
        if (_pos && !item.match || item.str.size() - 1 < static_cast<unsigned int>(_pos))
            continue;

        auto match = is_equal(item.str.c_str() + _pos);
        item.match = match;
        has_partial_match |= match;
        end_reached &= (item.str.size() - 1 == static_cast<unsigned int>(_pos));

        if (match && min_safe_pos > item.safe_pos || min_safe_pos == -1)
            min_safe_pos = item.safe_pos;

        if (end_reached && match)
            ok_state_ = item.ok_state;
    }

    if (has_partial_match)
    {
        save_char_buf(compare_buf_);
        save_to_buf(compare_buf_[compare_pos_]);
        compare_pos_ += char_size_;
        safe_position_ = min_safe_pos;
    }

    if (has_partial_match && !end_reached)
        return fixed_urls_compare_state::in_progress;
    else if (has_partial_match && end_reached)
        return fixed_urls_compare_state::match;
    else
        return fixed_urls_compare_state::no_match;
}

void common::tools::url_parser::start_fixed_urls_compare(states _fallback_state)
{
    compare_pos_ = 0;
    safe_position_ = 0;
    compare_buf_.clear();
    fallback_state_ = _fallback_state;

    for (auto& item : fixed_urls_)
        item.match = false;

    auto compare_result = compare_fixed_urls(compare_pos_);

    if (compare_result == fixed_urls_compare_state::match)
        state_ = ok_state_;
    else if (compare_result == fixed_urls_compare_state::no_match)
        state_ = fallback_state_;
    else
        state_ = states::compare;
}
