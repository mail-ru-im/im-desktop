#pragma once
#include "common.h"
#include "../../common.shared/uri_matcher/rk_searcher.h"

template<class _Char>
struct RkSearcherTestTraits
{
    using Sequence = std::vector<size_t>;

    using StringType = std::basic_string<_Char>;
    using StringViewType = std::basic_string_view<_Char>;

    using StringSet = std::vector<StringType>;
    using StringMap = std::vector<std::pair<StringType, int>>;
};


template<class _Char>
class RkSearcherTest : public ::testing::Test
{
public:

    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestCase()
    {
    }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestCase()
    {
    }

protected:
    template<class _Searcher, class _RandIt, class _OutIt>
    static void DoSearchSeq(_Searcher& searcher, _RandIt first, _RandIt last, _OutIt out)
    {
        using Sequence = typename RkSearcherTestTraits<_Char>::Sequence;
        auto p = searcher.search(first, last);
        for (; p.first != last; ++out)
        {
            *out = std::make_pair(std::distance(first, p.first), Sequence{ p.first, p.first + p.second->size() });
            p = searcher.search(p.first, last);
        }
    }

    template<class _Searcher, class _RandIt, class _OutIt>
    static void DoSearch(_Searcher& searcher, _RandIt first, _RandIt last, _OutIt out)
    {
        using StringType = typename RkSearcherTestTraits<_Char>::StringType;

        auto p = searcher.search(first, last);
        for (; p.first != last; ++out)
        {
            *out = std::make_pair(std::distance(first, p.first), StringType{ p.first, p.first + p.second->size() });
            p = searcher.search(p.first, last);
        }
    }

    template<class _Searcher, class _Comp, class _RandIt, class _OutIt>
    static void DoSearch(_Searcher& searcher, _Comp c, _RandIt first, _RandIt last, _OutIt out)
    {
        using StringType = typename RkSearcherTestTraits<_Char>::StringType;

        auto p = searcher.search_comp(first, last, c);
        for (; p.first != last; ++out)
        {
            *out = std::make_pair(std::distance(first, p.first), StringType{ p.first, p.first + p.second->size() });
            p = searcher.search_comp(p.first, last, c);
        }
    }

    template<class _Seacher, size_t _MinPatternLength, size_t _MaxPatternLength>
    static void TestSearcherStrSet(bool caseSensitive)
    {
        static_assert(_MinPatternLength > 2, "hash width must be greater than 2");

        using StringType = typename RkSearcherTestTraits<_Char>::StringType;

        static constexpr size_t SourceLength = _MinPatternLength + 16;

        static auto icasecomp = [](_Char x, _Char y) {
            return tolowercase(x) == y;
        };

        std::vector<StringType> patterns(_MaxPatternLength - _MinPatternLength);
        for (size_t i = 0; i < patterns.size(); ++i) // generate "aaa", "aaaa", ... etc
            patterns[i] = StringType(_MinPatternLength + i, _Char('a'));

        StringType source(SourceLength, caseSensitive ? _Char('a') : _Char('A'));
        std::vector<std::pair<size_t, StringType>> results;

        _Seacher searcher(std::begin(patterns), std::end(patterns));
        if (caseSensitive)
            DoSearch(searcher, source.begin(), source.end(), std::back_inserter(results));
        else
            DoSearch(searcher, icasecomp, source.begin(), source.end(), std::back_inserter(results));

        std::sort(results.begin(), results.end()); // sort results

        auto resultIt = results.begin();
        for (size_t i = 0; i < (SourceLength - (_MinPatternLength - 1)); ++i)
        {
            for (size_t j = 0; j < patterns.size(); j++)
            {
                size_t k = (i + patterns[j].size());
                if (k <= SourceLength)
                {
                    // make lower case to compare
                    std::transform(resultIt->second.begin(),
                        resultIt->second.end(),
                        resultIt->second.begin(),
                        [](auto c) { return tolowercase(c); });
                    ASSERT_EQ(resultIt->first, i); // assert position
                    ASSERT_EQ(resultIt->second, patterns[j]); // assert pattern
                    ++resultIt;
                }
            }
        }
    }

    template<class _Seacher, size_t _MinPatternLength, size_t _MaxPatternLength>
    static void TestSearcherIntSet()
    {
        static_assert(_MinPatternLength > 2, "hash width must be greater than 2");

        static constexpr size_t SourceLength = _MinPatternLength + 16;

        using Sequence = typename RkSearcherTestTraits<_Char>::Sequence;

        std::vector<Sequence> patterns(_MaxPatternLength - _MinPatternLength);
        for (size_t i = 0; i < patterns.size(); ++i) // generate "aaa", "aaaa", ... etc
            patterns[i] = Sequence(_MinPatternLength + i, size_t(1));

        Sequence source(SourceLength, 1);
        std::vector<std::pair<size_t, Sequence>> results;

        _Seacher searcher(std::begin(patterns), std::end(patterns));
        DoSearchSeq(searcher, source.begin(), source.end(), std::back_inserter(results));

        std::sort(results.begin(), results.end()); // sort results

        auto resultIt = results.begin();
        for (size_t i = 0; i < (SourceLength - (_MinPatternLength - 1)); ++i)
        {
            for (size_t j = 0; j < patterns.size(); j++)
            {
                size_t k = (i + patterns[j].size());
                if (k <= SourceLength)
                {
                    ASSERT_EQ(resultIt->first, i); // assert position
                    ASSERT_EQ(resultIt->second, patterns[j]); // assert pattern
                    ++resultIt;
                }
            }
        }
    }
};
