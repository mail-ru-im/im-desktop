#pragma once

namespace core::wim
{
    template <typename T, typename key_getter>
    class packet_queue
    {
        using set_key = typename std::result_of<decltype(&key_getter::key)(key_getter, T)>::type;
    public:
        bool is_empty() const;
        bool is_limit_reached() const;
        bool can_pop_element() const;

        using container = std::list<T>;
        using iterator = typename container::iterator;
        using const_iterator = typename container::const_iterator;

        iterator begin() noexcept { return queue_.begin(); }
        const_iterator begin() const noexcept { return queue_.begin(); }
        const_iterator cbegin() const noexcept { return queue_.cbegin(); }

        iterator end() noexcept { return queue_.end(); }
        const_iterator end() const noexcept { return queue_.end(); }
        const_iterator cend() const noexcept { return queue_.cend(); }

        [[nodiscard]] std::shared_ptr<T> pop();

        void clear();

        void set_limit(size_t _limit);

        void enqueue(const T& _element);
        void enqueue(T&& _element);

        void allow_using(const set_key& _key);

    private:
        template <typename Value>
        void emplace(Value&& _element);

        auto contains_extracted_predicate() const
        {
            return [this](const T& _elem) -> bool
            {
                return extracted_elements_.find(key_getter_.key(_elem)) == extracted_elements_.end();
            };
        }

    private:
        std::set<set_key> extracted_elements_;
        container queue_;
        size_t limit_ = 1;
        key_getter key_getter_;
    };

    template <typename T, typename key_getter>
    inline bool packet_queue<T, key_getter>::is_empty() const
    {
        return queue_.empty();
    }

    template <typename T, typename key_getter>
    inline bool packet_queue<T, key_getter>::is_limit_reached() const
    {
        return extracted_elements_.size() >= limit_;
    }

    template <typename T, typename key_getter>
    inline bool packet_queue<T, key_getter>::can_pop_element() const
    {
        return !is_empty() && !is_limit_reached() && std::any_of(queue_.cbegin(), queue_.cend(), contains_extracted_predicate());
    }

    template <typename T, typename key_getter>
    inline std::shared_ptr<T> packet_queue<T, key_getter>::pop()
    {
        auto it = std::find_if(queue_.begin(), queue_.end(), contains_extracted_predicate());
        if (it == queue_.end())
        {
            im_assert(!"element did not pop");
            return std::make_shared<T>();
        }
        const auto element = std::make_shared<T>(std::move(*it));
        queue_.erase(it);

        extracted_elements_.insert(key_getter_.key(*element));
        return element;
    }

    template <typename T, typename key_getter>
    inline void packet_queue<T, key_getter>::clear()
    {
        queue_.clear();
        extracted_elements_.clear();
    }

    template <typename T, typename key_getter>
    inline void packet_queue<T, key_getter>::set_limit(size_t _limit)
    {
        limit_ = _limit;
    }

    template <typename T, typename key_getter>
    inline void packet_queue<T, key_getter>::enqueue(const T& _value)
    {
        emplace(_value);
    }

    template <typename T, typename key_getter>
    inline void packet_queue<T, key_getter>::enqueue(T&& _value)
    {
        emplace(std::move(_value));
    }

    template<typename T, typename key_getter>
    inline void packet_queue<T, key_getter>::allow_using(const set_key& _key)
    {
        extracted_elements_.erase(_key);
    }

    template <typename T, typename key_getter>
    template <typename Value>
    inline void packet_queue<T, key_getter>::emplace(Value&& _value)
    {
        auto iter = queue_.begin();
        while (iter != queue_.end())
        {
            if (key_getter_.priority(_value) < key_getter_.priority(*iter))
            {
                queue_.insert(iter, std::forward<Value>(_value));
                return;
            }
            ++iter;
        }
        queue_.push_back(std::forward<Value>(_value));
    }
}; // namespace core::wim
