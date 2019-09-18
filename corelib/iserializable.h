#pragma once

#include "ifptr.h"

namespace core
{
    class coll_helper;

    struct iserializable
    {
        virtual ~iserializable() {}
        virtual void serialize(Out coll_helper &_coll) const = 0;

        virtual bool unserialize(const coll_helper &_coll) = 0;

    };

}
