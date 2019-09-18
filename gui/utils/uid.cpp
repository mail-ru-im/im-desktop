#include "stdafx.h"

#include "uid.h"

namespace Utils
{

    int64_t get_uid()
    {
        static int64_t seq = 0;
        return ++seq;
    }

}