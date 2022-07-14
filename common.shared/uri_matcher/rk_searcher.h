#pragma once
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <iterator>

#include "fixed_hash.h"
#include "reordering_traits.h"
#include "string_traits.h"

template<class _Pattern, size_t _HashSize>
struct rk_traits
{
    using key_type = _Pattern;
    using fixed_hasher_type = fixed_hash<_HashSize, _Pattern>;
    using lookup_type = std::unordered_set<_Pattern, fixed_hasher_type>;

};

template<class _Key, class _Value, size_t _HashSize>
struct rk_traits<std::pair<_Key, _Value>, _HashSize>
{
    using key_type = _Key;
    using fixed_hasher_type = fixed_hash<_HashSize, _Key>;
    using lookup_type = std::unordered_map<_Key, _Value, fixed_hasher_type>;
};


template<class _Char, class _Traits, class _Alloc, size_t _HashSize>
struct rk_traits<std::basic_string<_Char, _Traits, _Alloc>, _HashSize>
{
    using string_type = std::basic_string<_Char, _Traits, _Alloc>;
    using key_type = std::basic_string_view<_Char, _Traits>;
    using fixed_hasher_type = fixed_hash<_HashSize, key_type>;
    using lookup_type = std::unordered_set<string_type, fixed_hasher_type>;
};

template<class _Char, class _Traits, class _Alloc, class _Value, size_t _HashSize>
struct rk_traits<std::pair<std::basic_string<_Char, _Traits, _Alloc>, _Value>, _HashSize>
{
    using string_type = std::basic_string<_Char, _Traits, _Alloc>;
    using key_type = std::basic_string_view<_Char, _Traits>;
    using mapped_type = _Value;
    using fixed_hasher_type = fixed_hash<_HashSize, key_type>;
    using lookup_type = std::unordered_map<string_type, mapped_type, fixed_hasher_type>;
};


template<class _Pattern, size_t _HashSize>
struct rk_traits_ascii : public rk_traits<_Pattern, _HashSize> {};

template<class _Char, class _Traits, class _Alloc, size_t _HashSize>
struct rk_traits_ascii<std::basic_string<_Char, _Traits, _Alloc>, _HashSize>
{
    using string_type = std::basic_string<_Char, _Traits, _Alloc>;
    using key_type = std::basic_string_view<_Char, _Traits>;
    using fixed_hasher_type = fixed_hash_ascii<_HashSize, key_type>;
    using lookup_type = std::unordered_set<string_type, fixed_hasher_type>;
};

template<class _Char, class _Traits, class _Alloc, class _Value, size_t _HashSize>
struct rk_traits_ascii<std::pair<std::basic_string<_Char, _Traits, _Alloc>, _Value>, _HashSize>
{
    using string_type = std::basic_string<_Char, _Traits, _Alloc>;
    using key_type = std::basic_string_view<_Char, _Traits>;
    using mapped_type = _Value;
    using fixed_hasher_type = fixed_hash_ascii<_HashSize, key_type>;
    using lookup_type = std::unordered_map<string_type, mapped_type, fixed_hasher_type>;
};

template<class _Char, class _Traits, size_t _HashSize>
struct rk_traits_ascii<std::basic_string_view<_Char, _Traits>, _HashSize>
{
    using key_type = std::basic_string_view<_Char, _Traits>;
    using fixed_hasher_type = fixed_hash_ascii<_HashSize, key_type>;
    using lookup_type = std::unordered_set<key_type, fixed_hasher_type>;
};

template<class _Char, class _Traits, class _Value, size_t _HashSize>
struct rk_traits_ascii<std::pair<std::basic_string_view<_Char, _Traits>, _Value>, _HashSize>
{
    using key_type = std::basic_string_view<_Char, _Traits>;
    using mapped_type = _Value;
    using fixed_hasher_type = fixed_hash_ascii<_HashSize, key_type>;
    using lookup_type = std::unordered_map<key_type, mapped_type, fixed_hasher_type>;
};


/*!
 * \brief Rabin-Karp searcher for muti-pattern set
 *
 */
template<
    class _Pattern,
    size_t _HashWidth,
    class _Traits = rk_traits<_Pattern, _HashWidth>
>
class rk_searcher
{
public:
    using lookup_type = typename _Traits::lookup_type;
    using key_type = typename lookup_type::key_type;
    using value_type = typename lookup_type::value_type;
    using hasher = typename lookup_type::hasher;
    using key_equal = typename lookup_type::key_equal;
    using allocator_type = typename lookup_type::allocator_type;

    using pointer = typename lookup_type::pointer;
    using const_pointer = typename lookup_type::const_pointer;
    using reference = typename lookup_type::reference;
    using const_reference = typename lookup_type::const_reference;
    using iterator = typename lookup_type::iterator;
    using const_iterator = typename lookup_type::const_iterator;
    using local_iterator = typename lookup_type::local_iterator;
    using const_local_iterator = typename lookup_type::const_local_iterator;
    using size_type = typename lookup_type::size_type;
    using difference_type = typename lookup_type::difference_type;

    // minimal length of searching substring
    static constexpr size_t hash_width = _HashWidth;
    // sentinel value for maximum length of searching substring
    static constexpr size_t npos = ~size_t(0);

    rk_searcher()
        : hash_code_(0)
        , patterns_(0)
    { }

    /*!
     * \brief  Builds a searcher from a range.
     * \param _first  An input iterator.
     * \param _last  An input iterator.
     * \param _n  Minimal initial number of buckets.
     * \param _hf  A hash functor.
     * \param _eq  A key equality functor.
     * \param _a  An allocator object.
     * \detail
     *  Create a searcher consisting of copies of the elements from
     *  [_first,_last). This is linear in N (where N is
     *  distance(_first,_last)).
     */
    template<typename _InputIterator>
    rk_searcher(_InputIterator _first, _InputIterator _last,
                size_type _n = 0,
                const hasher& _hf = hasher(),
                const key_equal& _eq = key_equal(),
                const allocator_type& _a = allocator_type())
        : lookup_(_n, _hf, _eq, _a)
        , hash_code_(0)
        , patterns_(0)
    {
        for (; _first != _last; ++_first)
        {
            if (std::size(key_of(*_first)) < hash_width)
                continue;
            lookup_.emplace(*_first);
        }
    }

    /// Same as above
    template<typename _InputIterator>
    rk_searcher(_InputIterator _first, _InputIterator _last,
                size_type _n,
                const allocator_type& _a)
        : rk_searcher(_first, _last, _n, hasher(), key_equal(), _a)
    { }

    /// Same as above
    template<typename _InputIterator>
    rk_searcher(_InputIterator _first, _InputIterator _last,
                size_type _n, const hasher& _hf,
                const allocator_type& _a)
        : rk_searcher(_first, _last, _n, _hf, key_equal(), _a)
    { }

    /*!
     * \brief Creates an searcher with no elements.
     * \param _a An allocator object.
     */
    explicit rk_searcher(const allocator_type& _a)
        : lookup_(_a)
        , hash_code_(0)
        , patterns_(0)
    { }

    /*!
     * \brief Creates an searcher from unordered container
     * \param _umap  Input unordered container to copy from.
     * \param _a  An allocator object.
     */
    rk_searcher(const lookup_type& _umap,
                const allocator_type& _a)
        : lookup_(_umap, _a)
        , hash_code_(0)
        , patterns_(0)
    { }

    /*!
     * \brief Move constructor with allocator argument
     * \param _other Input searcher to move.
     * \param _a An allocator object.
     */
    rk_searcher(rk_searcher&& _other,
                const allocator_type& _a)
        : lookup_(std::move(_other), _a)
        , hash_code_(0)
        , patterns_(0)
    { }

    /*!
     * \brief Builds a searcher from an initializer_list.
     * \param _ilist  An initializer_list.
     * \param _n  Minimal initial number of buckets.
     * \param _hf  A hash functor.
     * \param _eq  A key equality functor.
     * \param _a  An allocator object.
     *
     *  Create an searcher consisting of copies of the elements in the
     *  list. This is linear in N (where N is _ilist.size()).
     */
    rk_searcher(std::initializer_list<value_type> _ilist,
                size_type _n = 0,
                const hasher& _hf = hasher(),
                const key_equal& _eq = key_equal(),
                const allocator_type& _a = allocator_type())
        : rk_searcher(_ilist.begin(), _ilist.end(), _n, _hf, _eq, _a)
    { }

    /// Same as above
    rk_searcher(std::initializer_list<value_type> _ilist,
                size_type _n,
                const allocator_type& _a)
        : rk_searcher(_ilist, _n, hasher(), key_equal(), _a)
    { }

    /// Same as above
    rk_searcher(std::initializer_list<value_type> _ilist,
                size_type _n, const hasher& _hf,
                const allocator_type& _a)
        : rk_searcher(_ilist, _n, _hf, key_equal(), _a)
    { }

    rk_searcher(const rk_searcher& _other)
        : lookup_(_other.lookup_)
        , head_(_other.head_)
        , tail_(_other.tail_)
        , hash_code_(_other.hash_code_)
        , patterns_(_other.patterns_)
    { }

    rk_searcher(rk_searcher&& _other)
        : lookup_(std::move(_other))
        , head_(std::move(_other.head_))
        , tail_(std::move(_other.tail_))
        , hash_code_(_other.hash_code_)
        , patterns_(_other.patterns_)
    {
        _other.hash_code_ = 0;
        _other.patterns_ = 0;
    }

    rk_searcher& operator=(const rk_searcher& _other)
    {
        if (std::addressof(&_other) == this)
            return (*this);

        lookup_ = _other.lookup_;
        head_ = _other.head_;
        tail_ = _other.tail_;
        hash_code_ = _other.hash_code_;
        patterns_ = _other.patterns_;
        return (*this);
    }

    rk_searcher& operator=(rk_searcher&& _other)
    {
        if (std::addressof(_other) == this)
            return (*this);

        lookup_ = std::move(_other.lookup_);
        head_ = std::move(_other.head_);
        tail_ = std::move(_other.tail_);
        hash_code_ = _other.hash_code_;
        patterns_ = _other.patterns_;
        _other.hash_code_ = 0;
        _other.patterns_ = 0;
        return (*this);
    }

    template<class _Val>
    void emplace(_Val&& _pattern)
    {
        reset();
        if (std::size(this->key_of(std::forward<_Val>(_pattern))) >= hash_width)
            lookup_.emplace(std::forward<_Val>(_pattern));
    }

    template<class _It>
    [[nodiscard]] std::pair<_It, const_local_iterator> search(_It _first, _It _last, size_t _max_length = npos) const noexcept {
        return search_comp(_first, _last, std::equal_to<>{}, _max_length);
    }

    template<class _It, class _Pred>
    [[nodiscard]] std::pair<_It, const_local_iterator> search_comp(_It _first, _It _last, _Pred&& _pred, size_t _max_length = npos) const noexcept {
        return search_impl(_first, _last, std::forward<_Pred>(_pred), _max_length);
    }

    auto begin() noexcept { return lookup_.begin(); }
    auto end() noexcept { return lookup_.end(); }

    auto begin() const noexcept { return lookup_.begin(); }
    auto end() const noexcept { return lookup_.end(); }

    void reset() noexcept { patterns_ = 0; }

private:
    template<class _It, class _Pred>
    [[nodiscard]] std::pair<_It, const_local_iterator> search_impl(_It _first, _It _last, _Pred&& _pred, size_t _max_length) const noexcept
    {
        static const_local_iterator e;

        if (lookup_.empty())
            return std::make_pair(_last, e);

        size_t available = std::distance(_first, _last);
        if (available < hash_width)
            return std::make_pair(_last, e);

        const size_t n = lookup_.size();
        bool reset = patterns_ != n;
        if (reset)
            patterns_ = n;

        _It end = std::prev(_last, static_cast<std::ptrdiff_t>(hash_width - 1));
        for (; _first != end; ++_first, --available)
        {
            if (reset)
            {
                if constexpr (is_string_type_v<key_type>)
                    hash_code_ = lookup_.bucket(key_type{ std::addressof(*_first), hash_width });
                else
                    hash_code_ = lookup_.bucket({ _first, _first + hash_width });

                // find the bucket
                head_ = lookup_.begin(hash_code_);
                tail_ = lookup_.end(hash_code_);
            }

            for (; head_ != tail_; ++head_)
            {
                const auto& key = key_of(*head_);
                const size_t k = std::size(key);
                if (k <= available && k <= _max_length)
                {
                    if (std::equal(_first, _first + k, std::begin(key), _pred))
                        return std::make_pair(_first, head_++);
                }
            }
            reset = true;
        }
        return std::make_pair(_last, e);
    }


    template<class _Key>
    static inline const _Key& key_of(const _Key& _k) {
        return _k;
    }

    template<class _Key, class _Value>
    static inline const _Key& key_of(const std::pair<const _Key, _Value>& _p) {
        return _p.first;
    }

    template<class _Key, class _Value>
    static inline const _Key& key_of(const std::pair<_Key, _Value>& _p) {
        return _p.first;
    }

private:
    lookup_type lookup_;
    mutable const_local_iterator head_;
    mutable const_local_iterator tail_;
    mutable size_t hash_code_;
    mutable size_t patterns_;
};





/*!
 * \brief Rabin-Karp searcher optimized for small muti-pattern set
 *
 */
template<
    class _Pattern,
    size_t _HashWidth,
    size_t _MaxSize,
    class _Traits = rk_traits<_Pattern, _HashWidth>,
    class _KeyEq = std::equal_to<>,
    class _Allocator = std::allocator<_Pattern>
>
class rk_searcher_sset
    : _Traits::fixed_hasher_type
    , _KeyEq
{
    using buckets_type = std::vector<size_t>;
    using storage_type = std::vector<_Pattern, _Allocator>;
    using const_local_iterator = typename buckets_type::const_iterator;
    using pattern_iterator = typename storage_type::const_iterator;

public:

    using traits_type = _Traits;

    using size_type = typename buckets_type::size_type;
    using difference_type = typename buckets_type::difference_type;

    using value_type = _Pattern;
    using key_type = typename traits_type::key_type;
    using hasher_type = typename traits_type::fixed_hasher_type;
    using key_equal = _KeyEq;
    using allocator_type = _Allocator;

    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = pattern_iterator;
    using const_iterator = pattern_iterator;

    // minimal length of searching substring
    static constexpr std::size_t hash_width = _HashWidth;
    // assume that L1D cache-line size is 64 bytes - it's true for the vast majority of modern processors
    static constexpr size_t L1D_CACHELINE_SIZE = 64;
    // threshold where we switch from linear search to binary search
    static constexpr size_t linear_search_threshold = 8 * L1D_CACHELINE_SIZE;
    // sentinel value for maximum length of searching substring
    static constexpr size_t npos = ~size_t(0);

    /// Default constructor.
    rk_searcher_sset()
        : head_(0)
        , offset_(0)
        , hash_code_(0)
        , count_(0)
        , collisions_(0)
        , reordering_(reordering_policy::none)
    { }

    /**
     *  \brief  Builds a searcher from a range.
     *  \param  _first  An input iterator.
     *  \param  _last  An input iterator.
     *  \param _n  Minimal initial number of buckets.
     *  \param _hf  A hash functor.
     *  \param _eq  A key equality functor.
     *  \param _a  An allocator object.
     *
     *  Create a searcher consisting of copies of the elements from
     *  [_first,_last).  This is linear in N (where N is
     *  distance(_first,_last)).
     */
    template<typename _InputIterator>
    rk_searcher_sset(_InputIterator _first, _InputIterator _last,
                     size_type _n = 0,
                     const hasher_type& _hf = hasher_type(),
                     const key_equal& _eq = key_equal(),
                     const allocator_type& _a = allocator_type())
        : hasher_type(_hf)
        , key_equal(_eq)
        , patterns_(_a)
        , head_(0)
        , offset_(0)
        , hash_code_(0)
        , count_(0)
        , collisions_(0)
        , reordering_(reordering_policy::none)
    {
        if constexpr (is_random_access_v<_InputIterator>)
        {
            (void)_n;
            reserve(std::distance(_first, _last));
            for (; _first != _last; ++_first)
            {
                if (std::size(key_of(*_first)) < hash_width)
                    continue;
                insert(*_first);
            }
        }
        else
        {
            reserve(_n);
            for (; _first != _last; ++_first)
            {
                if (std::size(key_of(*_first)) < hash_width)
                    continue;
                insert(*_first);
            }
            shrink();
        }

        if (!prefer_linear_search())
        {
            // sort the buckets and patterns
            // note: since we have parallel arrays
            // we cant use std::sort() here
            insertion_sort_zipped(buckets_.begin(), buckets_.end(),
                                  patterns_.begin(), patterns_.end());
        }
    }

    /// Same as above
    template<typename _InputIterator>
    rk_searcher_sset(_InputIterator _first, _InputIterator _last,
                     size_type _n,
                     const allocator_type& _a)
        : rk_searcher_sset(_first, _last, _n, hasher_type(), _a)
    { }

    /// Same as above
    template<typename _InputIterator>
    rk_searcher_sset(_InputIterator _first, _InputIterator _last,
                     size_type _n, const hasher_type& _hf,
                     const allocator_type& _a)
        : rk_searcher_sset(_first, _last, _n, _hf, _a)
    { }

    /*!
     * \brief Copy constructor with allocator argument.
     * \param  _umap  Input unordered container to copy.
     * \param  _a  An allocator object.
     */
    template<class _Hashtable>
    rk_searcher_sset(const _Hashtable& _umap,
                     const allocator_type& _a)
        : rk_searcher_sset(std::begin(_umap), std::end(_umap), std::size(_umap), _a)
    { }


    /*!
     * \brief  Builds a searcher from an initializer_list.
     * \param  _ilist  An initializer_list.
     * \param _n  Minimal initial number of buckets.
     * \param _hf  A hash functor.
     * \param _eq  A key equality functor.
     * \param _a  An allocator object.
     *
     *  Create a searcher consisting of copies of the elements in the
     *  list. This is linear in N (where N is _ilist.size()).
     */
    rk_searcher_sset(std::initializer_list<value_type> _ilist,
                     const hasher_type& _hf = hasher_type(),
                     const key_equal& _eq = key_equal(),
                     const allocator_type& _a = allocator_type())
        : rk_searcher_sset(_ilist.begin(), _ilist.end(), _ilist.size(), _hf, _eq, _a)
    { }

    /// Same as above
    rk_searcher_sset(std::initializer_list<value_type> _ilist,
                     const allocator_type& _a)
        : rk_searcher_sset(_ilist, hasher_type(), _a)
    { }

    /// Same as above
    rk_searcher_sset(std::initializer_list<value_type> _ilist,
                     const hasher_type& _hf,
                     const allocator_type& _a)
        : rk_searcher_sset(_ilist.begin(), _ilist.end(), _ilist.size(), _hf, key_equal(), _a)
    { }

    rk_searcher_sset(const rk_searcher_sset& _other)
        : buckets_(_other.buckets_)
        , patterns_(_other.patterns_)
        , head_(_other.head_)
        , offset_(_other.offset_)
        , hash_code_(_other.hash_code_)
        , count_(_other.count_)
        , collisions_(_other.collisions_)
        , reordering_(_other.reordering_)
    { }

    rk_searcher_sset(rk_searcher_sset&& _other)
        : buckets_(std::move(_other.buckets_))
        , patterns_(std::move(_other.patterns_))
        , head_(_other.head_)
        , offset_(_other.offset_)
        , hash_code_(_other.hash_code_)
        , count_(_other.count_)
        , collisions_(_other.collisions_)
        , reordering_(_other.reordering_)
    {
        _other.head_ = 0;
        _other.offset_ = 0;
        _other.hash_code_ = 0;
        _other.count_ = 0;
        _other.collisions_ = 0;
        _other.reordering_ = reordering_policy::none;
    }

    rk_searcher_sset& operator=(const rk_searcher_sset& _other)
    {
        if (std::addressof(_other) == this)
            return (*this);

        buckets_ = _other.buckets_;
        patterns_ = _other.patterns_;
        head_ = _other.head_;
        offset_ = _other.offset_;
        hash_code_ = _other.hash_code_;
        count_ = _other.count_;
        collisions_ = _other.collisions_;
        reordering_ = _other.reordering_;
    }

    rk_searcher_sset& operator=(rk_searcher_sset&& _other)
    {
        if (std::addressof(_other) == this)
            return (*this);

        buckets_ = std::move(_other.buckets_);
        patterns_ = std::move(_other.patterns_);
        head_ = _other.head_;
        offset_ = _other.offset_;
        hash_code_ = _other.hash_code_;
        count_ = _other.count_;
        collisions_ = _other.collisions_;
        reordering_ = _other.reordering_;

        _other.head_ = 0;
        _other.offset_ = 0;
        _other.hash_code_ = 0;
        _other.count_ = 0;
        _other.collisions_ = 0;
        _other.reordering_ = reordering_policy::none;
        return (*this);
    }

    size_t collisions() const noexcept { return collisions_; }

    const_iterator begin() const noexcept { return patterns_.begin(); }
    const_iterator end() const noexcept { return patterns_.end(); }

    void reset() noexcept { count_ = 0; }

    void set_reordering(reordering_policy __policy) { reordering_ = __policy; }
    reordering_policy reordering() const { return reordering_; }

    template<class _It>
    [[nodiscard]] std::pair<_It, pattern_iterator> search(_It _first, _It _last, size_t _max_length = npos) const noexcept
    {
        return search_comp(_first, _last, std::equal_to<>{}, _max_length);
    }

    template<class _It, class _Pred>
    [[nodiscard]] std::pair<_It, pattern_iterator> search_comp(_It _first, _It _last, _Pred&& _pred, size_t _max_length = npos) const noexcept
    {
        switch (reordering_)
        {
        case reordering_policy::mtf:
            return search_impl_aux<reordering_policy::mtf>(_first, _last, std::forward<_Pred>(_pred), _max_length);
        case reordering_policy::transpose:
            return search_impl_aux<reordering_policy::transpose>(_first, _last, std::forward<_Pred>(_pred), _max_length);
        default:
            break;
        }
        return search_impl_aux<reordering_policy::none>(_first, _last, std::forward<_Pred>(_pred), _max_length);
    }

private:
    void insert(const_reference _pattern)
    {
        if (buckets_.size() > _MaxSize)
            throw std::overflow_error("rk_searcher_sset overflows");

        if (std::size(key_of(_pattern)) < _HashWidth)
            return;

        const auto& key = key_of(_pattern);
        const size_t h = hasher_type::operator()(key);
        auto bucketIt = std::find(buckets_.begin(), buckets_.end(), h);
        if (bucketIt == buckets_.end())
        {
            emplace_pattern(h, _pattern);
            return;
        }

        auto patternIt = patterns_.begin() + std::distance(buckets_.begin(), bucketIt);
        if (key_equal::operator()(key_of(*patternIt), key))
            *patternIt = _pattern;
        else
            insert_pattern(patternIt, bucketIt, h, _pattern);
    }

    void emplace_pattern(size_t __h, const_reference _pattern)
    {
        buckets_.emplace_back(__h);
        patterns_.emplace_back(_pattern);
    }

    void insert_pattern(typename storage_type::iterator _patternIt,
                        typename buckets_type::iterator _bucketIt,
                        size_t _h, const_reference _pattern)
    {
        while (_bucketIt != buckets_.end() && (*_bucketIt == _h))
        {
            ++_bucketIt;
            ++_patternIt;
        }
        buckets_.insert(_bucketIt, _h);
        patterns_.insert(_patternIt, _pattern);
        ++collisions_;
    }

    void reserve(size_type n)
    {
        buckets_.reserve(n);
        patterns_.reserve(n);
    }

    void shrink()
    {
        buckets_.shrink_to_fit();
        patterns_.shrink_to_fit();
    }

    bool prefer_linear_search() const
    {
        return (buckets_.size() < linear_search_threshold);
    }

    template<reordering_policy _Policy, class _It, class _Pred>
    [[nodiscard]] std::pair<_It, pattern_iterator> search_impl_aux(_It _first, _It _last, _Pred&& _pred, size_t _max_length) const
    {
        if (prefer_linear_search())
            return search_impl<_Policy, false, _It, _Pred>(_first, _last, std::forward<_Pred>(_pred), _max_length);
        else
            return search_impl<_Policy, true, _It, _Pred>(_first, _last, std::forward<_Pred>(_pred), _max_length);
    }

    template<reordering_policy _Policy, bool _Sorted, class _It, class _Pred>
    [[nodiscard]] std::pair<_It, pattern_iterator> search_impl(_It _first, _It _last, _Pred&& _pred, size_t _max_length) const
    {
        if (buckets_.empty())
            return std::make_pair(_last, patterns_.end());

        size_t available = std::distance(_first, _last);
        if (available < hash_width)
            return std::make_pair(_last, patterns_.end());

        const size_type n = buckets_.size();
        bool reset = count_ != n;
        if (reset)
            count_ = n;

        _It end = std::prev(_last, static_cast<std::ptrdiff_t>(hash_width - 1));
        for (; _first != end; ++_first, --available)
        {
            if (reset)
            {
                if constexpr (is_string_type_v<key_type>)
                    hash_code_ = hasher_type::operator()(key_type{ std::addressof(*_first), hash_width });
                else
                    hash_code_ = hasher_type::operator()({ _first, _first + hash_width });

                // find the bucket
                head_ = std::distance(buckets_.begin(), bucket<_Sorted>(hash_code_));
                offset_ = head_;
            }

            auto patternFirst = patterns_.begin() + offset_;
            auto bucketFirst = buckets_.begin() + offset_;
            auto patternIt = patternFirst;
            auto bucketIt = bucketFirst;
            for (; (bucketIt != buckets_.end()) && (*bucketIt == hash_code_); ++bucketIt, ++patternIt, ++offset_)
            {
                const auto& key = key_of(*patternIt);
                const size_t k = std::size(key);
                if (k <= available && k <= _max_length)
                {
                    if (std::equal(_first, _first + k, std::begin(key), _pred))
                    {
                        // reorder patterns inside bucket
                        patternIt = reorderer<_Policy>::reorder(patternIt, patternFirst, patterns_.end());
                        ++offset_;
                        return std::make_pair(_first, patternIt);
                    }
                }
            }
            reset = true;
        }
        return std::make_pair(_last, patterns_.end());
    }

    template<bool _Sorted>
    const_local_iterator bucket(size_t _h) const
    {
        if constexpr (_Sorted)
            return std::lower_bound(buckets_.begin(), buckets_.end(), _h);
        else
            return std::find(buckets_.begin(), buckets_.end(), _h);
    }

    template<class _Key>
    static inline const _Key& key_of(const _Key& k) {
        return k;
    }

    template<class _Key, class _Value>
    static inline const _Key& key_of(const std::pair<const _Key, _Value>& _p) {
        return _p.first;
    }
    template<class _Key, class _Value>
    static inline const _Key& key_of(const std::pair<_Key, _Value>& _p) {
        return _p.first;
    }

    /// This is a helper function for the insertion sort routine.
    template<class _RAIt1, class _RAIt2>
    static void linear_insert_zipped(_RAIt1 _last1, _RAIt2 _last2)
    {
        auto val1 = std::move(*_last1);
        _RAIt1 next1 = _last1;
        --next1;

        auto val2 = std::move(*_last2);
        _RAIt2 next2 = _last2;
        --next2;

        while (val1 < *next1)
        {
            *_last1 = std::move(*next1);
            _last1 = next1;
            --next1;

            *_last2 = std::move(*next2);
            _last2 = next2;
            --next2;
        }
        *_last1 = std::move(val1);
        *_last2 = std::move(val2);
    }

    /// This is a insertion sort routine.
    template<class _RAIt1, class _RAIt2>
    static void insertion_sort_zipped(_RAIt1 _first1, _RAIt1 _last1, _RAIt2 _first2, _RAIt2 _last2)
    {
        if (_first1 == _last1)
            return;

        if ((_last1 - _first1) != (_last2 - _first2))
            return;

        _RAIt1 i1 = _first1 + 1;
        _RAIt2 i2 = _first2 + 1;
        for (; i1 != _last1; ++i1, ++i2)
        {
            if (*i1 < *_first1)
            {
                auto val1 = std::move(*i1);
                auto val2 = std::move(*i2);
                std::move_backward(_first1, i1, i1 + 1);
                std::move_backward(_first2, i2, i2 + 1);
                *_first1 = std::move(val1);
                *_first2 = std::move(val2);
            }
            else
            {
                linear_insert_zipped(i1, i2);
            }
        }
    }

private:
    buckets_type buckets_;
    mutable storage_type patterns_;
    mutable size_t head_;
    mutable size_t offset_;
    mutable size_t hash_code_;
    mutable size_type count_;
    size_t collisions_;
    reordering_policy reordering_;
};
