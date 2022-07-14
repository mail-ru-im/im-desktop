#include "stdafx.h"
#include "rk_searcher_fixture.h"

TYPED_TEST_CASE_P(RkSearcherTest);

TYPED_TEST_P(RkSearcherTest, RkSearchIntSet)
{
    using TestTraits = RkSearcherTestTraits<TypeParam>;
    using Sequence = typename TestTraits::Sequence;

	static constexpr size_t MinLen = 3;
	static constexpr size_t MaxLen = 8;

    using SearcherType = rk_searcher<Sequence, MinLen>;
    RkSearcherTest<TypeParam>::template TestSearcherIntSet<SearcherType, MinLen, MaxLen>();
}

TYPED_TEST_P(RkSearcherTest, RkSearchSetCaseSensitive)
{
	static constexpr size_t MinLen = 3;
	static constexpr size_t MaxLen = 8;

    using TestTraits = RkSearcherTestTraits<TypeParam>;
    using StringType = typename TestTraits::StringType;

	using SearcherType = rk_searcher<StringType, MinLen>;
    RkSearcherTest<TypeParam>::template TestSearcherStrSet<SearcherType, MinLen, MaxLen>(true);
}

TYPED_TEST_P(RkSearcherTest, RkSearchSetCaseInsensitive)
{
	static constexpr size_t MinLen = 3;
	static constexpr size_t MaxLen = 8;

    using TestTraits = RkSearcherTestTraits<TypeParam>;
    using StringType = typename TestTraits::StringType;

	using SearcherType = rk_searcher<StringType, MinLen>;
    RkSearcherTest<TypeParam>::template TestSearcherStrSet<SearcherType, MinLen, MaxLen>(false);
}

TYPED_TEST_P(RkSearcherTest, RkSearchSetCaseSensitiveAscii)
{
	static constexpr size_t MinLen = 3;
	static constexpr size_t MaxLen = 8;

    using TestTraits = RkSearcherTestTraits<TypeParam>;
    using StringType = typename TestTraits::StringType;

	using TraitsType = rk_traits_ascii<StringType, MinLen>;
	using SearcherType = rk_searcher<StringType, MinLen, TraitsType>;
    RkSearcherTest<TypeParam>::template TestSearcherStrSet<SearcherType, MinLen, MaxLen>(true);
}

TYPED_TEST_P(RkSearcherTest, RkSearchSetCaseInsensitiveAscii)
{
	static constexpr size_t MinLen = 3;
	static constexpr size_t MaxLen = 8;

    using TestTraits = RkSearcherTestTraits<TypeParam>;
    using StringType = typename TestTraits::StringType;

	using TraitsType = rk_traits_ascii<StringType, MinLen>;
	using SearcherType = rk_searcher<StringType, MinLen, TraitsType>;
    RkSearcherTest<TypeParam>::template TestSearcherStrSet<SearcherType, MinLen, MaxLen>(false);
}

TYPED_TEST_P(RkSearcherTest, RkSearchSmallSetCaseSensitive)
{
	static constexpr size_t MinLen = 3;
	static constexpr size_t MaxLen = 8;
	static constexpr size_t Count = MaxLen - MinLen;

    using TestTraits = RkSearcherTestTraits<TypeParam>;
    using StringType = typename TestTraits::StringType;

	using TraitsType = rk_traits<StringType, MinLen>;
	using SearcherType = rk_searcher_sset<StringType, MinLen, Count, TraitsType>;
    RkSearcherTest<TypeParam>::template TestSearcherStrSet<SearcherType, MinLen, MaxLen>(true);
}

TYPED_TEST_P(RkSearcherTest, RkSearchSmallSetCaseInsensitive)
{
	static constexpr size_t MinLen = 3;
	static constexpr size_t MaxLen = 8;
	static constexpr size_t Count = MaxLen - MinLen;

    using TestTraits = RkSearcherTestTraits<TypeParam>;
    using StringType = typename TestTraits::StringType;

	using TraitsType = rk_traits<StringType, MinLen>;
	using SearcherType = rk_searcher_sset<StringType, MinLen, Count, TraitsType>;
    RkSearcherTest<TypeParam>::template TestSearcherStrSet<SearcherType, MinLen, MaxLen>(false);
}

TYPED_TEST_P(RkSearcherTest, RkSearchSmallSetCaseSensitiveAscii)
{
	static constexpr size_t MinLen = 3;
	static constexpr size_t MaxLen = 8;
	static constexpr size_t Count = MaxLen - MinLen;

    using TestTraits = RkSearcherTestTraits<TypeParam>;
    using StringType = typename TestTraits::StringType;

	using TraitsType = rk_traits_ascii<StringType, MinLen>;
	using SearcherType = rk_searcher_sset<StringType, MinLen, Count, TraitsType>;
    RkSearcherTest<TypeParam>::template TestSearcherStrSet<SearcherType, MinLen, MaxLen>(true);
}

TYPED_TEST_P(RkSearcherTest, RkSearchSmallSetCaseInsensitiveAscii)
{
	static constexpr size_t MinLen = 3;
	static constexpr size_t MaxLen = 8;
	static constexpr size_t Count = MaxLen - MinLen;

    using TestTraits = RkSearcherTestTraits<TypeParam>;
    using StringType = typename TestTraits::StringType;

	using TraitsType = rk_traits_ascii<StringType, MinLen>;
	using SearcherType = rk_searcher_sset<StringType, MinLen, Count, TraitsType>;
    RkSearcherTest<TypeParam>::template TestSearcherStrSet<SearcherType, MinLen, MaxLen>(false);
}


REGISTER_TYPED_TEST_CASE_P(RkSearcherTest,
	RkSearchIntSet,
	RkSearchSetCaseSensitive, RkSearchSetCaseInsensitive,
	RkSearchSetCaseSensitiveAscii, RkSearchSetCaseInsensitiveAscii,
	RkSearchSmallSetCaseSensitive, RkSearchSmallSetCaseInsensitive,
	RkSearchSmallSetCaseSensitiveAscii, RkSearchSmallSetCaseInsensitiveAscii);

INSTANTIATE_TYPED_TEST_CASE_P(Searching, RkSearcherTest, CharTypeList);
