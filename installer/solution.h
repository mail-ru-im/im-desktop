#pragma once

#ifdef SOLUTION_DEF_installer_agent
#define  __AGENT
#endif //agent

#ifdef SOLUTION_DEF_installer_icq
#define __ICQ
#endif //icq

#ifdef SOLUTION_DEF_installer_biz
#define __BIZ
#endif //biz

#ifdef SOLUTION_DEF_installer_dit
#define __DIT
#endif //biz

namespace build
{
    static bool is_agent()
    {
#ifdef __AGENT
        return true;
#else
        return false;
#endif //__AGENT
    }

    static bool is_icq()
    {
#ifdef __ICQ
        return true;
#else
        return false;
#endif //__ICQ
    }

    static bool is_biz()
    {
#ifdef __BIZ
        return true;
#else
        return false;
#endif //__BIZ
    }

    static bool is_dit()
    {
#ifdef __DIT
        return true;
#else
        return false;
#endif //__DIT
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

        return _icq;
    }
}