#include "stdafx.h"
#include "session_info.h"

#include "../utils/gui_coll_helper.h"

namespace Data
{
    void SessionInfo::unserialize(const core::coll_helper& _coll)
    {
        os_ = QString::fromUtf8(_coll.get_value_as_string("os")).trimmed();
        clientName_ = QString::fromUtf8(_coll.get_value_as_string("name")).trimmed();
        location_ = QString::fromUtf8(_coll.get_value_as_string("location")).trimmed();
        ip_ = QString::fromUtf8(_coll.get_value_as_string("ip")).trimmed();
        hash_ = QString::fromUtf8(_coll.get_value_as_string("hash")).trimmed();
        startedTime_ = QDateTime::fromSecsSinceEpoch(_coll.get_value_as_int64("started"));
        isCurrent_ = _coll.get_value_as_bool("current");

        assert(!hash_.isEmpty());
    }

    const SessionInfo& SessionInfo::emptyCurrent()
    {
        static const SessionInfo info = []()
        {
            SessionInfo i;
            i.isCurrent_ = true;
            return i;
        }();

        return info;
    }
}