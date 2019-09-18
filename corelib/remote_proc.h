#pragma once

#include "namespaces.h"

#include "ifptr.h"

#include "ivalue.h"

CORE_NS_BEGIN

class coll_helper;

typedef std::function<void(const coll_helper&)> remote_proc_t;

class remote_proc final
{
public:
    remote_proc(const remote_proc_t _proc);

    ~remote_proc();

    ifptr<ivalue> toValue() const;

private:
    struct impl;

    std::unique_ptr<impl> impl_;

};

CORE_NS_END