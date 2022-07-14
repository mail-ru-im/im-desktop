#include "uri_view_fixture.h"

TYPED_TEST_SUITE_P(UriViewTest);

TYPED_TEST_P(UriViewTest, UriViewTestParsing)
{
    using Traits = UriViewTestTraits<TypeParam>;
    using UriView = typename Traits::UriView;

    UriView uri;
    const auto& mapping = UriViewTest<TypeParam>::GetUrlMapping();
    for (const auto& kvp : mapping)
    {
        uri.assign(kvp.first);
        ASSERT_TRUE(kvp.second.compare(uri));
    }
}

TYPED_TEST_P(UriViewTest, UriViewTestParsingHinted)
{
    using Traits = UriViewTestTraits<TypeParam>;
    using UriView = typename Traits::UriView;

    UriView uri;
    const auto& mapping = UriViewTest<TypeParam>::GetUrlMapping();
    for (const auto& kvp : mapping)
    {
        uri.assign((scheme_type)kvp.second.scheme_, kvp.first);
        ASSERT_TRUE(kvp.second.compare(uri));
    }
}

TYPED_TEST_P(UriViewTest, UriViewTestFormatting)
{
    using Traits = UriViewTestTraits<TypeParam>;
    using UriView = typename Traits::UriView;
    using StringType = typename Traits::StringType;

    UriView uri;
    const auto& formatted = UriViewTest<TypeParam>::GetFormattedMapping();
    for (const auto& kvp : formatted)
    {
        uri.assign(kvp.first);
        auto result = uri.template to_string<StringType>();
        ASSERT_EQ(result, kvp.second);
    }
}

REGISTER_TYPED_TEST_SUITE_P(UriViewTest, UriViewTestParsing, UriViewTestParsingHinted, UriViewTestFormatting);

INSTANTIATE_TYPED_TEST_SUITE_P(URI, UriViewTest, CharTypeList);
