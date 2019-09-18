#ifndef __PRODUCT_CORE_H_
#define __PRODUCT_CORE_H_



namespace build
{
    extern int32_t is_core_icq;

    constexpr inline bool is_biz() noexcept
    {
#ifdef BIZ_VERSION
        return true;
#else
        return false;
#endif //BIZ_VERSION
    }

    constexpr inline bool is_dit() noexcept
    {
#ifdef DIT_VERSION
        return true;
#else
        return false;
#endif //DIT_VERSION
    }

    inline bool is_icq() noexcept
    {
        if (is_biz() || is_dit())
            return false;

        return !!is_core_icq;
    }

    inline bool is_agent() noexcept
    {
        if (is_biz() || is_dit())
            return false;

        return !is_core_icq;
    }

    template <typename T> T& get_product_variant(T& _icq, T& _agent, T& _biz, T& _dit)
    {
        if (is_icq())
            return _icq;

        if (is_biz())
            return _biz;

        if (is_dit())
            return _dit;

        if (is_agent())
            return _agent;

        assert(false);
        return _icq;
    }

    template <typename T> T* get_product_variant(T* _icq, T* _agent, T* _biz, T* _dit)
    {
        if (is_icq())
            return _icq;

        if (is_biz())
            return _biz;

        if (is_dit())
            return _dit;

        if (is_agent())
            return _agent;

        assert(false);
        return _icq;
    }

    inline void set_core(const int32_t _is_core_icq) noexcept
    {
        is_core_icq = _is_core_icq;
    }

    inline std::string product_name()
    {
        return get_product_variant("ICQ", "Agent", "Myteam", "Messenger");
    }

    inline std::string product_name_short()
    {
        return get_product_variant("icq", "agent", "myteam", "messenger");
    }

    inline std::string product_name_full()
    {
        return get_product_variant("ICQ", "Mail.ru Agent", "Myteam", "Messenger");
    }
}

#endif // __PRODUCT_CORE_H_
