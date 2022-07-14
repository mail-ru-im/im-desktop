#pragma once
#include "common.h"
#include "../../common.shared/uri_matcher/uri_matcher.h"

#ifdef _WIN32
#define _INPUT_PATH_PREFIX "..\\..\\unittests\\input\\uri\\"
#else
#define _INPUT_PATH_PREFIX "../../unittests/input/uri/"
#endif

template<class _Char>
struct UriCategoryTestTraits
{
    using UriView = basic_uri_view<_Char>;
    using StringType = std::basic_string<_Char>;
    using UriViewMap = std::vector<std::pair<StringType, int>>;
    using UriCategoryMatcher = uri_category_matcher<_Char>;
};


template<class _Char>
class UriCategoryTest : public ::testing::Test
{
public:
    using TraitsType = UriCategoryTestTraits<_Char>;
    using UriView = typename TraitsType::UriView;
    using StringType = typename TraitsType::StringType;
    using UriViewMap = typename TraitsType::UriViewMap;

    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestCase()
    {
        if (mapping_.empty())
            read_kvp<_Char, StringType, int>(_INPUT_PATH_PREFIX "categories.txt", _Char(' '), std::back_inserter(mapping_));
    }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestCase()
    {
        mapping_.clear();
    }


    static const UriViewMap& GetMapping() { return mapping_; }

    static UriViewMap mapping_;
};

template<class _Char>
typename UriCategoryTest<_Char>::UriViewMap UriCategoryTest<_Char>::mapping_;

#undef _INPUT_PATH_PREFIX
