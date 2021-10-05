#include "stdafx.h"

#include "remote_proc.h"

#include "collection_helper.h"

namespace core
{

    void coll_helper::set_value_as_proc(std::string_view _name, const remote_proc_t _proc)
    {
        im_assert(!_name.empty());
        im_assert(_proc);

        //remote_proc_t::create(_callback);
    }

}