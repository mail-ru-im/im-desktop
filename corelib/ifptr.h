#pragma once

#include "namespaces.h"

#include "ibase.h"

CORE_NS_BEGIN

template <class t_>
class ifptr
{
    static_assert(std::is_base_of<ibase, t_>::value, "ifptr works only with ibase descendants");

    mutable t_* p_;

    mutable bool auto_release_;

    void copy(const ifptr& _copy)
    {
        assert(&_copy != this);

        release_owned_p();

        p_ = _copy.p_;
        auto_release_ = _copy.auto_release_;

        addref_owned_p();
    }

    void release_owned_p()
    {
        if (p_ && auto_release_)
        {
            p_->release();
        }
    }

    void addref_owned_p()
    {
        if (p_ && auto_release_)
        {
            p_->addref();
        }
    }

public:
    ifptr()
        : p_(nullptr)
        , auto_release_(false)
    {
    }

    ifptr(t_* _p, const bool _auto_release = true)
        : p_(_p)
        , auto_release_(_auto_release)
    {
    }

    ifptr(const ifptr<t_>& _copy)
        : p_(nullptr)
        , auto_release_(false)
    {
        copy(_copy);
    }

    ifptr(const ifptr<t_>&& _rv_copy)
        : p_(_rv_copy.p_)
        , auto_release_(_rv_copy.auto_release_)
    {
        _rv_copy.p_ = nullptr;
        _rv_copy.auto_release_ = false;
    }

    ~ifptr()
    {
        release_owned_p();
    }

    t_* detach()
    {
        t_* pt = p_;

        p_ = nullptr;
        auto_release_ = false;

        return pt;
    }

    t_* get() const
    {
        return p_;
    }

    operator bool() const
    {
        return get();
    }

    bool empty() const
    {
        return !get();
    }

    t_* operator->() const
    {
        return get();
    }

    ifptr& operator=(const ifptr& _copy)
    {
        if (&_copy != this)
        {
            copy(_copy);
        }

        return *this;
    }

    ifptr& operator=(const ifptr&& _rv_copy)
    {
        release_owned_p();

        p_ = _rv_copy.p_;
        auto_release_ = _rv_copy.auto_release_;

        _rv_copy.p_ = nullptr;
        _rv_copy.auto_release_ = false;

        return *this;
    }
};

CORE_NS_END