#ifndef __PRODUCT_H_
#define __PRODUCT_H_

namespace build
{
    extern int is_build_icq;

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

        return !!is_build_icq;
    }

    inline bool is_agent() noexcept
    {
        if (is_biz() || is_dit())
            return false;

        return !is_build_icq;
    }

    template <typename T> const T& GetProductVariant(const T& _icq, const T& _agent, const T& _biz, const T& _dit)
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

    template <typename T> T* GetProductVariant(T* _icq, T* _agent, T* _biz, T* _dit)
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

    inline QString ProductName()
    {
        return GetProductVariant(qsl("ICQ"), qsl("Agent"), qsl("Myteam"), qsl("Messenger"));
    }

    inline QString ProductNameShort()
    {
        return GetProductVariant(qsl("icq"), qsl("agent"), qsl("myteam"), qsl("messenger"));
    }

    inline QString ProductNameFull()
    {
        return GetProductVariant(qsl("ICQ"), qsl("Mail.ru Agent"), qsl("Myteam"), qsl("Messenger"));
    }
}

#endif // __PRODUCT_H_
