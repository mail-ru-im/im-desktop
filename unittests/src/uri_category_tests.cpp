#include "uri_category_fixture.h"
#include "../../common.shared/config/config.h"

TYPED_TEST_SUITE_P(UriCategoryTest);

TYPED_TEST_P(UriCategoryTest, UriCategoryTestMatching)
{
    using Traits = UriCategoryTestTraits<TypeParam>;
    using UriView = typename Traits::UriView;
    using UriCategoryMatcher = typename Traits::UriCategoryMatcher;

    const bool isB2B = config::get().string(config::values::product_name_short) == "vkteams";

    UriCategoryMatcher matcher;
    UriView uri;
    const auto& mapping = UriCategoryTest<TypeParam>::GetMapping();
    for (const auto& kvp : mapping)
    {
        if (isB2B && kvp.second == static_cast<int>(category_type::icqprotocol))
            continue;

        uri.assign(kvp.first);
        const auto result = matcher.find_category(uri);
        ASSERT_TRUE(kvp.second == static_cast<int>(result));
    }
}

REGISTER_TYPED_TEST_SUITE_P(UriCategoryTest, UriCategoryTestMatching);

INSTANTIATE_TYPED_TEST_SUITE_P(URI, UriCategoryTest, CharTypeList);