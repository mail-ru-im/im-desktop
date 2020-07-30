#include "stdafx.h"
#include "smartreply_marker.h"

#include "../../corelib/collection_helper.h"
#include "../../common.shared/smartreply/smartreply_types.h"

namespace core
{
    namespace smartreply
    {
        marker::marker()
            : msgid_(-1)
            , type_(smartreply::type::invalid)
        {
        }

        bool marker::unserialize(const coll_helper& _coll)
        {
            if (!_coll.is_value_exist("type") || !_coll.is_value_exist("msgId"))
                return false;

            type_ = (core::smartreply::type)_coll.get_value_as_int("type");
            msgid_ = _coll.get_value_as_int64("msgId");
            return true;
        }
    }
}