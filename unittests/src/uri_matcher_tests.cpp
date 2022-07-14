#include "uri_matcher_fixture.h"

TYPED_TEST_SUITE_P(UriMatcherTest);

TYPED_TEST_P(UriMatcherTest, MatchUriWhiteList)
{ 
    using Traits = UriMatcherTestTraits<TypeParam>;
    using StringVector = typename Traits::StringVector;

    const StringVector& whitelist = UriMatcherTest<TypeParam>::GetWhiteList();
    StringVector results;
    for (const auto& s : whitelist)
    {
        this->match(s, std::back_inserter(results));
        ASSERT_EQ(results.size(), size_t(1));
        ASSERT_EQ(results.front(), s);
        results.clear();
    }
}

TYPED_TEST_P(UriMatcherTest, MatchUriBlackList)
{
    using Traits = UriMatcherTestTraits<TypeParam>;
    using StringVector = typename Traits::StringVector;

    const StringVector& blacklist = UriMatcherTest<TypeParam>::GetBlackList();
    StringVector results;
    for (const auto& s : blacklist)
    {
        this->match(s, std::back_inserter(results));
        ASSERT_TRUE(results.empty());
        results.clear();
    }
};

TYPED_TEST_P(UriMatcherTest, MatchUriMixedList)
{
    using Traits = UriMatcherTestTraits<TypeParam>;
    using StringVector = typename Traits::StringVector;
    using StringMap = typename Traits::StringMap;

    const StringMap& mixedlist = UriMatcherTest<TypeParam>::GetMixedList();
    StringVector results;
    for (const auto& kvp : mixedlist)
    {
        this->match(kvp.first, std::back_inserter(results));
        ASSERT_EQ(results, kvp.second);
        results.clear();
    }
};

TYPED_TEST_P(UriMatcherTest, MatchLongUriMixedList)
{
    using Traits = UriMatcherTestTraits<TypeParam>;
    using StringVector = typename Traits::StringVector;
    using StringMap = typename Traits::StringMap;

    const StringMap& mixedlist = UriMatcherTest<TypeParam>::GetLongMixedList();
    StringVector results;
    for (const auto& kvp : mixedlist)
    {
        this->match(kvp.first, std::back_inserter(results));
        ASSERT_EQ(results, kvp.second);
        results.clear();
    }
};


using UnicodeUriMatcherTest = UriMatcherTest<wchar_t>;

TEST_F(UnicodeUriMatcherTest, MatchUnicodeText)
{
    const auto& text = GetUnicodePlainText();
    const auto& expected = GetUnicodeResults();
    std::vector<std::wstring> results;
    match(text, std::back_inserter(results));
    ASSERT_EQ(results, expected);
}

REGISTER_TYPED_TEST_SUITE_P(UriMatcherTest, MatchUriWhiteList, MatchUriBlackList, MatchUriMixedList, MatchLongUriMixedList);

INSTANTIATE_TYPED_TEST_SUITE_P(URI, UriMatcherTest, CharTypeList);

