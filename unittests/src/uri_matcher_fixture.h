#pragma once
#include <unordered_map>
#include "common.h"
#include "../../common.shared/uri_matcher/uri_matcher.h"

#ifdef _WIN32
#define _INPUT_PATH_PREFIX "..\\..\\unittests\\input\\uri\\"
#else
#define _INPUT_PATH_PREFIX "../../unittests/input/uri/"
#endif

template<class _Char>
struct UriMatcherTestTraits
{
    using StringType = std::basic_string<_Char>;
    using StringVector = std::vector<StringType>;
    using StringMap = std::vector<std::pair<StringType, StringVector>>;
    using MatcherType = basic_uri_matcher<_Char>;
};


template<class _Char>
class UriMatcherTest : public ::testing::Test
{
public:
    using TraitsType = UriMatcherTestTraits<_Char>;
    using StringType = typename TraitsType::StringType;
    using StringVector = typename TraitsType::StringVector;
    using StringMap = typename TraitsType::StringMap;
    using MatcherType = typename TraitsType::MatcherType;

    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestCase()
    {
        if (whiteList_.empty())
            read_lines<_Char>(_INPUT_PATH_PREFIX "whitelist.txt", std::back_inserter(whiteList_));

        if (blackList_.empty())
            read_lines<_Char>(_INPUT_PATH_PREFIX "blacklist.txt", std::back_inserter(blackList_));

        if (mixedMap_.empty())
            read_kvl<_Char>(_INPUT_PATH_PREFIX "mixedlist.txt", _Char(' '), std::back_inserter(mixedMap_));

        if (longMixedMap_.empty())
            read_kvl<_Char>(_INPUT_PATH_PREFIX "longmixed.txt", _Char(' '), std::back_inserter(longMixedMap_));

        if (unicodeResults_.empty())
            readUnicodeResults();

        if (unicodeText_.empty())
            readUnicodeText();

    }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestCase()
    {
        whiteList_.clear();
        blackList_.clear();
        mixedMap_.clear();
        longMixedMap_.clear();
    }

protected:
    static const StringVector& GetWhiteList() { return whiteList_; }
    static const StringVector& GetBlackList() { return blackList_; }
    static const StringMap& GetMixedList() { return mixedMap_; }
    static const StringMap& GetLongMixedList() { return longMixedMap_; }
    static const std::wstring& GetUnicodePlainText() { return unicodeText_; }
    static const std::vector<std::wstring>& GetUnicodeResults() { return unicodeResults_; }

    static constexpr size_t widechar_size = sizeof(wchar_t);
    static_assert (widechar_size == 2 || widechar_size == 4, "wide character type is unsupported");

    static void readUnicodeText()
    {
        std::string rawdata;
        read_content(_INPUT_PATH_PREFIX "plain_text_utf16.txt", rawdata);
        if (rawdata.size() > 0 && (rawdata.size() % sizeof(wchar_t)) == 0)
        {
            const size_t n = rawdata.size() / sizeof(char16_t);
            if constexpr (widechar_size == 2) // wchar_t seems to be UTF-16
            {
                unicodeText_.resize(rawdata.size() / sizeof(wchar_t));
                std::memcpy(unicodeText_.data(), rawdata.data(), rawdata.size());
            }
            else
            {
                unicodeText_.resize(n);
                const char16_t* utf16 = reinterpret_cast<const char16_t*>(rawdata.data());
                std::copy(utf16, utf16 + n, unicodeText_.begin());
            }
        }
    }

    static void readUnicodeResults()
    {
        std::string rawdata;
        read_content(_INPUT_PATH_PREFIX "results_plain_text_utf16.txt", rawdata);

        if (rawdata.size() > 0 && (rawdata.size() % sizeof(char16_t)) == 0)
        {
            size_t len = rawdata.size() / sizeof(char16_t);
            const char* p = rawdata.data();
            uint8_t b = *p;
            if (b == 0xFF || b == 0xFE)
            {
                if (len < 2)
                    return;

                uint16_t bom = 0;
                std::memcpy(&bom, p, sizeof(uint16_t));
                if (bom != 0xFFFE && bom != 0xFEFF)
                    return;
                p += 2;
                --len;
            }

            if constexpr (widechar_size == 2) // wchar_t seems to be UTF-16
            {
                const wchar_t* s = reinterpret_cast<const wchar_t*>(p);
                su::split_if(std::wstring_view{ s, len }, std::back_inserter(unicodeResults_), [](auto c) { return c == wchar_t('\r') || c == wchar_t('\n'); });
            }
            else
            {
                std::wstring tmp;
                tmp.reserve(len);
                const char16_t* utf16 = reinterpret_cast<const char16_t*>(p);
                std::copy(utf16, utf16 + len, std::back_inserter(tmp));
                su::split_if((std::wstring_view)tmp, std::back_inserter(unicodeResults_), [](auto c) { return c == wchar_t('\r') || c == wchar_t('\n'); });
            }
        }

    }

    template<class _OutIt>
    static void match(const StringType& s, _OutIt out)
    {
        MatcherType matcher;
        matcher.search_copy(s.begin(), s.end(), out);
    }

private:
    static StringVector whiteList_;
    static StringVector blackList_;
    static StringMap    mixedMap_;
    static StringMap    longMixedMap_;
    static std::wstring unicodeText_;
    static std::vector<std::wstring> unicodeResults_;
};

template<class _Char>
typename UriMatcherTest<_Char>::StringVector UriMatcherTest<_Char>::whiteList_;

template<class _Char>
typename UriMatcherTest<_Char>::StringVector UriMatcherTest<_Char>::blackList_;

template<class _Char>
typename UriMatcherTest<_Char>::StringMap UriMatcherTest<_Char>::mixedMap_;

template<class _Char>
typename UriMatcherTest<_Char>::StringMap UriMatcherTest<_Char>::longMixedMap_;

template<class _Char>
std::wstring UriMatcherTest<_Char>::unicodeText_;

template<class _Char>
std::vector<std::wstring> UriMatcherTest<_Char>::unicodeResults_;

#undef _INPUT_PATH_PREFIX
