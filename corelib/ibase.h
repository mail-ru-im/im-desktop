#pragma once

#include "namespaces.h"

CORE_NS_BEGIN

struct ibase
{
    virtual int32_t addref() = 0;
    virtual int32_t release() = 0;

    virtual ~ibase() {}
};

CORE_NS_END