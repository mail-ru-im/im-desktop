#include "stdafx.h"

#include "../corelib/remote_proc.h"

CORE_NS_BEGIN

namespace
{

    int64_t next_uid_ = 0;

    typedef std::tuple<int32_t, remote_proc_t> proc_info;

    std::unordered_map<int64_t, proc_info> callbacks_;

    void addref(const int64_t _id);

    void release(const int64_t _id);

    int64_t create(const remote_proc_t &_proc);

    int32_t get_counter(const proc_info &_info);

    const remote_proc_t& get_proc(const proc_info &_info);

    proc_info make_info(const int32_t _counter, const remote_proc_t &_proc);
}

struct remote_proc::impl
{
    impl(const remote_proc_t &_proc);

    const int64_t id_;

    remote_proc_t proc_;
};

remote_proc::impl::impl(const remote_proc_t &_proc)
    : id_(create(_proc))
{
}

remote_proc::remote_proc(const remote_proc_t _proc)
    : impl_(new impl(_proc))
{
}

remote_proc::~remote_proc()
{
    release(impl_->id_);
}

ifptr<ivalue> remote_proc::toValue() const
{
    return ifptr<ivalue>();
}

namespace
{
    void release(const int64_t _id)
    {
        assert(_id > 0);

        auto iter = callbacks_.find(_id);
        if (iter == callbacks_.end())
        {
            assert(!"release on an unknown callback");
            return;
        }

        const auto &info = std::get<1>(*iter);

        const auto ref_counter = get_counter(info);
        assert(ref_counter > 0);

        if (ref_counter == 1)
        {
            callbacks_.erase(iter);
            return;
        }

        const auto &proc = get_proc(info);
        assert(proc);

        iter->second = make_info(ref_counter - 1, proc);
    }

    int64_t create(const remote_proc_t &_proc)
    {
        assert(_proc);

        const auto uid = ++next_uid_;

        callbacks_.emplace(uid, make_info(1, _proc));

        return uid;
    }

    int32_t get_counter(const proc_info &_info)
    {
        return std::get<0>(_info);
    }

    const remote_proc_t& get_proc(const proc_info &_info)
    {
        return std::get<1>(_info);
    }

    proc_info make_info(const int32_t _counter, const remote_proc_t &_proc)
    {
        assert(_proc);

        return std::make_tuple(_counter, _proc);
    }
}

CORE_NS_END