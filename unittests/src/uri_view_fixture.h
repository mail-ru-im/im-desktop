#pragma once
#include "common.h"
#include "../../common.shared/uri_matcher/uri.h"

#ifdef _WIN32
#define _INPUT_PATH_PREFIX "..\\..\\unittests\\input\\uri\\"
#else
#define _INPUT_PATH_PREFIX "../../unittests/input/uri/"
#endif


template<class _Char>
struct UriViewParts
{
    using StringType = std::basic_string<_Char>;
    StringType username_;
    StringType password_;
    StringType host_;
    StringType path_;
    StringType query_;
    StringType fragment_;
    int scheme_;
    int32_t port_;

    bool compare(basic_uri_view<_Char> _v) const noexcept
    {
        return scheme_ == _v.scheme() &&
            username_ == _v.username() &&
            password_ == _v.password() &&
            host_ == _v.host() &&
            port_ == _v.port() &&
            path_ == _v.path() &&
            query_ == _v.query() &&
            fragment_ == _v.fragment();
    }
};

template<class _Char, class _Traits>
inline std::basic_istream<_Char, _Traits>& operator>> (std::basic_istream<_Char, _Traits>& _in, UriViewParts<_Char>& _p)
{
    _in >> _p.scheme_;
    _in >> std::quoted(_p.username_);
    _in >> std::quoted(_p.password_);
    _in >> std::quoted(_p.host_);
    _in >> _p.port_;
    _in >> std::quoted(_p.path_);
    _in >> std::quoted(_p.query_);
    _in >> std::quoted(_p.fragment_);
    return _in;
}


template<class _Char>
struct UriViewTestTraits
{
    using UriView = basic_uri_view<_Char>;
    using StringType = std::basic_string<_Char>;
    using UriViewMap = std::vector<std::pair<StringType, UriViewParts<_Char>>>;
    using UriFormattedMap = std::vector<std::pair<StringType, StringType>>;
};

template<class _Char>
class UriViewTest : public ::testing::Test
{
public:
    using TraitsType = UriViewTestTraits<_Char>;
    using UriView = typename TraitsType::UriView;
    using StringType = typename TraitsType::StringType;
    using UriViewMap = typename TraitsType::UriViewMap;
    using UriFormattedMap = typename TraitsType::UriFormattedMap;

    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestCase()
    {
        if (mapping_.empty())
            read_kvp<_Char, StringType, UriViewParts<_Char>>(_INPUT_PATH_PREFIX "uri.txt", _Char(' '), std::back_inserter(mapping_));
        if (formatted_.empty())
            read_kvp<_Char, StringType, StringType>(_INPUT_PATH_PREFIX "formatting.txt", _Char(' '), std::back_inserter(formatted_));
    }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestCase()
    {
        mapping_.clear();
        formatted_.clear();
    }

protected:
    static const UriViewMap& GetUrlMapping() { return mapping_; }
    static const UriFormattedMap& GetFormattedMapping() { return formatted_; }

private:
    static UriViewMap mapping_;
    static UriFormattedMap formatted_;
};

template<class _Char>
typename UriViewTest<_Char>::UriViewMap UriViewTest<_Char>::mapping_;

template<class _Char>
typename UriViewTest<_Char>::UriFormattedMap UriViewTest<_Char>::formatted_;

#undef _INPUT_PATH_PREFIX
